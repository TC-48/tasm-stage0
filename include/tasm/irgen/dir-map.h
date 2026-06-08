#pragma once

#include <tasm/defs/ints.h>
#include <tasm/ir/defs.h>
#include <strlib/sv.h>

typedef struct TasmDirectiveMapping {
    StringView mnemonic;
    TasmDirectiveKind kind;
} TasmDirectiveMapping;

extern TasmDirectiveMapping tasm_directive_map[];
extern const usize tasm_directive_map_size;

bool tasm_parse_directive_kind(StringView m, TasmDirectiveKind* out);
