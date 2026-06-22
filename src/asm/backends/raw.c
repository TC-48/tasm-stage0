#include <tasm/asm/backends/raw.h>

#include <tasm/asm/lower.h>
#include <tasm/asm/sema.h>

#include <tc48/cpu/regs.h>
#include <tc48/cpu/instr.h>
#include <tc48/cpu/encode.h>

#include <tasm/parser/literals.h>

#include <stdlib.h>

void tasm_raw_asm_init(TasmBackendRaw* as, const TasmAsrBuf* asr, TasmDiagEngine* diag) {
    as->asr = asr;
    as->diag = diag;
    as->item_addresses = calloc(asr->count, sizeof(tc48_word));
    as->symbols = malloc(asr->count * sizeof(TasmRawBeSymbol));
    as->symbol_count = 0;
    as->current_scope_id = (usize)-1;
}

void tasm_raw_asm_free(TasmBackendRaw* as) {
    if (as->item_addresses) free(as->item_addresses);
    if (as->symbols) free(as->symbols);
    as->item_addresses = NULL;
    as->symbols = NULL;
}

static void
    pass1(TasmBackendRaw*, tc48_word*),
    pass2(TasmBackendRaw*, tc48_memory*);

tc48_memory* tasm_assemble_to_raw(TasmBackendRaw* as) {
    tc48_word size;
    pass1(as, &size);
    if (as->diag->error_count > 0) {
        return NULL;
    }


    tc48_memory* mem = tc48_mem_alloc(size);
    pass2(as, mem);

    return mem;
}

static TasmRawBeSymbol* find_symbol(TasmBackendRaw* as, StringView name, bool is_local, usize scope) {
    for (usize i = 0; i < as->symbol_count; i++) {
        TasmRawBeSymbol* s = &as->symbols[i];
        if (!sv_eql(s->name, name)) continue;

        if (is_local) {
            if (s->is_local && s->parent_global_id == scope) return s;
        } else {
            if (!s->is_local) return s;
        }
    }
    return NULL;
}

static bool resolve_operand(void* ctx, const TasmOperand* op, tc48_word* out) {
    TasmBackendRaw* as = ctx;
    if (op->kind == TASM_OPERAND_IMM) {
        *out = (tc48_word)op->imm;
        return true;
    }
    if (op->kind == TASM_OPERAND_LABEL) {
        TasmRawBeSymbol* s = find_symbol(as, op->label.name, op->label.is_local, as->current_scope_id);
        if (!s) {
            tasm_report_error(as->diag, op->span, "undefined symbol: "SV_FMT, SV_FARG(op->label.name));
            return false;
        }
        *out = as->item_addresses[s->id];
        return true;
    }
    return false;
}


static bool lower_string_directive(TasmBackendRaw* as, tc48_memory* mem, const TasmAsrDir* dir, tc48_word addr) {
    usize count = 0;
    if (!tasm_parse_lit_string_chars(as->diag, dir->value.span, dir->value.str, NULL, &count)) {
        return false;
    }

    int32_t* chars = malloc(count * sizeof(int32_t));
    if (chars == NULL) return false;

    if (tasm_parse_lit_string_chars(as->diag, dir->value.span, dir->value.str, chars, &count)) {
        for (usize c = 0; c < count; c++) {
            tc48_mem_store6(mem, addr + c, chars[c]);
        }
    }
    free(chars);

    return true;
}

static bool lower_directive(TasmBackendRaw* as, const TasmAsrDir* dir, tc48_memory* mem, tc48_word addr) {
    // already handled in pass1()
    if (dir->kind == TASM_DIR_ORG) return false;

    if (dir->kind == TASM_DIR_STRING) {
        return lower_string_directive(as, mem, dir, addr);
    }

    tc48_word value;
    if (!resolve_operand(as, &dir->value, &value)) {
        return false;
    }

    switch (dir->kind) {
    case TASM_DIR_TRYTE:   tc48_mem_store6 (mem, addr, (tc48_tryte)value);   return true;
    case TASM_DIR_QUARTER: tc48_mem_store12(mem, addr, (tc48_quarter)value); return true;
    case TASM_DIR_HALF:    tc48_mem_store24(mem, addr, (tc48_half)value);    return true;
    case TASM_DIR_WORD:    tc48_mem_store48(mem, addr, (tc48_word)value);    return true;
    default: return false; // probably unreachable
    }
}

static void pass1(TasmBackendRaw* as, tc48_word* out_size) {
    tc48_word current_addr = 0;
    as->current_scope_id = (usize)-1;

    for (usize i = 0; i < as->asr->count; i++) {
        TasmAsrItem* item = &as->asr->items[i];
        as->item_addresses[item->id] = current_addr;

        if (item->kind == TASM_IR_LABEL) {
            bool is_local = item->as.label.is_local;
            if (!is_local) as->current_scope_id = item->id;

            as->symbols[as->symbol_count++] = (TasmRawBeSymbol){
                .name = item->as.label.name,
                .id = item->id,
                .parent_global_id = is_local ? as->current_scope_id : (usize)-1,
                .is_local = is_local
            };
        } else if (item->kind == TASM_IR_DIRECTIVE) {
            if (item->as.directive.kind == TASM_DIR_ORG) {
                current_addr = item->as.directive.value.imm;
            } else {
                current_addr += tasm_get_directive_size(as->diag, &item->as.directive);
            }
        } else if (item->kind == TASM_IR_INSTR) {
            current_addr += tasm_get_instr_size(as->diag, &item->as.instr);
        }
    }

    if (out_size != NULL) {
        *out_size = current_addr;
    }
}

static void pass2_handle_instruction(TasmBackendRaw* as, tc48_memory* mem, TasmAsrItem* item, tc48_word addr) {
    TasmResolver resolver = { .resolve_operand = resolve_operand, .context = as };

    // NOTE: passing NULL here fixes double-diagnostics problem
    tc48_instr instr;
    if (tasm_lower_instr(NULL, &resolver, &item->as.instr, &instr)) {
        tc48_encode(mem, addr, &instr);
    }
}

static void pass2(TasmBackendRaw* as, tc48_memory* mem) {
    as->current_scope_id = (usize)-1;

    for (usize i = 0; i < as->asr->count; i++) {
        TasmAsrItem* item = &as->asr->items[i];
        tc48_word addr = as->item_addresses[item->id];

        if (item->kind == TASM_IR_LABEL) {
            if (!item->as.label.is_local) {
                as->current_scope_id = item->id;
            }
            continue;
        }

        if (item->kind == TASM_IR_INSTR) {
            pass2_handle_instruction(as, mem, item, addr);
        } else if (item->kind == TASM_IR_DIRECTIVE) {
            if (item->as.directive.kind == TASM_DIR_ORG) continue;
            (void) lower_directive(as, &item->as.directive, mem, addr);
        }
    }
}
