#include <tasm/asm/backends/tobj.h>
#include <tasm/asm/lower.h>
#include <tasm/asm/sema.h>

#include <tasm/asr/tostr.h>

#include <tc48/cpu/regs.h>
#include <tc48/cpu/instr.h>
#include <tc48/cpu/encode.h>

#include <tasm/parser/literals.h>
#include <tasm/gen/tscs.h>

#include <stdlib.h>

typedef TasmTObjBeSymbol SymbolMeta;
typedef TasmTObjBeSec    SectionMeta;

typedef TasmTObjBeSecRelAddr SectionRelAddr;

// this kinda sucks but is a lot easier to
// use than for example something like
//   struct { usize value; bool has_value; }
// and nobody declares SIZE_MAX amount of symbols
// anyway so this should work just fine (probably).
#define SENTINEL ((usize)-1)

// 0sQQ
#define RELOC_PLACEHOLDER_6 \
    TC48_TRYTE(2,2,2,2,2,2)
// 0sNEON
#define RELOC_PLACEHOLDER_12 \
    TC48_QUARTER(2,1,2,1,1,2, 2,2,0,2,1,2)
// 0sDEADBEEF
#define RELOC_PLACEHOLDER_24      \
    TC48_HALF(                    \
        1,1,1,1,1,2, 1,0,1,1,1,1, \
        1,0,2,1,1,2, 1,1,2,1,2,0  \
    )
// 0sDEADBEEFDEADBEEF
#define RELOC_PLACEHOLDER_48                                \
    TC48_WORD(                                              \
        1,1,1,1,1,2, 1,0,1,1,1,1, 1,0,2,1,1,2, 1,1,2,1,2,0, \
        1,1,1,1,1,2, 1,0,1,1,1,1, 1,0,2,1,1,2, 1,1,2,1,2,0  \
    )

void tasm_tobj_asm_init(TasmBackendTObj* as, const TasmAsrBuf* asr, TasmDiagEngine* diag) {
    as->asr = asr;
    as->diag = diag;

    tobj_bldr_init(&as->bldr);

    as->sym_map    = calloc(asr->count, sizeof(TasmTObjBeSymbol));
    as->sec_map    = calloc(asr->count, sizeof(TasmTObjBeSec));
    as->item_addrs = calloc(asr->count, sizeof(SectionRelAddr));

    for (usize i = 0; i < asr->count; i++) {
        as->sym_map[i].builder_sym_idx = SENTINEL;
        as->sym_map[i].parent_global_id = SENTINEL;
    }
}

void tasm_tobj_asm_free(TasmBackendTObj* as) {
    tobj_bldr_free(&as->bldr);
    free(as->sym_map);
    free(as->sec_map);
    free(as->item_addrs);
}

static bool dir_kind_to_binding(TasmAsrDirKind kind, tobj_binding* out) {
    switch (kind) {
    case TASM_DIR_GLOBAL: *out = TOBJ_BINDING_GLOBAL; return true;
    case TASM_DIR_LOCAL:  *out = TOBJ_BINDING_LOCAL;  return true;
    case TASM_DIR_WEAK:   *out = TOBJ_BINDING_WEAK;   return true;
    default: return false;
    }
}

static StringView get_section_name(const TasmAsrDir* dir) {
    StringView sec_name = SV(".data");
    if (VECTOR_SIZE(&dir->operands) >= 1) {
        const TasmOperand* op = &dir->operands.begin[0];
        if (op->kind == TASM_OPERAND_LABEL) {
            sec_name = op->label.name;
        } else if (op->kind == TASM_OPERAND_STR) {
            sec_name = op->str;
        }
    }
    return sec_name;
}

// while writing this I realized that the entire source document
// should probably be converted to TSCS before it goes through the lexer.
// so the whole assembler can operate directly on TSCS instead of ascii/unicode
// but now i don't want to interrupt my work on this backend,
// so i'll leave it this way (for now) even though it sucks
static usize register_string(tobj_builder* bldr, StringView sv) {
    tc48_tryte* buf = malloc(sv.len * sizeof(tc48_tryte));
    for (usize i = 0; i < sv.len; i++) {
        unsigned char ch = (unsigned char)sv.data[i];
        buf[i] = ch <= 127 ? tscs_from_ascii[ch] : 0;
    }
    usize idx = tobj_bldr_add_string(bldr, buf, (tc48_half)sv.len);
    free(buf);
    return idx;
}

