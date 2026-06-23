#include <tasm/asr/buf.h>

#include <stdlib.h>

// i just didn't know where to put this
// but it doesn't matter anyway
VECTOR_DEFINE(TasmOperands, tasm_operands, TasmOperand);

#define TASM_IR_INITIAL_CAPACITY 8

static bool tasm_asr_resize(TasmAsrBuf* asr, usize new_capacity) {
    TasmAsrItem* new_items = realloc(asr->items, new_capacity * sizeof(TasmAsrItem));
    if (!new_items) return false;

    asr->items = new_items;
    asr->capacity = new_capacity;
    return true;
}

void tasm_asr_init(TasmAsrBuf* asr) {
    asr->items = NULL;
    asr->count = 0;
    asr->capacity = 0;
}

void tasm_asr_free(TasmAsrBuf* asr) {
    if (asr->items != NULL) {
        for (usize i = 0; i < asr->count; i++)
            if (asr->items[i].kind == TASM_IR_DIRECTIVE)
                tasm_operands_free(&asr->items[i].as.directive.operands);
        free(asr->items);
    }

    asr->items = NULL;
    asr->count = 0;
    asr->capacity = 0;
}

void tasm_asr_add(TasmAsrBuf* asr, const TasmAsrItem* item) {
    if (asr->count >= asr->capacity) {
        usize new_capacity = (asr->capacity == 0) ? TASM_IR_INITIAL_CAPACITY : asr->capacity * 3;

        if (!tasm_asr_resize(asr, new_capacity)) {
            return; // TODO: error handling
        }
    }

    asr->items[asr->count] = *item;
    asr->items[asr->count].id = asr->count;
    asr->count++;
}

void tasm_asr_shrink(TasmAsrBuf* asr) {
    if (asr->capacity == 0) return;

    if (asr->count == 0) {
        free(asr->items);
        asr->items = NULL;
        asr->capacity = 0;
        return;
    }

    if (asr->count < asr->capacity) {
        tasm_asr_resize(asr, asr->count);
    }
}
