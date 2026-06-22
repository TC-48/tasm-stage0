#pragma once

#include <tasm/util/diag.h>
#include <tasm/asr/buf.h>

#include <tc48/mem.h>

typedef struct TasmBackendTObj {
    const TasmAsrBuf* asr;
    TasmDiagEngine*  diag;

    // TODO: ???
} TasmBackendTObj;

void tasm_tobj_asm_init(TasmBackendTObj* as, const TasmAsrBuf* asr, TasmDiagEngine* diag);
void tasm_tobj_asm_free(TasmBackendTObj* as);

tc48_memory* tasm_assemble_to_tobj(TasmBackendTObj* as);

