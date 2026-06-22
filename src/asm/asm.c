#include <tasm/asm/asm.h>
#include <tasm/asm/lower.h>
#include <tasm/asm/sema.h>
#include <tasm/asm/lower.h>

#include <tc48/cpu/regs.h>
#include <tc48/cpu/instr.h>

#include <tasm/parser/literals.h>

#include <stdlib.h>

void tasm_asm_init(TasmAssembler* as, const TasmAsrBuf* asr, TasmDiagEngine* diag) {
    as->asr = asr;
    as->diag = diag;
    as->item_addresses = calloc(asr->count, sizeof(tc48_word));
    as->symbols = malloc(asr->count * sizeof(TasmSymbol));
    as->symbol_count = 0;
    as->current_scope_id = (usize)-1;
}

void tasm_asm_free(TasmAssembler* as) {
    if (as->item_addresses) free(as->item_addresses);
    if (as->symbols) free(as->symbols);
    as->item_addresses = NULL;
    as->symbols = NULL;
}

static void
    pass1(TasmAssembler*, tc48_word*),
    pass2(TasmAssembler*, TasmIR*);

TasmIR tasm_assemble(TasmAssembler* as) {
    TasmIR ir;
    tasm_ir_init(&ir);

    pass1(as, &ir.size);
    if (as->diag->error_count > 0) {
        return ir;
    }
    pass2(as, &ir);

    return ir;
}

static TasmSymbol* find_symbol(TasmAssembler* as, StringView name, bool is_local, usize scope) {
    for (usize i = 0; i < as->symbol_count; i++) {
        TasmSymbol* s = &as->symbols[i];
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
    TasmAssembler* as = ctx;
    if (op->kind == TASM_OPERAND_IMM) {
        *out = (tc48_word)op->imm;
        return true;
    }
    if (op->kind == TASM_OPERAND_LABEL) {
        TasmSymbol* s = find_symbol(as, op->label.name, op->label.is_local, as->current_scope_id);
        if (!s) {
            tasm_report_error(as->diag, op->span, "undefined symbol: "SV_FMT, SV_FARG(op->label.name));
            return false;
        }
        *out = as->item_addresses[s->id];
        return true;
    }
    return false;
}

static bool lower_directive(TasmAssembler* as, const TasmAsrDir* dir, tc48_word* out) {
    // already handled in pass1()
    if (dir->kind == TASM_DIR_ORG) return false;

    if (dir->value.kind == TASM_OPERAND_IMM) {
        *out = (tc48_word)dir->value.imm;
        return true;
    } else if (dir->value.kind == TASM_OPERAND_LABEL) {
        TasmSymbol* s = find_symbol(as, dir->value.label.name, dir->value.label.is_local, as->current_scope_id);
        if (!s) {
            tasm_report_error(as->diag, dir->value.span, "undefined symbol: "SV_FMT, SV_FARG(dir->value.label.name));
            return false;
        }
        *out = as->item_addresses[s->id];
        return true;
    }
    return false;
}

static void pass1(TasmAssembler* as, tc48_word* out_size) {
    tc48_word current_addr = 0;
    as->current_scope_id = (usize)-1;

    for (usize i = 0; i < as->asr->count; i++) {
        TasmAsrItem* item = &as->asr->items[i];
        as->item_addresses[item->id] = current_addr;

        if (item->kind == TASM_IR_LABEL) {
            bool is_local = item->as.label.is_local;
            if (!is_local) as->current_scope_id = item->id;

            as->symbols[as->symbol_count++] = (TasmSymbol){
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

    *out_size = current_addr;
}

static void pass2(TasmAssembler* as, TasmIR* ir) {
    as->current_scope_id = (usize)-1;

    for (usize i = 0; i < as->asr->count; i++) {
        TasmAsrItem* item = &as->asr->items[i];
        tc48_word addr = as->item_addresses[item->id];

        if (item->kind == TASM_IR_LABEL && !item->as.label.is_local) {
            as->current_scope_id = item->id;
            continue;
        }

        if (item->kind == TASM_IR_INSTR) {
            TasmIRItem ir_item = { .address = addr, .kind = TASM_LIR_INSTR };
            TasmResolver resolver = { .resolve_operand = resolve_operand, .context = as };

            // NOTE: passing NULL here fixes double-diagnostics problem
            if (tasm_lower_instr(NULL, &resolver, &item->as.instr, &ir_item.as.instr)) {
                tasm_ir_add(ir, &ir_item);
            }
        } else if (item->kind == TASM_IR_DIRECTIVE && item->as.directive.kind != TASM_DIR_ORG) {
            if (item->as.directive.kind == TASM_DIR_STRING) {
                usize count = 0;
                tasm_parse_lit_string_chars(as->diag, item->as.directive.value.span, item->as.directive.value.str, NULL, &count);
                int32_t* chars = malloc(count * sizeof(int32_t));
                if (tasm_parse_lit_string_chars(as->diag, item->as.directive.value.span, item->as.directive.value.str, chars, &count)) {
                    for (usize c = 0; c < count; c++) {
                        TasmIRItem ir_item = {
                            .address = addr + c,
                            .kind = TASM_LIR_DATA_TRYTE,
                            .as.data = (tc48_word)chars[c]
                        };
                        tasm_ir_add(ir, &ir_item);
                    }
                }
                free(chars);
            } else {
                TasmIRItem ir_item = { .address = addr };

                switch (item->as.directive.kind) {
                case TASM_DIR_TRYTE:   ir_item.kind = TASM_LIR_DATA_TRYTE;   break;
                case TASM_DIR_QUARTER: ir_item.kind = TASM_LIR_DATA_QUARTER; break;
                case TASM_DIR_HALF:    ir_item.kind = TASM_LIR_DATA_HALF;    break;
                case TASM_DIR_WORD:    ir_item.kind = TASM_LIR_DATA_WORD;    break;
                default: break;
                }

                if (lower_directive(as, &item->as.directive, &ir_item.as.data)) {
                    tasm_ir_add(ir, &ir_item);
                }
            }
        }

    }
}
