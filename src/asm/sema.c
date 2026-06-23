#include <tasm/asm/sema.h>
#include <tasm/parser/literals.h>

#include <tc48/cpu/regs.h>
#include <tc48/macros.h>

bool tasm_validate_and_inspect(
    TasmDiagEngine* diag, const TasmInstr* instr,
    enum tc48_instr_format* fmt, enum tc48_operand_width* width
) {
    TasmWidth inferred_width = TASM_WIDTH_NONE;

    bool is_addr_op = (instr->opcode == TASM_OP_LOAD || instr->opcode == TASM_OP_STORE ||
                      instr->opcode == TASM_OP_IN || instr->opcode == TASM_OP_OUT ||
                      instr->opcode == TASM_OP_DIN || instr->opcode == TASM_OP_IOUT ||
                      instr->opcode == TASM_OP_DLOAD || instr->opcode == TASM_OP_ISTORE);

    for (usize i = 0; i < instr->num_operands; i++) {
        if (instr->operands[i].kind == TASM_OPERAND_REG) {
            if (is_addr_op && i > 0) {
                if (instr->operands[i].reg.width != TASM_WIDTH_48) {
                    tasm_report_error(diag, instr->operands[i].span,
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
                tasm_report_error(diag, instr->span, "conflicting operand widths");
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
            for (usize i = 0; i < instr->num_operands; i++) {
                if (instr->operands[i].kind == TASM_OPERAND_REG &&
                    instr->operands[i].reg.id.base == TC48_CPU_REG_AZ) {
                    inferred_width = TASM_WIDTH_6;
                    break;
                }
            }
            if (inferred_width != TASM_WIDTH_NONE) {
                *width = (enum tc48_operand_width)inferred_width;
            } else {
                tasm_report_error(diag, instr->span, "cannot infer instruction width");
                return false;
            }
        }
    } else {
        *width = (enum tc48_operand_width)instr->width;
        if (inferred_width != TASM_WIDTH_NONE && inferred_width != instr->width) {
            tasm_report_error(diag, instr->span, "operand width does not match instruction width");
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
            tasm_report_error(diag, instr->span, "invalid operand count: expected 3 or 2");
            return false;
        }

        const TasmOperand* op1 = (instr->num_operands == 2) ? &instr->operands[0] : &instr->operands[1];
        const TasmOperand* op2 = (instr->num_operands == 2) ? &instr->operands[1] : &instr->operands[2];

        if (instr->operands[0].kind != TASM_OPERAND_REG || op1->kind != TASM_OPERAND_REG) {
            tasm_report_error(diag, instr->span, "incorrect operand kind: expected register");
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
                tasm_report_error(diag, op2->span, "label can be used as imm only in instructions with operand width 48");
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
            tasm_report_error(diag, instr->span, "invalid operand count: expected 2 or 1");
            return false;
        }

        const TasmOperand* op1 = (instr->num_operands == 1) ? &instr->operands[0] : &instr->operands[1];

        if (instr->operands[0].kind != TASM_OPERAND_REG) {
            tasm_report_error(diag, instr->span, "incorrect operand kind: expected register");
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
                tasm_report_error(diag, instr->span, "label can be used as imm only in instructions with operand width 48");
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
            tasm_report_error(diag, instr->span, "invalid operand count: expected 2 or 1");
            return false;
        }

        if (instr->operands[0].kind != TASM_OPERAND_REG) {
            tasm_report_error(diag, instr->operands[0].span, "expected register");
            return false;
        }
        if (instr->num_operands == 2 && instr->operands[1].kind != TASM_OPERAND_REG) {
            tasm_report_error(diag, instr->operands[1].span, "expected register for source");
            return false;
        }

        *fmt = TC48_INSTR_FORMAT_RRI;
        return true;
    }


    case TASM_OP_HALT: case TASM_OP_NOP:
        if (instr->num_operands != 0) {
            tasm_report_error(diag, instr->span, "invalid operand count: expected 0");
            return false;
        }
        if (instr->width != TASM_WIDTH_NONE) {
            tasm_report_error(diag, instr->span, "expected no operand width specified");
            return false;
        }

        *fmt = TC48_INSTR_FORMAT_NONE;
        return true;

    case TASM_OP_LOAD: case TASM_OP_IN:
    {
        if (instr->num_operands < 2 || instr->num_operands > 3) {
            tasm_report_error(diag, instr->span, "invalid operand count");
            return false;
        }
        if (instr->operands[0].kind != TASM_OPERAND_REG) {
            tasm_report_error(diag, instr->operands[0].span, "expected register as first operand");
            return false;
        }

        bool has_imm = false;
        for (usize i = 1; i < instr->num_operands; i++) {
            if (instr->operands[i].kind != TASM_OPERAND_REG) {
                if (has_imm) {
                    tasm_report_error(diag, instr->span, "too many immediate operands");
                    return false;
                }
                has_imm = true;
            }
        }

        if (has_imm) *fmt = TC48_INSTR_FORMAT_RRA;
        else *fmt = TC48_INSTR_FORMAT_RRR;

        return true;
    }

    case TASM_OP_STORE: case TASM_OP_OUT:
    {
        if (instr->num_operands < 2 || instr->num_operands > 3) {
            tasm_report_error(diag, instr->span, "invalid operand count");
            return false;
        }

        if (instr->operands[0].kind == TASM_OPERAND_IMM || instr->operands[0].kind == TASM_OPERAND_LABEL) {
            if (instr->operands[0].kind == TASM_OPERAND_LABEL && *width != TC48_OPERAND_WIDTH_48) {
                tasm_report_error(diag, instr->operands[0].span, "label can be used as imm only in instructions with operand width 48");
                return false;
            }
            for (usize i = 1; i < instr->num_operands; i++) {
                if (instr->operands[i].kind != TASM_OPERAND_REG) {
                    tasm_report_error(diag, instr->operands[i].span, "expected register for address/offset operands in IRR store/out");
                    return false;
                }
            }
            *fmt = TC48_INSTR_FORMAT_IRR;
            return true;
        }

        if (instr->operands[0].kind != TASM_OPERAND_REG) {
            tasm_report_error(diag, instr->operands[0].span, "expected register or immediate/label as first operand");
            return false;
        }

        bool has_imm = false;
        for (usize i = 1; i < instr->num_operands; i++) {
            if (instr->operands[i].kind != TASM_OPERAND_REG) {
                if (has_imm) {
                    tasm_report_error(diag, instr->span, "too many immediate operands");
                    return false;
                }
                has_imm = true;
            }
        }

        if (has_imm) *fmt = TC48_INSTR_FORMAT_RRA;
        else *fmt = TC48_INSTR_FORMAT_RRR;

        return true;
    }

    case TASM_OP_DIN: case TASM_OP_DLOAD:
    {
        if (instr->num_operands != 2) {
            tasm_report_error(diag, instr->span, "invalid operand count: expected 2");
            return false;
        }
        if (instr->operands[0].kind != TASM_OPERAND_REG) {
            tasm_report_error(diag, instr->operands[0].span, "expected register as destination operand");
            return false;
        }
        if (instr->operands[1].kind != TASM_OPERAND_REG) {
            tasm_report_error(diag, instr->operands[1].span, "expected register as pointer operand");
            return false;
        }
        *fmt = TC48_INSTR_FORMAT_RR;
        return true;
    }

    case TASM_OP_IOUT: case TASM_OP_ISTORE:
    {
        if (instr->num_operands != 2) {
            tasm_report_error(diag, instr->span, "invalid operand count: expected 2");
            return false;
        }
        if (instr->operands[1].kind != TASM_OPERAND_REG) {
            tasm_report_error(diag, instr->operands[1].span, "expected register as pointer operand");
            return false;
        }

        const TasmOperand* op0 = &instr->operands[0];
        switch (op0->kind) {
        case TASM_OPERAND_REG:
            *fmt = TC48_INSTR_FORMAT_RR;
            break;
        case TASM_OPERAND_IMM:
            *fmt = TC48_INSTR_FORMAT_IR;
            break;
        case TASM_OPERAND_LABEL:
            if (*width != TC48_OPERAND_WIDTH_48) {
                tasm_report_error(diag, op0->span, "label can be used as imm only in instructions with operand width 48");
                return false;
            }
            *fmt = TC48_INSTR_FORMAT_IR;
            break;
        default:
            return false;
        }
        return true;
    }

    case TASM_OP_PUSH:
    {
        if (instr->num_operands != 1) {
            tasm_report_error(diag, instr->span, "invalid operand count: expected 1");
            return false;
        }

        const TasmOperand* op0 = &instr->operands[0];
        switch (op0->kind) {
        case TASM_OPERAND_REG:
            *fmt = TC48_INSTR_FORMAT_RR;
            break;
        case TASM_OPERAND_IMM:
            *fmt = TC48_INSTR_FORMAT_IR;
            break;
        case TASM_OPERAND_LABEL:
            if (*width != TC48_OPERAND_WIDTH_48) {
                tasm_report_error(diag, op0->span, "label can be used as imm only in instructions with operand width 48");
                return false;
            }
            *fmt = TC48_INSTR_FORMAT_IR;
            break;
        default:
            return false;
        }
        return true;
    }

    case TASM_OP_POP:
    {
        if (instr->num_operands != 1) {
            tasm_report_error(diag, instr->span, "invalid operand count: expected 1");
            return false;
        }
        if (instr->operands[0].kind != TASM_OPERAND_REG) {
            tasm_report_error(diag, instr->operands[0].span, "expected register as destination operand");
            return false;
        }
        *fmt = TC48_INSTR_FORMAT_RR;
        return true;
    }

    case TASM_OP_SET:
    case TASM_OP_CMP:
    {
        if (instr->num_operands != 2) {
            tasm_report_error(diag, instr->span, "invalid operand count: expected 2");
            return false;
        }
        if (instr->operands[0].kind != TASM_OPERAND_REG) {
            tasm_report_error(diag, instr->operands[0].span, "expected register");
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
                tasm_report_error(diag, op1->span, "label can be used as imm only in instructions with operand width 48");
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
            tasm_report_error(diag, instr->span, "invalid operand count: expected 1");
            return false;
        }
        if (*width != TC48_OPERAND_WIDTH_48) {
            tasm_report_error(diag, instr->span, "jmp instruction must have operand width 48");
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

tc48_word tasm_get_instr_size(TasmDiagEngine* diag, const TasmInstr* instr) {
    enum tc48_instr_format fmt;
    enum tc48_operand_width width;
    if (!tasm_validate_and_inspect(diag, instr, &fmt, &width)) {
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
    case TC48_INSTR_FORMAT_IR:
        size += REG_SIZE_TRYTES + get_imm_size(width); break;
    case TC48_INSTR_FORMAT_RRI:
    case TC48_INSTR_FORMAT_IRR:
        size += (REG_SIZE_TRYTES * 2) + get_imm_size(width); break;
    case TC48_INSTR_FORMAT_RRA:
        size += (REG_SIZE_TRYTES * 2) + ADDR_SIZE_TRYTES; break;
    }
    return size;
}

tc48_word tasm_get_directive_size(TasmDiagEngine* diag, const TasmAsrDir* dir) {
    switch (dir->kind) {
    case TASM_DIR_WORD:    return 8 * VECTOR_SIZE(&dir->operands);
    case TASM_DIR_HALF:    return 4 * VECTOR_SIZE(&dir->operands);
    case TASM_DIR_QUARTER: return 2 * VECTOR_SIZE(&dir->operands);
    case TASM_DIR_TRYTE:   return 1 * VECTOR_SIZE(&dir->operands);
    case TASM_DIR_STRING: {
        tc48_word total_count = 0;
        for (TasmOperand* op = dir->operands.begin; op < dir->operands.end; ++op) {
            usize count = 0;
            if (tasm_parse_lit_string_chars(diag, op->span, op->str, NULL, &count)) {
                total_count += count;
            }
        }
        return total_count;
    }

    case TASM_DIR_ORG: case TASM_DIR_SECTION:
    case TASM_DIR_LOCAL: case TASM_DIR_GLOBAL:
    case TASM_DIR_WEAK: case TASM_DIR_EXTERN:
        return 0;
    }
    TC48_UNREACHABLE_ENUM_VAL(TasmAsrDirKind, dir->kind);
}

