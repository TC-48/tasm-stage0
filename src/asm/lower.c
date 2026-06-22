#include <tasm/asm/lower.h>
#include <tasm/asm/sema.h>
#include <tasm/asm/asm.h>

#include <tc48/cpu/regs.h>

#include <stdbool.h>

static void fill_imm(tc48_imm* imm, enum tc48_operand_width width, tc48_word val) {
    switch (width) {
    case TC48_OPERAND_WIDTH_6:  imm->i6  = (tc48_tryte)val;   break;
    case TC48_OPERAND_WIDTH_12: imm->i12 = (tc48_quarter)val; break;
    case TC48_OPERAND_WIDTH_24: imm->i24 = (tc48_half)val;    break;
    case TC48_OPERAND_WIDTH_48: imm->i48 = (tc48_word)val;    break;
    }
}

bool tasm_lower_instr(TasmDiagEngine* diag, const TasmResolver* resolver, const TasmInstr* instr, tc48_instr* out) {
    enum tc48_instr_format fmt;
    enum tc48_operand_width width;
    if (!tasm_validate_and_inspect(diag, instr, &fmt, &width)) {
        return false;
    }

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
        if (!resolver->resolve_operand(resolver->context, op_imm, &imm_val)) return false;
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
        if (!resolver->resolve_operand(resolver->context, op_imm, &imm_val)) return false;
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
            if (!resolver->resolve_operand(resolver->context, op_imm, &imm_val)) {
                return false;
            }
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
