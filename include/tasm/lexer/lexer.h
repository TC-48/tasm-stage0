#pragma once

#include <tasm/srcdoc/srcdoc.h>
#include <tasm/lexer/token.h>
#include <tasm/defs/ints.h>

typedef enum TasmLexerResult {
    TASM_LEXRES_OK,
    TASM_LEXRES_UNEXPECTED_CHAR,
    TASM_LEXRES_UNTERM_LITERAL,
} TasmLexerResult;

typedef struct TasmLexer {
    const TasmSourceDocument* doc;
    TasmSourceLocation start_loc;
    TasmSourceLocation current_loc;
} TasmLexer;

void            tasm_lexer_init(TasmLexer* lexer, const TasmSourceDocument* doc);
TasmLexerResult tasm_lexer_next(TasmLexer* lexer, TasmToken* out_tok);
