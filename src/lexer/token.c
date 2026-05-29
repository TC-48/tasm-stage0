#include <tasm/lexer/token.h>

StringView token_type_to_string_map[] = {
    [TT_EOF]         = SV("EOF"),
    
    [TT_DOT]         = SV("DOT"),
    [TT_COMMA]       = SV("COMMA"),
    [TT_IDENT]       = SV("IDENT"),
    [TT_COLON]       = SV("COLON"),
    [TT_LPAREN]      = SV("LPAREN"),
    [TT_RPAREN]      = SV("RPAREN"),
    [TT_NEWLINE]     = SV("NEWLINE"),

    [TT_IMM_INT]     = SV("IMM_INT"),
    [TT_IMM_CHAR]    = SV("IMM_CHAR"),
    [TT_IMM_STR]     = SV("IMM_STR"),

    [TT_KW_IF]       = SV("KW_IF"),

    [TT_KW_OP_NOP]   = SV("KW_OP_NOP"),
    [TT_KW_OP_HALT]  = SV("KW_OP_HALT"),
    [TT_KW_OP_MIN]   = SV("KW_OP_MIN"),
    [TT_KW_OP_MAX]   = SV("KW_OP_MAX"),
    [TT_KW_OP_ROT]   = SV("KW_OP_ROT"),
    [TT_KW_OP_SHL]   = SV("KW_OP_SHL"),
    [TT_KW_OP_SHR]   = SV("KW_OP_SHR"),
    [TT_KW_OP_NOT]   = SV("KW_OP_NOT"),
    [TT_KW_OP_ADD]   = SV("KW_OP_ADD"),
    [TT_KW_OP_SUB]   = SV("KW_OP_SUB"),
    [TT_KW_OP_UMUL]  = SV("KW_OP_UMUL"),
    [TT_KW_OP_UDIV]  = SV("KW_OP_UDIV"),
    [TT_KW_OP_SMUL]  = SV("KW_OP_SMUL"),
    [TT_KW_OP_SDIV]  = SV("KW_OP_SDIV"),
    [TT_KW_OP_IN]    = SV("KW_OP_IN"),
    [TT_KW_OP_OUT]   = SV("KW_OP_OUT"),
    [TT_KW_OP_LOAD]  = SV("KW_OP_LOAD"),
    [TT_KW_OP_STORE] = SV("KW_OP_STORE"),
    [TT_KW_OP_SET]   = SV("KW_OP_SET"),
    [TT_KW_OP_INC]   = SV("KW_OP_INC"),
    [TT_KW_OP_DEC]   = SV("KW_OP_DEC"),
    [TT_KW_OP_CMP]   = SV("KW_OP_CMP"),
    [TT_KW_OP_JMP]   = SV("KW_OP_JMP"),
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
