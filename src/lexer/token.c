#include <tasm/lexer/token.h>

static StringView token_type_to_string_map[] = {
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
};

StringView tasm_token_type_to_string(TasmTokenType tt) {
    if (tt < 0 || tt >= _TT_COUNT) return SV_NULL;
    StringView s = token_type_to_string_map[tt];
    if (sv_is_null(s)) return SV("UNKNOWN");
    return s;
}

usize tasm_token_print(const TasmToken* tok, FILE* out) {
    usize bytes_written = 0;
    StringView type_string = tasm_token_type_to_string(tok->type);

    bytes_written += sv_print(type_string, out);

    if (!sv_is_null(tok->lexeme)) {
        bytes_written += fputc('(', out) == EOF ? 0 : 1;
        bytes_written += sv_print(tok->lexeme, out);
        bytes_written += fputc(')', out) == EOF ? 0 : 1;
    }

    return bytes_written;
}
