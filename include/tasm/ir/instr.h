#pragma once

#include <tasm/srcdoc/srcspan.h>
#include <tc48/cpu/opcode.h>
#include <tc48/cpu/instr.h>

#include <tasm/ir/label.h>
#include <strlib/sv.h>

typedef enum TasmWidth {
    TASM_WIDTH_6    = TC48_OPERAND_WIDTH_6,
    TASM_WIDTH_12   = TC48_OPERAND_WIDTH_12,
    TASM_WIDTH_24   = TC48_OPERAND_WIDTH_24,
    TASM_WIDTH_48   = TC48_OPERAND_WIDTH_48,
    TASM_WIDTH_NONE,
} TasmWidth;

typedef enum TasmOperandKind {
    TASM_OPERAND_REG,
    TASM_OPERAND_IMM,
    TASM_OPERAND_LABEL,
} TasmOperandKind;

typedef struct TasmOperand {
    TasmOperandKind kind;
    TasmSourceSpan  span;
    union {
        tc48_reg_id reg;
        tc48_i128b  imm;
        TasmLabel   label;
    };
} TasmOperand;

//  x  - required operand
// [x] - optional operand
// x/y - x or y
typedef enum TasmOpcode {
    TASM_OP_NOP,   // operands: none
    TASM_OP_HALT,  // operands: none

    TASM_OP_MIN,   // operands: reg, [reg], reg/imm
    TASM_OP_MAX,   // operands: reg, [reg], reg/imm
    TASM_OP_ROT,   // operands: reg, [reg], reg/imm
    TASM_OP_SHL,   // operands: reg, [reg], reg/imm
    TASM_OP_SHR,   // operands: reg, [reg], reg/imm

    TASM_OP_NOT,   // operands: reg, [reg/imm]
    TASM_OP_NEG,   // operands: reg, [reg/imm]

    TASM_OP_ADD,   // operands: reg, [reg], reg/imm
    TASM_OP_SUB,   // operands: reg, [reg], reg/imm
    TASM_OP_UMUL,  // operands: reg, [reg], reg/imm
    TASM_OP_UDIV,  // operands: reg, [reg], reg/imm
    TASM_OP_SMUL,  // operands: reg, [reg], reg/imm
    TASM_OP_SDIV,  // operands: reg, [reg], reg/imm

    TASM_OP_IN,    // operands: reg, reg/imm/(reg, imm)
    TASM_OP_OUT,   // operands: reg, reg/imm/(reg, imm)
    TASM_OP_LOAD,  // operands: reg, reg/imm/(reg, imm)
    TASM_OP_STORE, // operands: reg, reg/imm/(reg, imm)

    TASM_OP_SET,   // operands: reg, reg/imm
    TASM_OP_INC,   // operands: reg, [reg/imm]
    TASM_OP_DEC,   // operands: reg, [reg/imm]
    TASM_OP_CMP,   // operands: reg, reg/imm
    TASM_OP_JMP,   // operands: reg/imm
} TasmOpcode;

typedef enum tc48_pred  TasmPred;
typedef tc48_trit_state TasmWCFR;

typedef struct TasmInstr {
    TasmOpcode opcode;
    TasmPred   pred;
    TasmWCFR   wcfr;
    TasmWidth  width;

    TasmOperand operands[3];
    tc48_u8b    num_operands;

    TasmSourceSpan span;
} TasmInstr;
