#pragma once

#include <tasm/defs/ints.h>
#include <tasm/ir/instr.h>
#include <strlib/sv.h>

typedef struct TasmWidthMapping {
    StringView name;
    tc48_u8b   size;
    StringView size_str;
    TasmWidth  width;
} TasmWidthMapping;

extern TasmWidthMapping tasm_width_map[];
extern const usize tasm_width_map_size;

bool tasm_parse_width(StringView m, TasmWidth* out);
