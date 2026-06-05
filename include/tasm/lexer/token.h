#pragma once

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

    TT_IMM_INT,  /// 123
    TT_IMM_CHAR, /// 'x'
    TT_IMM_STR,  /// "anything"

    TT_KW_IF, /// if

    TT_KW_OP_NOP,   /// nop
    TT_KW_OP_HALT,  /// halt
    TT_KW_OP_MIN,   /// min
    TT_KW_OP_MAX,   /// max
    TT_KW_OP_ROT,   /// rot
    TT_KW_OP_SHL,   /// shl
    TT_KW_OP_SHR,   /// shr
    TT_KW_OP_NOT,   /// not
    TT_KW_OP_ADD,   /// add
    TT_KW_OP_SUB,   /// sub 
    TT_KW_OP_UMUL,  /// umul
    TT_KW_OP_UDIV,  /// udiv
    TT_KW_OP_SMUL,  /// smul
    TT_KW_OP_SDIV,  /// sdiv
    TT_KW_OP_IN,    /// in
    TT_KW_OP_OUT,   /// out
    TT_KW_OP_LOAD,  /// load
    TT_KW_OP_STORE, /// store
    TT_KW_OP_SET,   /// set
    TT_KW_OP_INC,   /// inc 
    TT_KW_OP_DEC,   /// dec
    TT_KW_OP_CMP,   /// cmp
    TT_KW_OP_JMP,   /// jmp

    _TT_COUNT,
} TasmTokenType;

typedef struct TasmToken {
    TasmTokenType  type;
    StringView lexeme;
} TasmToken;

StringView tasm_token_type_to_string(TasmTokenType tt);
usize tasm_token_print(const TasmToken* tok, FILE* out);
