#pragma once

#include <tasm/asr/instr.h>
#include <tasm/defs/ints.h>
#include <tasm/srcdoc/srcspan.h>

#include <tc48/cpu/opcode.h>
#include <tc48/cpu/instr.h>

#include <vector/vector.h>
#include <strlib/sv.h>

VECTOR_DECLARE(TasmOperands, tasm_operands, TasmOperand);

typedef enum TasmAsrDirKind {
    TASM_DIR_WORD,
    TASM_DIR_HALF,
    TASM_DIR_QUARTER,
    TASM_DIR_TRYTE,
    TASM_DIR_STRING,

    TASM_DIR_ORG,
    TASM_DIR_SECTION,
    TASM_DIR_GLOBAL,
    TASM_DIR_LOCAL,
    TASM_DIR_WEAK,
} TasmAsrDirKind;

typedef struct TasmAsrDir {
    TasmAsrDirKind kind;
    TasmSourceSpan span;
    TasmOperands operands;
} TasmAsrDir;

typedef enum TasmAsrItemKind {
    TASM_IR_LABEL,
    TASM_IR_INSTR,
    TASM_IR_DIRECTIVE,
} TasmAsrItemKind;

typedef struct TasmAsrItem {
    usize id;
    TasmAsrItemKind kind;
    TasmSourceSpan span;
    union {
        TasmLabel     label;
        TasmInstr     instr;
        TasmAsrDir directive;
    } as;
} TasmAsrItem;
