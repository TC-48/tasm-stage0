#pragma once

#include <tasm/ir/instr.h>
#include <strlib/sv.h>

typedef struct TasmOpcodeMapping {
    StringView mnemonic;
    TasmOpcode opcode;
} TasmOpcodeMapping;

extern TasmOpcodeMapping tasm_opcode_map[];
