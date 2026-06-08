#include <tasm/asm/emit.h>
#include <tc48/mem.h>
#include <tc48/cpu/encode.h>

void tasm_emit(const TasmLIR* lir, const char* path) {
    if (lir->count == 0) return;

    tc48_memory* mem = tc48_mem_alloc(lir->size);
    if (!mem) return;

    for (usize i = 0; i < lir->count; i++) {
        TasmLIRItem* item = &lir->items[i];
        switch (item->kind) {
        case TASM_LIR_INSTR:
            tc48_encode(mem, item->address, &item->as.instr);
            break;
        case TASM_LIR_DATA_TRYTE:
            tc48_mem_store6(mem, item->address, (tc48_tryte)item->as.data);
            break;
        case TASM_LIR_DATA_QUARTER:
            tc48_mem_store12(mem, item->address, (tc48_quarter)item->as.data);
            break;
        case TASM_LIR_DATA_HALF:
            tc48_mem_store24(mem, item->address, (tc48_half)item->as.data);
            break;
        case TASM_LIR_DATA_WORD:
            tc48_mem_store48(mem, item->address, (tc48_word)item->as.data);
            break;
        }
    }

    tc48_mem_save(mem, path);
    tc48_mem_free(mem);
}
