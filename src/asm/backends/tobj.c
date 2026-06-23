#include <tasm/asm/backends/tobj.h>

void tasm_tobj_asm_init(TasmBackendTObj* as, const TasmAsrBuf* asr, TasmDiagEngine* diag) {
    as->asr = asr;
    as->diag = diag;
}

void tasm_tobj_asm_free(TasmBackendTObj* as) {
    (void) as;
}

tc48_memory* tasm_assemble_to_tobj(TasmBackendTObj* as) {
    return (void) as, NULL;
}
