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
    [TT_EXCLAMATION] = SV("EXCLAMATION"),
    [TT_QUESTION]    = SV("QUESTION"),

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

static inline void print_quoted(StringView sv, FILE* out) {
    fputc('"', out);
    for (const char* c = sv.data; c < sv.data+sv.len; ++c) {
        switch (*c) {
        case '\n': fputs("\\n", out);  break;
        case '\t': fputs("\\t", out);  break;
        case '\r': fputs("\\r", out);  break;
        case '\\': fputs("\\\\", out); break;
        case '"':  fputs("\\\"", out); break;
        default:   fputc(*c, out);     break;
        }
    }
    fputc('"', out);
}

void tasm_token_print(const TasmToken* tok, FILE* out) {
    fprintf(out, "%zu:%zu ", tok->span.start.line, tok->span.start.column);
    StringView type_string = tasm_token_type_to_string(tok->type);
    sv_print(type_string, out);

    if (!sv_is_null(tok->lexeme)) {
        fputc('(', out);
        sv_print(tok->lexeme, out);
        fputc(')', out);
    }

    fputs(" :: ", out);
    print_quoted(tasm_srcspan_to_sv(tok->span), out);
}
