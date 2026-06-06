#pragma once

#include <tasm/ir/instr.h>
#include <strlib/sv.h>

typedef struct TasmPredMapping {
    StringView name;
    StringView shrt;
    TasmPred   pred;
} TasmPredMapping;

extern TasmPredMapping tasm_pred_map[];
