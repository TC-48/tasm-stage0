#include <tasm/asm/emit.h>
#include <tc48/mem.h>
#include <tc48/cpu/encode.h>

void tasm_emit(const TasmLIR* lir, const char* path) {
    if (lir->count == 0) return;

    tc48_word max_addr = 0;
    for (usize i = 0; i < lir->count; i++) {
        tc48_word end_addr = lir->items[i].address;
        switch (lir->items[i].kind) {
            case TASM_LIR_INSTR:        end_addr += 16; break; // safe upper bound
            case TASM_LIR_DATA_TRYTE:   end_addr += 1;  break;
            case TASM_LIR_DATA_QUARTER: end_addr += 2;  break;
            case TASM_LIR_DATA_HALF:    end_addr += 4;  break;
            case TASM_LIR_DATA_WORD:    end_addr += 8;  break;
        }
        if (end_addr > max_addr) max_addr = end_addr;
    }

    tc48_memory* mem = tc48_mem_alloc(max_addr);
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