static usize register_symbol_name(TasmBackendTObj* as, const TasmLabel* label, usize parent_global_id) {
    if (!label->is_local || parent_global_id == SENTINEL) {
        return register_string(&as->bldr, label->name);
    }

    TasmAsrItem* parent = &as->asr->items[parent_global_id];
    StringView parent_name = parent->as.label.name;
    usize full_len = parent_name.len + 1 + label->name.len;

    tc48_tryte* buf = malloc(full_len * sizeof(tc48_tryte));
    for (usize i = 0; i < parent_name.len; i++) {
        unsigned char ch = (unsigned char)parent_name.data[i];
        buf[i] = ch <= 127 ? tscs_from_ascii[ch] : 0;
    }
    buf[parent_name.len] = tscs_from_ascii[(unsigned char)'.'];
    for (usize i = 0; i < label->name.len; i++) {
        unsigned char ch = (unsigned char)label->name.data[i];
        buf[parent_name.len + 1 + i] = ch <= 127 ? tscs_from_ascii[ch] : 0;
    }

    usize idx = tobj_bldr_add_string(&as->bldr, buf, (tc48_half)full_len);
    free(buf);
    return idx;
}

static bool dir_emits_data(TasmAsrDirKind kind) {
    switch (kind) {
    case TASM_DIR_TRYTE: case TASM_DIR_QUARTER:
    case TASM_DIR_HALF:  case TASM_DIR_WORD:
    case TASM_DIR_STRING:
    case TASM_DIR_ORG:
        return true;
    default:
        return false;
    }
}

static void update_symbol_binding(
    TasmBackendTObj* as, StringView target, bool is_local,
    usize scope_id, tobj_binding binding, TasmSourceSpan span
) {
    for (usize j = 0; j < as->asr->count; j++) {
        TasmAsrItem* other = &as->asr->items[j];
        if (other->kind != TASM_IR_LABEL || !sv_eql(other->as.label.name, target)) {
            continue;
        }
        if (other->as.label.is_local != is_local) {
            continue;
        }

        SymbolMeta* meta = &as->sym_map[other->id];
        if (is_local && meta->parent_global_id != scope_id) {
            continue;
        }

        if (meta->binding_specified && meta->binding != binding) {
            tasm_report_error(as->diag, span, "conflicting binding for symbol '"SV_FMT"'", SV_FARG(target));
            return;
        }

        meta->binding_specified = true;
        meta->binding = binding;

        if (meta->builder_sym_idx != SENTINEL && meta->builder_sym_idx < VECTOR_SIZE(&as->bldr.symbols)) {
            as->bldr.symbols.begin[meta->builder_sym_idx].binding = binding;
        }
        return;
    }
}

static void handle_section_directive(TasmBackendTObj* as, TasmAsrItem* item) {
    TasmAsrDir* dir = &item->as.directive;
    usize sec_idx = as->sec_map[item->id].idx;

    for (usize i = 0; i < item->id; i++) {
        TasmAsrItem* prev = &as->asr->items[i];
        if (prev->kind != TASM_IR_DIRECTIVE || prev->as.directive.kind != TASM_DIR_SECTION) {
            continue;
        }
        if (as->sec_map[prev->id].idx == sec_idx) {
            as->current_section_idx = sec_idx;
            return;
        }
    }

    usize name_idx = register_string(&as->bldr, get_section_name(dir));
    tobj_bldr_add_section(&as->bldr, &(tobj_bldr_section){
        .name_idx = name_idx,
        .align    = 1,
    });

    as->current_section_idx = sec_idx;
}

