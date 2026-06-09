#pragma once

#include <tasm/util/diag.h>
#include <tasm/asm/ir.h>
#include <tasm/asr/buf.h>

typedef struct TasmSymbol {
    StringView name;
    usize      id;
    usize      parent_global_id;
    bool       is_local;
} TasmSymbol;

typedef struct TasmAssembler {
    const TasmAsrBuf* asr;
    TasmDiagEngine*  diag;

    usize current_scope_id;

    tc48_word*      item_addresses;
    TasmSymbol*     symbols;
    usize           symbol_count;
} TasmAssembler;

void tasm_asm_init(TasmAssembler* as, const TasmAsrBuf* asr, TasmDiagEngine* diag);
void tasm_asm_free(TasmAssembler* as);

TasmIR tasm_assemble(TasmAssembler* as);
