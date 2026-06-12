#include <tasm/asm/asm.h>

#include <tc48/cpu/regs.h>
#include <tc48/cpu/instr.h>

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

static bool validate_and_inspect(
    TasmAssembler* as, const TasmInstr* instr,
    enum tc48_instr_format* fmt, enum tc48_operand_width* width
) {
    TasmWidth inferred_width = TASM_WIDTH_NONE;

    bool is_addr_op = (instr->opcode == TASM_OP_LOAD || instr->opcode == TASM_OP_STORE ||
                      instr->opcode == TASM_OP_IN || instr->opcode == TASM_OP_OUT);

    for (usize i = 0; i < instr->num_operands; i++) {
        if (instr->operands[i].kind == TASM_OPERAND_REG) {
            if (is_addr_op && i > 0) {
                if (instr->operands[i].reg.width != TASM_WIDTH_48) {
                    tasm_report_error(as->diag, instr->operands[i].span,
                        "memory address register must be 48-bit (e.g., r0, r0:w)");
                    return false;
                }
                continue;
            }

            if (instr->operands[i].reg.id.base == TC48_CPU_REG_AZ) continue;

            TasmWidth op_width = instr->operands[i].reg.width;
            if (inferred_width == TASM_WIDTH_NONE) {
                inferred_width = op_width;
            } else if (inferred_width != op_width) {
                tasm_report_error(as->diag, instr->span, "conflicting operand widths");
                return false;
            }
        }
    }

    if (instr->width == TASM_WIDTH_NONE) {
        if (instr->opcode == TASM_OP_NOP || instr->opcode == TASM_OP_HALT) {
            *width = TC48_OPERAND_WIDTH_6;
        } else if (instr->opcode == TASM_OP_JMP) {
            *width = TC48_OPERAND_WIDTH_48;
        } else if (inferred_width != TASM_WIDTH_NONE) {
            *width = (enum tc48_operand_width)inferred_width;
        } else {
            // If only RAZ registers were found, use their width (usually 48).
            for (usize i = 0; i < instr->num_operands; i++) {
                if (instr->operands[i].kind == TASM_OPERAND_REG &&
                    instr->operands[i].reg.id.base == TC48_CPU_REG_AZ) {
                    inferred_width = instr->operands[i].reg.width;
                    break;
                }
            }
            if (inferred_width != TASM_WIDTH_NONE) {
                *width = (enum tc48_operand_width)inferred_width;
            } else {
                tasm_report_error(as->diag, instr->span, "cannot infer instruction width");
                return false;
            }
        }
    } else {
        *width = (enum tc48_operand_width)instr->width;
        if (inferred_width != TASM_WIDTH_NONE && inferred_width != instr->width) {
            tasm_report_error(as->diag, instr->span, "operand width does not match instruction width");
            return false;
        }
    }

    switch (instr->opcode) {
    case TASM_OP_MIN: case TASM_OP_MAX: case TASM_OP_ROT:
    case TASM_OP_SHR: case TASM_OP_SHL:
    case TASM_OP_ADD: case TASM_OP_SUB:
    case TASM_OP_UMUL: case TASM_OP_SMUL:
    case TASM_OP_UDIV: case TASM_OP_SDIV:
    {
        if (instr->num_operands != 3 && instr->num_operands != 2) {
            tasm_report_error(as->diag, instr->span, "invalid operand count: expected 3 or 2");
            return false;
        }

        const TasmOperand* op1 = (instr->num_operands == 2) ? &instr->operands[0] : &instr->operands[1];
        const TasmOperand* op2 = (instr->num_operands == 2) ? &instr->operands[1] : &instr->operands[2];

        if (instr->operands[0].kind != TASM_OPERAND_REG || op1->kind != TASM_OPERAND_REG) {
            tasm_report_error(as->diag, instr->span, "incorrect operand kind: expected register");
            return false;
        }

        switch (op2->kind) {
        case TASM_OPERAND_REG:
            *fmt = TC48_INSTR_FORMAT_RRR;
            break;
        case TASM_OPERAND_IMM:
            *fmt = TC48_INSTR_FORMAT_RRI;
            break;
        case TASM_OPERAND_LABEL:
            if (*width != TC48_OPERAND_WIDTH_48) {
                tasm_report_error(as->diag, op2->span, "label can be used as imm only in instructions with operand width 48");
                return false;
            }
            *fmt = TC48_INSTR_FORMAT_RRI;
            break;
        default:
            return false;
        }
        return true;
    }

    case TASM_OP_NOT: case TASM_OP_NEG:
    {
        if (instr->num_operands != 2 && instr->num_operands != 1) {
            tasm_report_error(as->diag, instr->span, "invalid operand count: expected 2 or 1");
            return false;
        }

        const TasmOperand* op1 = (instr->num_operands == 1) ? &instr->operands[0] : &instr->operands[1];

        if (instr->operands[0].kind != TASM_OPERAND_REG) {
            tasm_report_error(as->diag, instr->span, "incorrect operand kind: expected register");
            return false;
        }

        switch (op1->kind) {
        case TASM_OPERAND_REG:
            *fmt = TC48_INSTR_FORMAT_RR;
            break;
        case TASM_OPERAND_IMM:
            *fmt = TC48_INSTR_FORMAT_RI;
            break;
        case TASM_OPERAND_LABEL:
            if (*width != TC48_OPERAND_WIDTH_48) {
                tasm_report_error(as->diag, instr->span, "label can be used as imm only in instructions with operand width 48");
                return false;
            }
            *fmt = TC48_INSTR_FORMAT_RI;
            break;
        default:
            return false;
        }

        return true;
    }

    case TASM_OP_INC: case TASM_OP_DEC:
    {
        if (instr->num_operands != 2 && instr->num_operands != 1) {
            tasm_report_error(as->diag, instr->span, "invalid operand count: expected 2 or 1");
            return false;
        }

        if (instr->operands[0].kind != TASM_OPERAND_REG) {
            tasm_report_error(as->diag, instr->operands[0].span, "expected register");
            return false;
        }
        if (instr->num_operands == 2 && instr->operands[1].kind != TASM_OPERAND_REG) {
            tasm_report_error(as->diag, instr->operands[1].span, "expected register for source");
            return false;
        }

        *fmt = TC48_INSTR_FORMAT_RRI;
        return true;
    }


    case TASM_OP_HALT: case TASM_OP_NOP:
        if (instr->num_operands != 0) {
            tasm_report_error(as->diag, instr->span, "invalid operand count: expected 0");
            return false;
        }
        if (instr->width != TASM_WIDTH_NONE) {
            tasm_report_error(as->diag, instr->span, "expected no operand width specified");
            return false;
        }

        *fmt = TC48_INSTR_FORMAT_NONE;
        return true;

    case TASM_OP_LOAD: case TASM_OP_STORE:
    case TASM_OP_IN:   case TASM_OP_OUT:
    {
        if (instr->num_operands < 2 || instr->num_operands > 3) {
            tasm_report_error(as->diag, instr->span, "invalid operand count");
            return false;
        }
        if (instr->operands[0].kind != TASM_OPERAND_REG) {
            tasm_report_error(as->diag, instr->operands[0].span, "expected register as first operand");
            return false;
        }

        bool has_imm = false;
        for (usize i = 1; i < instr->num_operands; i++) {
            if (instr->operands[i].kind != TASM_OPERAND_REG) {
                if (has_imm) {
                    tasm_report_error(as->diag, instr->span, "too many immediate operands");
                    return false;
                }
                has_imm = true;
            }
        }

        if (has_imm) *fmt = TC48_INSTR_FORMAT_RRA;
        else *fmt = TC48_INSTR_FORMAT_RRR;

        return true;
    }

    case TASM_OP_SET:
    case TASM_OP_CMP:
    {
        if (instr->num_operands != 2) {
            tasm_report_error(as->diag, instr->span, "invalid operand count: expected 2");
            return false;
        }
        if (instr->operands[0].kind != TASM_OPERAND_REG) {
            tasm_report_error(as->diag, instr->operands[0].span, "expected register");
            return false;
        }

        const TasmOperand* op1 = &instr->operands[1];
        switch (op1->kind) {
        case TASM_OPERAND_REG:
            *fmt = TC48_INSTR_FORMAT_RRR;
            break;
        case TASM_OPERAND_IMM:
            *fmt = TC48_INSTR_FORMAT_RRI;
            break;
        case TASM_OPERAND_LABEL:
            if (*width != TC48_OPERAND_WIDTH_48) {
                tasm_report_error(as->diag, op1->span, "label can be used as imm only in instructions with operand width 48");
                return false;
            }
            *fmt = TC48_INSTR_FORMAT_RRI;
            break;
        default:
            return false;
        }
        return true;
    }

    case TASM_OP_JMP:
    {
        if (instr->num_operands != 1) {
            tasm_report_error(as->diag, instr->span, "invalid operand count: expected 1");
            return false;
        }
        if (*width != TC48_OPERAND_WIDTH_48) {
            tasm_report_error(as->diag, instr->span, "jmp instruction must have operand width 48");
            return false;
        }

        const TasmOperand* op0 = &instr->operands[0];
        switch (op0->kind) {
        case TASM_OPERAND_REG:
            *fmt = TC48_INSTR_FORMAT_RRR;
            break;
        case TASM_OPERAND_IMM:
        case TASM_OPERAND_LABEL:
            *fmt = TC48_INSTR_FORMAT_RRI;
            break;
        default:
            return false;
        }
        return true;
    }
    }
    return false;
}