static void handle_label_definition(TasmBackendTObj* as, TasmAsrItem* item) {
    TasmLabel* label = &item->as.label;
    if (!label->is_local) {
        as->current_global_idx = item->id;
    }

    SectionRelAddr addr = as->item_addrs[item->id];
    if (addr.section_idx == SENTINEL) {
        tasm_report_error(as->diag, item->span, "label defined before %%section");
        return;
    }

    SymbolMeta* meta = &as->sym_map[item->id];
    usize parent_global_id = label->is_local ? as->current_global_idx : SENTINEL;
    usize name_idx = register_symbol_name(as, label, parent_global_id);

    // local by default (!)
    tobj_binding binding = TOBJ_BINDING_LOCAL;
    if (meta->binding_specified) {
        binding = meta->binding;
    }

    usize builder_sym_idx = VECTOR_SIZE(&as->bldr.symbols);
    tobj_bldr_add_symbol(&as->bldr, &(tobj_bldr_symbol) {
        .name_idx    = name_idx,
        .section_idx = addr.section_idx,
        .offset      = (tc48_half)addr.offset,
        .size        = 0,
        .binding     = binding,
    });

    *meta = (SymbolMeta){
        .name_idx          = name_idx,
        .builder_sym_idx   = builder_sym_idx,
        .binding_specified = meta->binding_specified,
        .binding           = binding,
        .section_idx       = addr.section_idx,
        .offset            = addr.offset,
        .parent_global_id  = parent_global_id,
    };
}

static void handle_extern_directive(TasmBackendTObj* as, TasmAsrDir* dir) {
    for (TasmOperand* op = dir->operands.begin; op < dir->operands.end; ++op) {
        if (op->kind != TASM_OPERAND_LABEL) continue;

        usize name_idx = register_string(&as->bldr, op->label.name);
        tobj_bldr_add_symbol(&as->bldr, &(tobj_bldr_symbol) {
            .name_idx    = name_idx,
            .section_idx = SENTINEL,
            .offset      = 0,
            .size        = 0,
            .binding     = TOBJ_BINDING_GLOBAL,
        });
    }
}

static void ensure_in_section(TasmBackendTObj* as, TasmSourceSpan span) {
    if (as->current_section_idx == SENTINEL) {
        tasm_report_error(as->diag, span, "instruction or data directive used before %%section");
    }
}

static void advance_offset_for_item(TasmBackendTObj* as, TasmAsrItem* item, usize* current_offset) {
    if (item->kind == TASM_IR_INSTR) {
        *current_offset += tasm_get_instr_size(as->diag, &item->as.instr);
    } else if (item->kind == TASM_IR_DIRECTIVE) {
        TasmAsrDir* dir = &item->as.directive;
        if (dir->kind != TASM_DIR_ORG && dir->kind != TASM_DIR_SECTION) {
            *current_offset += tasm_get_directive_size(as->diag, dir);
        }
    }
}

static void validate_binding_operands(TasmBackendTObj* as, TasmAsrDir* dir) {
    tobj_binding binding;
    if (dir_kind_to_binding(dir->kind, &binding)) {
        for (TasmOperand* op = dir->operands.begin; op < dir->operands.end; ++op) {
            if (op->kind != TASM_OPERAND_LABEL) {
                tasm_report_error(as->diag, op->span, SV_FMT" directive expects label operands", SV_FARG(tasm_dir_kind_to_str(dir->kind)));
            }
        }
    }
}

