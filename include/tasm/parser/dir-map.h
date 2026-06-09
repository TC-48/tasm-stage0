#pragma once

#include <tasm/defs/ints.h>
#include <tasm/asr/defs.h>
#include <strlib/sv.h>

typedef struct TasmAsrDirMapping {
    StringView mnemonic;
    TasmAsrDirKind kind;
} TasmAsrDirMapping;

extern TasmAsrDirMapping tasm_directive_map[];
extern const usize tasm_directive_map_size;

bool tasm_parse_directive_kind(StringView m, TasmAsrDirKind* out);
