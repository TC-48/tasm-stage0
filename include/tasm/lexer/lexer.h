#pragma once

#include <tasm/lexer/token.h>

typedef enum TasmLexerResult {
    TASM_LEXRES_OK,
    TASM_LEXRES_UNEXPECTED_CHAR,
    TASM_LEXRES_UNTERM_LITERAL,
} TasmLexerResult;

typedef struct TasmLexer {
    StringView source;
    usize index;
} TasmLexer;

void            tasm_lexer_init(TasmLexer* lexer, StringView source);
TasmLexerResult tasm_lexer_next(TasmLexer* lexer, Token* out_tok);