static void pass1(TasmBackendTObj* as) {
    as->current_section_idx = SENTINEL;
    as->current_global_idx = SENTINEL;
    usize current_offset = 0;
    usize section_count = 0;
    usize* section_offsets = calloc(as->asr->count, sizeof(usize));

    for (usize i = 0; i < as->asr->count; i++) {
        TasmAsrItem* item = &as->asr->items[i];

        if (item->kind == TASM_IR_DIRECTIVE) {
            TasmAsrDir* dir = &item->as.directive;

            validate_binding_operands(as, dir);

            if (dir->kind == TASM_DIR_SECTION) {
                StringView sec_name = get_section_name(dir);
                usize sec_idx = SENTINEL;

                for (usize j = 0; j < i; j++) {
                    TasmAsrItem* prev = &as->asr->items[j];
                    if (prev->kind != TASM_IR_DIRECTIVE || prev->as.directive.kind != TASM_DIR_SECTION) {
                        continue;
                    }
                    if (sv_eql(get_section_name(&prev->as.directive), sec_name)) {
                        sec_idx = as->sec_map[prev->id].idx;
                        break;
                    }
                }

                if (sec_idx == SENTINEL) {
                    sec_idx = section_count++;
                }

                as->sec_map[item->id].idx = sec_idx;
                as->current_section_idx = sec_idx;
                current_offset = section_offsets[sec_idx];
            } else if (dir_emits_data(dir->kind)) {
                ensure_in_section(as, dir->span);

                if (dir->kind == TASM_DIR_ORG) {
                    if (VECTOR_SIZE(&dir->operands) != 1) {
                        tasm_report_error(as->diag, dir->span, "%%org directive expects exactly one operand");
                    } else {
                        current_offset = (usize)dir->operands.begin[0].imm;
                    }
                }
            }
        }

        if (item->kind == TASM_IR_INSTR) {
            ensure_in_section(as, item->span);
        }
        if (item->kind == TASM_IR_LABEL) {
            if (item->as.label.is_local) {
                as->sym_map[item->id].parent_global_id = as->current_global_idx;
            } else {
                as->current_global_idx = item->id;
            }
        }

        as->item_addrs[item->id] = (SectionRelAddr){
            as->current_section_idx,
            current_offset
        };

        advance_offset_for_item(as, item, &current_offset);
        if (as->current_section_idx != SENTINEL) {
            section_offsets[as->current_section_idx] = current_offset;
        }
    }

    free(section_offsets);
}
static void pass2(TasmBackendTObj* as) {
    as->current_section_idx = SENTINEL;
    as->current_global_idx = SENTINEL;

    for (usize i = 0; i < as->asr->count; i++) {
        TasmAsrItem* item = &as->asr->items[i];

        if (item->kind == TASM_IR_DIRECTIVE) {
            TasmAsrDir* dir = &item->as.directive;
            if (dir->kind == TASM_DIR_SECTION) {
                handle_section_directive(as, item);
            } else if (dir->kind == TASM_DIR_EXTERN) {
                handle_extern_directive(as, dir);
            } else {
                tobj_binding binding;
                if (dir_kind_to_binding(dir->kind, &binding)) {
                    for (TasmOperand* op = dir->operands.begin; op < dir->operands.end; ++op) {
                        if (op->kind == TASM_OPERAND_LABEL) {
                            update_symbol_binding(
                                as, op->label.name, op->label.is_local,
                                as->current_global_idx, binding, op->span
                            );
                        }
                    }
                }
            }
        } else if (item->kind == TASM_IR_LABEL) {
            handle_label_definition(as, item);
        }
    }
}

// no idea how this function works, made by ai
static usize find_symbol_index(TasmBackendTObj* as, StringView name, bool is_local, usize scope_id) {
    for (size_t j = 0; j < as->asr->count; j++) {
        TasmAsrItem* item = &as->asr->items[j];
        if (item->kind == TASM_IR_LABEL && sv_eql(item->as.label.name, name)) {
            if (item->as.label.is_local != is_local) {
                continue;
            }
            if (is_local && as->sym_map[item->id].parent_global_id != scope_id) {
                continue;
            }
            return as->sym_map[item->id].builder_sym_idx;
        }
    }

    tobj_bldr_symbols* syms = &as->bldr.symbols;
    for (size_t si = 0; si < VECTOR_SIZE(syms); si++) {
        tobj_bldr_string str = as->bldr.strings.begin[syms->begin[si].name_idx];
        if ((size_t)str.len != name.len) {
            continue;
        }
        bool match = true;
        for (size_t c = 0; c < name.len; c++) {
            unsigned char ch = (unsigned char)name.data[c];
            int16_t tscs_idx = (ch < 128) ? tscs_from_ascii[ch] : -1;
            tc48_tryte expected = (tscs_idx >= 0) ? (tc48_tryte)tscs_idx : 0;
            if (str.chars[c] != expected) {
                match = false;
                break;
            }
        }
        if (match) return si;
        else continue;
    }

    return (size_t)-1;
}

