#include <tasm/asm/lir.h>

#include <stdbool.h>
#include <stdlib.h>

#define TASM_LIR_INITIAL_CAPACITY 8

static bool tasm_lir_resize(TasmLIR* lir, usize new_capacity) {
    TasmLIRItem* new_items = realloc(lir->items, new_capacity * sizeof(TasmLIRItem));
    if (!new_items) return false;

    lir->items = new_items;
    lir->capacity = new_capacity;
    return true;
}

void tasm_lir_init(TasmLIR* lir) {
    if (!lir) return;

    lir->items = NULL;
    lir->count = 0;
    lir->capacity = 0;
}

void tasm_lir_free(TasmLIR* lir) {
    if (!lir) return;

    if (lir->items) {
        free(lir->items);
    }

    lir->items = NULL;
    lir->count = 0;
    lir->capacity = 0;
}

void tasm_lir_add(TasmLIR* lir, const TasmLIRItem* item) {
    if (!lir || !item) return;

    if (lir->count >= lir->capacity) {
        usize new_capacity = (lir->capacity == 0) ? TASM_LIR_INITIAL_CAPACITY : lir->capacity * 3;

        if (!tasm_lir_resize(lir, new_capacity)) {
            return; // TODO: error handling
        }
    }

    lir->items[lir->count] = *item;
    lir->count++;
}

void tasm_lir_shrink(TasmLIR* lir) {
    if (!lir || lir->capacity == 0) return;

    if (lir->count == 0) {
        free(lir->items);
        lir->items = NULL;
        lir->capacity = 0;
        return;
    }

    if (lir->count < lir->capacity) {
        tasm_lir_resize(lir, lir->count);
    }
}

