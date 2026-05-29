#include <tasm/lexer/token.h>

StringView token_type_to_string_map[] = {
    [TT_EOF]         = "EOF",
    
    [TT_DOT]         = "DOT",
    [TT_COMMA]       = "COMMA",
    [TT_IDENT]       = "IDENT",
    [TT_COLON]       = "COLON",
    [TT_NEWLINE]     = "NEWLINE",

    [TT_IMM_INT]     = "IMM_INT",
    [TT_IMM_CHAR]    = "IMM_CHAR",
    [TT_IMM_STR]     = "IMM_STR",

    [TT_KW_OP_NOP]   = "KW_OP_NOP",
    [TT_KW_OP_HALT]  = "KW_OP_HALT",
    [TT_KW_OP_MIN]   = "KW_OP_MIN",
    [TT_KW_OP_MAX]   = "KW_OP_MAX",
    [TT_KW_OP_ROT]   = "KW_OP_ROT",
    [TT_KW_OP_SHL]   = "KW_OP_SHL",
    [TT_KW_OP_SHR]   = "KW_OP_SHR",
    [TT_KW_OP_NOT]   = "KW_OP_NOT",
    [TT_KW_OP_ADD]   = "KW_OP_ADD",
    [TT_KW_OP_SUB]   = "KW_OP_SUB",
    [TT_KW_OP_UMUL]  = "KW_OP_UMUL",
    [TT_KW_OP_UDIV]  = "KW_OP_UDIV",
    [TT_KW_OP_SMUL]  = "KW_OP_SMUL",
    [TT_KW_OP_SDIV]  = "KW_OP_SDIV",
    [TT_KW_OP_IN]    = "KW_OP_IN",
    [TT_KW_OP_OUT]   = "KW_OP_OUT",
    [TT_KW_OP_LOAD]  = "KW_OP_LOAD",
    [TT_KW_OP_STORE] = "KW_OP_STORE",
    [TT_KW_OP_SET]   = "KW_OP_SET",
    [TT_KW_OP_INC]   = "KW_OP_INC",
    [TT_KW_OP_DEC]   = "KW_OP_DEC",
    [TT_KW_OP_CMP]   = "KW_OP_CMP",
    [TT_KW_OP_JMP]   = "KW_OP_JMP",
};

StringView token_type_to_string(TokenType tt) {
    if (tt < 0 || tt >= _TT_COUNT) return SV_NULL;
    StringView s = token_type_to_string_map[tt];
    if (sv_is_null(s)) return SV("UNKNOWN");
    return s;
}

usize token_print(const Token* tok, FILE* out) {
    usize bytes_written = 0;
    StringView type_string = token_type_to_string(tok->type);

    bytes_written += sv_print(type_string, out);

    if (!sv_is_null(tok->lexeme)) {
        bytes_written += fputc('(', out) == EOF ? 0 : 1;
        bytes_written += sv_print(tok->lexeme, out);
        bytes_written += fputc(')', out) == EOF ? 0 : 1;
    }

    return bytes_written;
}