static bool resolve_operand_tobj(void* ctx, const TasmOperand* op, tc48_word* out) {
    (void)ctx;
    if (op->kind == TASM_OPERAND_IMM) {
        *out = (tc48_word)op->imm;
        return true;
    }
    if (op->kind == TASM_OPERAND_LABEL) {
        *out = 0;
        return true;
    }
    return false;
}

static void sec_ensure_size(tobj_bldr_section* sec, usize size) {
    while (VECTOR_SIZE(&sec->data) < size) {
        tobj_trytes_push(&sec->data, 0);
    }
}

static void emit_string_directive(TasmBackendTObj* as, TasmAsrDir* dir, tobj_bldr_section* sec, usize offset) {
    for (TasmOperand* op = dir->operands.begin; op < dir->operands.end; ++op) {
        usize count = 0;
        if (!tasm_parse_lit_string_chars(as->diag, op->span, op->str, NULL, &count)) {
            return;
        }

        int32_t* chars = malloc(count * sizeof(int32_t));
        if (!chars) {
            return;
        }

        if (tasm_parse_lit_string_chars(as->diag, op->span, op->str, chars, &count)) {
            for (usize c = 0; c < count; c++) {
                tc48_tryte tryte = (tc48_tryte)chars[c];
                if (VECTOR_SIZE(&sec->data) <= offset + c) {
                    tobj_trytes_push(&sec->data, tryte);
                } else {
                    sec->data.begin[offset + c] = tryte;
                }
            }
        }
        free(chars);
    }
}

static void emit_reloc_for_operand(
    TasmBackendTObj* as, const TasmOperand* op,
    usize section_idx, tc48_half offset,
    usize scope_id, tobj_reloc_type reloc_type
) {
    if (op->kind != TASM_OPERAND_LABEL) {
        return;
    }

    usize sym_idx = find_symbol_index(as, op->label.name, op->label.is_local, scope_id);
    if (sym_idx == SENTINEL) {
        tasm_report_error(as->diag, op->span, "undefined symbol: "SV_FMT, SV_FARG(op->label.name));
        return;
    }

    tobj_bldr_add_reloc(&as->bldr, (tobj_bldr_reloc){
        .section_idx = section_idx,
        .offset      = offset,
        .symbol_idx  = sym_idx,
        .type        = reloc_type,
    });
}

static void pass3_handle_instruction(TasmBackendTObj* as, TasmAsrItem* item, SectionRelAddr addr) {
    TasmResolver resolver = { .resolve_operand = resolve_operand_tobj, .context = as };
    tc48_instr instr;
    if (!tasm_lower_instr(NULL, &resolver, &item->as.instr, &instr)) {
        return;
    }

    tc48_word instr_size = tasm_get_instr_size(as->diag, &item->as.instr);

    // this sucks, tc48_encode should probably accept
    // something more abstract that tc48_memory but cares
    // about just one extra allocation (... for each instruction)
    // from a different perspective, it still uses less memory than average
    // javascript runtime just to print hello world, so maybe it's fine
    tc48_memory* tmp = tc48_mem_alloc(instr_size);
    tc48_encode(tmp, 0, &instr);

    tobj_bldr_section* sec = &as->bldr.sections.begin[addr.section_idx];
    sec_ensure_size(sec, addr.offset);

    for (usize t = 0; t < (usize)instr_size; t++) {
        tc48_tryte tryte = tc48_mem_load6(tmp, (tc48_word)t);
        if (VECTOR_SIZE(&sec->data) <= addr.offset + t) {
            tobj_trytes_push(&sec->data, tryte);
        } else {
            sec->data.begin[addr.offset + t] = tryte;
        }
    }
    tc48_mem_free(tmp);

    TasmInstr* ti = &item->as.instr;

    tc48_half imm_off = 0;
    bool has_imm_off = tasm_get_instr_imm_offset(ti, &imm_off);

    for (tc48_u8b oi = 0; oi < ti->num_operands; oi++) {
        const TasmOperand* op = &ti->operands[oi];
        if (op->kind != TASM_OPERAND_LABEL) {
            continue;
        }

        tc48_half reloc_off = (tc48_half)addr.offset;
        if (has_imm_off) {
            reloc_off = (tc48_half)(addr.offset + imm_off);
        }

        emit_reloc_for_operand(as, op, addr.section_idx, reloc_off, as->current_global_idx, TOBJ_RELOC_ABS);
    }
}

