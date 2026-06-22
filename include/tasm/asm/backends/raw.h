#pragma once

#include <tasm/util/diag.h>
#include <tasm/asr/buf.h>

#include <tc48/mem.h>

typedef struct TasmRawBeSymbol {
    StringView name;
    usize      id;
    usize      parent_global_id;
    bool       is_local;
} TasmRawBeSymbol;

typedef struct TasmBackendRaw {
    const TasmAsrBuf* asr;
    TasmDiagEngine*  diag;

    usize current_scope_id;

    tc48_word*      item_addresses;
    TasmRawBeSymbol*     symbols;
    usize           symbol_count;
} TasmBackendRaw;

void tasm_raw_asm_init(TasmBackendRaw* as, const TasmAsrBuf* asr, TasmDiagEngine* diag);
void tasm_raw_asm_free(TasmBackendRaw* as);

tc48_memory* tasm_assemble_to_raw(TasmBackendRaw* as);