static tc48_word get_imm_size(enum tc48_operand_width width) {
    switch (width) {
    case TC48_OPERAND_WIDTH_6:  return 1;
    case TC48_OPERAND_WIDTH_12: return 2;
    case TC48_OPERAND_WIDTH_24: return 4;
    case TC48_OPERAND_WIDTH_48: return 8;
    }
    return 0;
}

#define REG_SIZE_TRYTES    1
#define HEADER_SIZE_TRYTES 2
#define ADDR_SIZE_TRYTES   8

static tc48_word get_instr_size(TasmAssembler* as, const TasmInstr* instr) {
    enum tc48_instr_format fmt;
    enum tc48_operand_width width;
    if (!validate_and_inspect(as, instr, &fmt, &width)) {
        return 0;
    }

    tc48_word size = HEADER_SIZE_TRYTES;

    switch (fmt) {
    case TC48_INSTR_FORMAT_NONE:
        break;
    case TC48_INSTR_FORMAT_R:
        size += REG_SIZE_TRYTES; break;
    case TC48_INSTR_FORMAT_RR:
        size += REG_SIZE_TRYTES * 2; break;
    case TC48_INSTR_FORMAT_RRR:
        size += REG_SIZE_TRYTES * 3; break;
    case TC48_INSTR_FORMAT_RI:
        size += REG_SIZE_TRYTES + get_imm_size(width); break;
    case TC48_INSTR_FORMAT_RRI:
        size += (REG_SIZE_TRYTES * 2) + get_imm_size(width); break;
    case TC48_INSTR_FORMAT_RRA:
        size += (REG_SIZE_TRYTES * 2) + ADDR_SIZE_TRYTES; break;
    }
    return size;
}

static tc48_word get_directive_size(TasmAsrDirKind kind) {
    switch (kind) {
    case TASM_DIR_WORD:    return 8;
    case TASM_DIR_HALF:    return 4;
    case TASM_DIR_QUARTER: return 2;
    case TASM_DIR_TRYTE:   return 1;
    case TASM_DIR_ORG:     return 0;
    }
    return 0;
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
    if (!validate_and_inspect(as, instr, &fmt, &width)) return false;

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
                current_addr += get_directive_size(item->as.directive.kind);
            }
        } else if (item->kind == TASM_IR_INSTR) {
            current_addr += get_instr_size(as, &item->as.instr);
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
