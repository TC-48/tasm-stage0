#pragma once

#include <tasm/lexer/lexer.h>
#include <tasm/util/diag.h>

#include <tasm/asr/defs.h>
#include <tasm/asr/buf.h>

#define TASM_PARSER_LOOKAHEAD_SIZE 2

typedef struct TasmParser {
    TasmDiagEngine* diag;

    // maybe we shouldn't store the lexer instance directly here
    // and create some kind of TokenStream interface
    // but this is not java, so maybe we don't need to
    TasmLexer* lexer;

    TasmToken lookahead[TASM_PARSER_LOOKAHEAD_SIZE];
    usize     lookahead_count;

    usize     current_id;
} TasmParser;

void tasm_parser_init(TasmParser* parser, TasmLexer* lexer, TasmDiagEngine* diag);
void tasm_parse(TasmParser* parser, TasmAsrBuf* buf);
