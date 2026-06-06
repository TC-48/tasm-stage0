#include <tasm/ir/ir.h>

#include <stdlib.h>

#define TASM_IR_INITIAL_CAPACITY 8

static bool tasm_ir_resize(TasmIR* ir, usize new_capacity) {
    TasmIRItem* new_items = realloc(ir->items, new_capacity * sizeof(TasmIRItem));
    if (!new_items) return false;

    ir->items = new_items;
    ir->capacity = new_capacity;
    return true;
}

void tasm_ir_init(TasmIR* ir) {
    if (!ir) return;

    ir->items = NULL;
    ir->count = 0;
    ir->capacity = 0;
}

void tasm_ir_free(TasmIR* ir) {
    if (!ir) return;

    if (ir->items) {
        free(ir->items);
    }

    ir->items = NULL;
    ir->count = 0;
    ir->capacity = 0;
}

void tasm_ir_add(TasmIR* ir, const TasmIRItem* item) {
    if (!ir || !item) return;

    if (ir->count >= ir->capacity) {
        usize new_capacity = (ir->capacity == 0) ? TASM_IR_INITIAL_CAPACITY : ir->capacity * 3;

        if (!tasm_ir_resize(ir, new_capacity)) {
            return; // TODO: error handling
        }
    }

    ir->items[ir->count] = *item;
    ir->count++;
}

void tasm_ir_shrink(TasmIR* ir) {
    if (!ir || ir->capacity == 0) return;

    if (ir->count == 0) {
        free(ir->items);
        ir->items = NULL;
        ir->capacity = 0;
        return;
    }

    if (ir->count < ir->capacity) {
        tasm_ir_resize(ir, ir->count);
    }
}
