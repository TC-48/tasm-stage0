#include <tasm/asm/asm.h>
#include <tasm/asm/sema.h>

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

static bool resolve_imm(TasmAssembler* as, const TasmOperand* op, tc48_word* out) {
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

static void fill_imm(tc48_imm* imm, enum tc48_operand_width width, tc48_word val) {
    switch (width) {
    case TC48_OPERAND_WIDTH_6:  imm->i6  = (tc48_tryte)val;   break;
    case TC48_OPERAND_WIDTH_12: imm->i12 = (tc48_quarter)val; break;
    case TC48_OPERAND_WIDTH_24: imm->i24 = (tc48_half)val;    break;
    case TC48_OPERAND_WIDTH_48: imm->i48 = (tc48_word)val;    break;
    }
}

static bool lower_instr(TasmAssembler* as, const TasmInstr* instr, tc48_instr* out) {
    enum tc48_instr_format fmt;
    enum tc48_operand_width width;
    if (!tasm_validate_and_inspect(as, instr, &fmt, &width)) return false;

    out->format = (tc48_doublet)fmt;
    out->width = (tc48_doublet)width;
    out->wcfr = instr->wcfr;
    out->pred = (tc48_triplet)instr->pred;

    tc48_reg_id r1 = TC48_WHOLE_REG(TC48_CPU_REG_AZ);
    tc48_reg_id r2 = TC48_WHOLE_REG(TC48_CPU_REG_AZ);
    tc48_reg_id r3 = TC48_WHOLE_REG(TC48_CPU_REG_AZ);
    const TasmOperand* op_imm = NULL;

    switch (instr->opcode) {
    case TASM_OP_NOP:  out->opcode = TC48_OP_NOP; return true;
    case TASM_OP_HALT: out->opcode = TC48_OP_HALT; return true;

    case TASM_OP_MIN: out->opcode = TC48_OP_MIN; goto op_3;
    case TASM_OP_MAX: out->opcode = TC48_OP_MAX; goto op_3;
    case TASM_OP_ROT: out->opcode = TC48_OP_ROT; goto op_3;
    case TASM_OP_SHL: out->opcode = TC48_OP_SHL; goto op_3;
    case TASM_OP_SHR: out->opcode = TC48_OP_SHR; goto op_3;
    case TASM_OP_ADD: out->opcode = TC48_OP_ADD; goto op_3;
    case TASM_OP_SUB: out->opcode = TC48_OP_SUB; goto op_3;
    case TASM_OP_UMUL: out->opcode = TC48_OP_UMUL; goto op_3;
    case TASM_OP_UDIV: out->opcode = TC48_OP_UDIV; goto op_3;
    case TASM_OP_SMUL: out->opcode = TC48_OP_SMUL; goto op_3;
    case TASM_OP_SDIV: out->opcode = TC48_OP_SDIV; goto op_3;

    case TASM_OP_NOT: out->opcode = TC48_OP_NOT; goto op_2;
    case TASM_OP_NEG: out->opcode = TC48_OP_NEG; goto op_2;

    case TASM_OP_LOAD:  out->opcode = TC48_OP_LOAD;  goto op_mem;
    case TASM_OP_STORE: out->opcode = TC48_OP_STORE; goto op_mem;
    case TASM_OP_IN:    out->opcode = TC48_OP_IN;    goto op_mem;
    case TASM_OP_OUT:   out->opcode = TC48_OP_OUT;   goto op_mem;

    case TASM_OP_INC: out->opcode = TC48_OP_ADD; goto op_inc_dec;
    case TASM_OP_DEC: out->opcode = TC48_OP_SUB; goto op_inc_dec;

    case TASM_OP_SET:
        out->opcode = TC48_OP_ADD;
        r1 = instr->operands[0].reg.id;
        r2 = TC48_WHOLE_REG(TC48_CPU_REG_AZ);
        op_imm = &instr->operands[1];
        goto op_3_lower;

    case TASM_OP_CMP:
        out->opcode = TC48_OP_SUB;
        out->wcfr = TC48_WCFR_FULL;
        r1 = TC48_WHOLE_REG(TC48_CPU_REG_AZ);
        r2 = instr->operands[0].reg.id;
        op_imm = &instr->operands[1];
        goto op_3_lower;

    case TASM_OP_JMP:
        out->opcode = TC48_OP_ADD;
        r1 = TC48_WHOLE_REG(TC48_CPU_REG_IP);
        r2 = TC48_WHOLE_REG(TC48_CPU_REG_AZ);
        op_imm = &instr->operands[0];
        goto op_3_lower;
    }

    return false;

op_3:
    r1 = instr->operands[0].reg.id;
    r2 = (instr->num_operands == 3) ? instr->operands[1].reg.id : r1;
    op_imm = &instr->operands[instr->num_operands - 1];
    goto op_3_lower;

op_3_lower:
    if (fmt == TC48_INSTR_FORMAT_RRR) {
        out->operands.rrr.r1 = r1;
        out->operands.rrr.r2 = r2;
        out->operands.rrr.r3 = op_imm->reg.id;
    } else {
        out->operands.rri.r1 = r1;
        out->operands.rri.r2 = r2;
        tc48_word imm_val;
        if (!resolve_imm(as, op_imm, &imm_val)) return false;
        fill_imm(&out->operands.rri.imm, width, imm_val);
    }
    return true;

op_2:
    r1 = instr->operands[0].reg.id;
    op_imm = &instr->operands[instr->num_operands - 1];

    if (fmt == TC48_INSTR_FORMAT_RR) {
        out->operands.rr.r1 = r1;
        out->operands.rr.r2 = op_imm->reg.id;
    } else {
        out->operands.ri.r1 = r1;
        tc48_word imm_val;
        if (!resolve_imm(as, op_imm, &imm_val)) return false;
        fill_imm(&out->operands.ri.imm, width, imm_val);
    }
    return true;

op_mem:
    r1 = instr->operands[0].reg.id;
    r2 = TC48_WHOLE_REG(TC48_CPU_REG_AZ);
    r3 = TC48_WHOLE_REG(TC48_CPU_REG_AZ);

    if (instr->num_operands == 2) {
        if (instr->operands[1].kind == TASM_OPERAND_REG) {
            r2 = instr->operands[1].reg.id;
        } else {
            op_imm = &instr->operands[1];
        }
    } else if (instr->num_operands == 3) {
        if (instr->operands[1].kind == TASM_OPERAND_REG) {
            r2 = instr->operands[1].reg.id;
            if (instr->operands[2].kind == TASM_OPERAND_REG) {
                r3 = instr->operands[2].reg.id;
            } else {
                op_imm = &instr->operands[2];
            }
        } else {
            op_imm = &instr->operands[1];
            r2 = instr->operands[2].reg.id;
        }
    }

    if (fmt == TC48_INSTR_FORMAT_RRR) {
        out->operands.rrr.r1 = r1;
        out->operands.rrr.r2 = r2;
        out->operands.rrr.r3 = r3;
    } else {
        out->operands.rra.r1 = r1;
        out->operands.rra.r2 = r2;
        tc48_word imm_val = 0;
        if (op_imm != NULL) {
            if (!resolve_imm(as, op_imm, &imm_val)) return false;
        }
        out->operands.rra.addr = imm_val;
    }
    return true;

op_inc_dec:
    r1 = instr->operands[0].reg.id;
    r2 = (instr->num_operands == 2) ? instr->operands[1].reg.id : r1;
    out->operands.rri.r1 = r1;
    out->operands.rri.r2 = r2;
    fill_imm(&out->operands.rri.imm, width, 1);
    return true;
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
                current_addr += tasm_get_directive_size(as, &item->as.directive);
            }
        } else if (item->kind == TASM_IR_INSTR) {
            current_addr += tasm_get_instr_size(as, &item->as.instr);
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
            if (lower_instr(as, &item->as.instr, &ir_item.as.instr)) {
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