static void pass3_handle_directive(TasmBackendTObj* as, TasmAsrItem* item, SectionRelAddr addr) {
    TasmAsrDir* dir = &item->as.directive;
    tobj_bldr_section* sec = &as->bldr.sections.begin[addr.section_idx];

    sec_ensure_size(sec, addr.offset);

    switch (dir->kind) {
    case TASM_DIR_ORG: case TASM_DIR_SECTION:
    case TASM_DIR_EXTERN: case TASM_DIR_GLOBAL:
    case TASM_DIR_LOCAL: case TASM_DIR_WEAK:
        return;

    case TASM_DIR_STRING:
        emit_string_directive(as, dir, sec, addr.offset);
        return;

    case TASM_DIR_TRYTE: case TASM_DIR_QUARTER:
    case TASM_DIR_HALF:  case TASM_DIR_WORD:
        break; // handled bellow
    }

    usize elem_trytes = 0;
    switch (dir->kind) {
    case TASM_DIR_TRYTE:   elem_trytes = 1; break;
    case TASM_DIR_QUARTER: elem_trytes = 2; break;
    case TASM_DIR_HALF:    elem_trytes = 4; break;
    case TASM_DIR_WORD:    elem_trytes = 8; break;
    default: return;
    }

    usize write_offset = addr.offset;
    for (TasmOperand* op = dir->operands.begin; op < dir->operands.end; ++op) {
        tc48_word value = 12345; // initialization to make the compiler happy
        if (op->kind == TASM_OPERAND_IMM) {
            value = (tc48_word)op->imm;
        } else if (op->kind == TASM_OPERAND_LABEL) {
            emit_reloc_for_operand(as, op, addr.section_idx, (tc48_half)write_offset, as->current_global_idx, TOBJ_RELOC_ABS);
            if (elem_trytes == 1)      value = RELOC_PLACEHOLDER_6;
            else if (elem_trytes == 2) value = RELOC_PLACEHOLDER_12;
            else if (elem_trytes == 4) value = RELOC_PLACEHOLDER_24;
            else if (elem_trytes == 8) value = RELOC_PLACEHOLDER_48;
        }

        for (usize t = 0; t < elem_trytes; t++) {
            tc48_tryte tryte = value % TC48_TRYTE_VALUES;

            if (VECTOR_SIZE(&sec->data) <= write_offset + t)
                tobj_trytes_push(&sec->data, tryte);
            else
                sec->data.begin[write_offset + t] = tryte;
            value /= TC48_TRYTE_VALUES;
        }
        write_offset += elem_trytes;
    }
}

static void pass3(TasmBackendTObj* as) {
    as->current_global_idx = SENTINEL;

    for (usize i = 0; i < as->asr->count; i++) {
        TasmAsrItem* item = &as->asr->items[i];
        SectionRelAddr addr = as->item_addrs[item->id];

        if (addr.section_idx == SENTINEL) {
            continue;
        }

        if (item->kind == TASM_IR_LABEL) {
            if (!item->as.label.is_local) {
                as->current_global_idx = item->id;
            }
            continue;
        }

        if (item->kind == TASM_IR_INSTR) {
            pass3_handle_instruction(as, item, addr);
        } else if (item->kind == TASM_IR_DIRECTIVE) {
            pass3_handle_directive(as, item, addr);
        }
    }
}

tc48_memory* tasm_assemble_to_tobj(TasmBackendTObj* as) {
    pass1(as);
    if (as->diag->error_count > 0) {
        return NULL;
    }
    pass2(as);
    pass3(as);

    return tobj_bldr_build(&as->bldr, NULL);
}
