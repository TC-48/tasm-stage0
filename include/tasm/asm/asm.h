#pragma once

#include <tasm/util/diag.h>
#include <tasm/asm/lir.h>
#include <tasm/ir/ir.h>

typedef struct TasmSymbol {
    StringView name;
    usize      id;
    usize      parent_global_id;
    bool       is_local;
} TasmSymbol;

typedef struct TasmAssembler {
    const TasmIR*   ir;
    TasmDiagEngine* diag;

    usize current_addr;
    usize current_scope_id;

    tc48_word*      item_addresses;
    TasmSymbol*     symbols;
    usize           symbol_count;
} TasmAssembler;

void tasm_asm_init(TasmAssembler* as, const TasmIR* ir, TasmDiagEngine* diag);
void tasm_asm_free(TasmAssembler* as);

TasmLIR tasm_assemble(TasmAssembler* as);
