#pragma once

#include <tasm/defs/ints.h>
#include <tasm/asr/defs.h>

typedef struct TasmAsrBuf {
    TasmAsrItem* items;
    usize count;
    usize capacity;
} TasmAsrBuf;

void tasm_asr_init(TasmAsrBuf* asr);
void tasm_asr_free(TasmAsrBuf* asr);

void tasm_asr_add(TasmAsrBuf* asr, const TasmAsrItem* item);
void tasm_asr_shrink(TasmAsrBuf* asr);
