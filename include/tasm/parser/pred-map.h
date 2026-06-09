#pragma once

#include <tasm/defs/ints.h>
#include <tasm/asr/instr.h>
#include <strlib/sv.h>

typedef struct TasmPredMapping {
    StringView name;
    StringView shrt;
    TasmPred   pred;
} TasmPredMapping;

extern TasmPredMapping tasm_pred_map[];
extern const usize tasm_pred_map_size;

bool tasm_parse_pred(StringView m, TasmPred* out);
