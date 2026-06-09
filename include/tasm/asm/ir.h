#pragma once

#include <tasm/defs/ints.h>
#include <tc48/cpu/instr.h>
#include <tc48/word.h>

typedef enum TasmIRItemKind {
    TASM_LIR_INSTR,
    TASM_LIR_DATA_TRYTE,
    TASM_LIR_DATA_QUARTER,
    TASM_LIR_DATA_HALF,
    TASM_LIR_DATA_WORD,
} TasmIRItemKind;

typedef struct TasmIRItem {
    tc48_word       address;
    TasmIRItemKind kind;
    union {
        tc48_instr instr;
        tc48_word  data;
    } as;
} TasmIRItem;

typedef struct TasmIR {
    TasmIRItem* items;
    tc48_word size;
    usize count;
    usize capacity;
} TasmIR;

void tasm_ir_init(TasmIR* ir);
void tasm_ir_free(TasmIR* ir);
void tasm_ir_add(TasmIR* ir, const TasmIRItem* item);
void tasm_ir_shrink(TasmIR* ir);
