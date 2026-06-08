#pragma once

#include <tasm/defs/ints.h>
#include <tc48/cpu/instr.h>
#include <tc48/word.h>

typedef enum TasmLIRItemKind {
    TASM_LIR_INSTR,
    TASM_LIR_DATA_TRYTE,
    TASM_LIR_DATA_QUARTER,
    TASM_LIR_DATA_HALF,
    TASM_LIR_DATA_WORD,
} TasmLIRItemKind;

typedef struct TasmLIRItem {
    tc48_word       address;
    TasmLIRItemKind kind;
    union {
        tc48_instr instr;
        tc48_word  data;
    } as;
} TasmLIRItem;

typedef struct TasmLIR {
    TasmLIRItem* items;
    tc48_word size;
    usize count;
    usize capacity;
} TasmLIR;

void tasm_lir_init(TasmLIR* lir);
void tasm_lir_free(TasmLIR* lir);
void tasm_lir_add(TasmLIR* lir, const TasmLIRItem* item);
void tasm_lir_shrink(TasmLIR* lir);
