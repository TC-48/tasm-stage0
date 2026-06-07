#pragma once

#include <tasm/lexer/lexer.h>
#include <tasm/util/diag.h>
#include <tasm/ir/ir.h>

typedef struct TasmIRGen {
    TasmDiagEngine* diag;

    // maybe we shouldn't store the lexer instance directly here
    // and create some kind of TokenStream interface
    // but this is not java, so maybe we don't need to
    TasmLexer* lexer;

    bool  has_lookahead;
    TasmToken lookahead;
} TasmIRGen;

void tasm_irgen_init(TasmIRGen* irgen, TasmLexer* lexer, TasmDiagEngine* diag);
TasmIR tasm_irgen(TasmIRGen* irgen);
