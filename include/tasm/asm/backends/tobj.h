#pragma once

#include <tasm/util/diag.h>
#include <tasm/asr/buf.h>

#include <tobj/builder/bldr.h>

#include <tc48/mem.h>

#include <stdbool.h>

typedef struct TasmTObjBeSecRelAddr {
    usize section_idx;
    usize offset;
} TasmTObjBeSecRelAddr;

typedef struct TasmTObjBeSymbol {
    usize name_idx;
    usize builder_sym_idx;
    bool binding_specified;
    tobj_binding binding;
    usize section_idx;
    usize offset;
    usize parent_global_id;
} TasmTObjBeSymbol;

typedef struct TasmTObjBeSec {
    usize idx;
} TasmTObjBeSec;

typedef struct TasmBackendTObj {
    const TasmAsrBuf* asr;
    TasmDiagEngine*  diag;

    tobj_builder bldr;

    TasmTObjBeSymbol* sym_map;
    TasmTObjBeSec*    sec_map;

    TasmTObjBeSecRelAddr* item_addrs;

    usize current_global_idx;
    usize current_section_idx;
} TasmBackendTObj;

void tasm_tobj_asm_init(TasmBackendTObj* as, const TasmAsrBuf* asr, TasmDiagEngine* diag);
void tasm_tobj_asm_free(TasmBackendTObj* as);

tc48_memory* tasm_assemble_to_tobj(TasmBackendTObj* as);

