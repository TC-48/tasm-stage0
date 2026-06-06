#pragma once

typedef enum TasmSrcdocResult {
    TASM_SRCDOC_SUCCESS,

    TASM_SRCDOC_ERR_ALLOC,
    TASM_SRCDOC_ERR_FOPEN,
    TASM_SRCDOC_ERR_FTELL,
    TASM_SRCDOC_ERR_FREAD,
    TASM_SRCDOC_ERR_FWRITE,
} TasmSrcdocResult;

