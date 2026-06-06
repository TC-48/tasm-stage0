#pragma once

#include <tasm/defs/ints.h>
#include <tasm/ir/defs.h>

typedef struct TasmIR {
    TasmIRItem* items;
    usize count;
    usize capacity;
} TasmIR;

void tasm_ir_init(TasmIR* ir);
void tasm_ir_free(TasmIR* ir);

void tasm_ir_add(TasmIR* ir, const TasmIRItem* item);
void tasm_ir_shrink(TasmIR* ir);
