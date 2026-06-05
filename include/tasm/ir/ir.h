#pragma once

#include <tc48/cpu/opcode.h>
#include <tc48/cpu/instr.h>

#include <tasm/defs/sv.h>
#include <tasm/ir/instr.h>

typedef enum TasmDirectiveKind {
    TASM_DIR_WORD,
    TASM_DIR_HALF,
    TASM_DIR_QUARTER,
    TASM_DIR_TRYTE,
    TASM_DIR_ORG,
} TasmDirectiveKind;

typedef struct TasmDirective {
    TasmDirectiveKind kind;
    TasmOperand       value;
} TasmDirective;

typedef enum TasmIRItemKind {
    TASM_IR_LABEL,
    TASM_IR_INSTR,
    TASM_IR_DIRECTIVE,
} TasmIRItemKind;

typedef struct TasmIRItem {
    TasmIRItemKind kind;
    union {
        StringView    label;
        TasmInstr     instr;
        TasmDirective directive;
    };
} TasmIRItem;

typedef struct TasmIR {
    TasmIRItem* items;
    usize count;
    usize capacity;
} TasmIR;
