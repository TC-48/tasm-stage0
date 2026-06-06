#pragma once

#include <tasm/ir/instr.h>
#include <strlib/sv.h>

typedef struct TasmWidthMapping {
    StringView name;
    tc48_u8b   size;
    StringView size_str;
    TasmWidth  width;
} TasmWidthMapping;

extern TasmWidthMapping tasm_width_map[];

