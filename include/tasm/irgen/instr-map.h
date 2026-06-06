#pragma once

#include <tasm/defs/ints.h>
#include <tasm/ir/instr.h>
#include <strlib/sv.h>

typedef struct TasmOpcodeMapping {
    StringView mnemonic;
    TasmOpcode opcode;
} TasmOpcodeMapping;

extern TasmOpcodeMapping tasm_opcode_map[];
extern const usize tasm_opcode_map_size;
