#pragma once

#include <tasm/asm/asm.h>
#include <stdbool.h>

typedef struct TasmResolver {
    bool (*resolve_operand)(void* context, const TasmOperand* op, tc48_word* out_value);
    void* context;
} TasmResolver;

bool tasm_lower_instr(TasmDiagEngine* diag, const TasmResolver* resolver, const TasmInstr* instr, tc48_instr* out);
