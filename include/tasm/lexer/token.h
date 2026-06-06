#pragma once

#include <tasm/srcdoc/srcspan.h>
#include <tasm/defs/ints.h>
#include <strlib/sv.h>
#include <stdio.h>

typedef enum TasmTokenType {
    TT_EOF,
    
    TT_DOT,     /// .
    TT_COMMA,   /// ,
    TT_IDENT,   /// anything 
    TT_COLON,   /// :
    TT_LPAREN,  /// (
    TT_RPAREN,  /// )
    TT_NEWLINE, /// \n
    TT_EXCLAMATION, /// !
    TT_QUESTION,    /// ?

    TT_IMM_INT,  /// 123
    TT_IMM_CHAR, /// 'x'
    TT_IMM_STR,  /// "anything"

    _TT_COUNT,
} TasmTokenType;

typedef struct TasmToken {
    TasmTokenType  type;
    StringView     lexeme;
    TasmSourceSpan span;
} TasmToken;

StringView tasm_token_type_to_string(TasmTokenType tt);
void tasm_token_print(const TasmToken* tok, FILE* out);
