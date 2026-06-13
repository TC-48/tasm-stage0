#include <tasm/lexer/lexer.h>

#include <stdbool.h>
#include <ctype.h>

static inline StringView get_source(const TasmLexer* lexer) {
    return tasm_srcdoc_content(lexer->doc);
}

static inline char peek(TasmLexer* lexer) {
    StringView source = get_source(lexer);
    if (lexer->current_loc.offset >= source.len) return '\0';
    return source.data[lexer->current_loc.offset];
}

static inline char peek_next(TasmLexer* lexer) {
    StringView source = get_source(lexer);
    if (lexer->current_loc.offset + 1 >= source.len) return '\0';
    return source.data[lexer->current_loc.offset + 1];
}

static inline char next(TasmLexer* lexer) {
    StringView source = get_source(lexer);
    if (lexer->current_loc.offset >= source.len) return '\0';
    char c = source.data[lexer->current_loc.offset++];
    if (c == '\n') {
        lexer->current_loc.line++;
        lexer->current_loc.column = 1;
    } else {
        lexer->current_loc.column++;
    }
    return c;
}

static inline bool is_at_end(TasmLexer* lexer) {
    return lexer->current_loc.offset >= get_source(lexer).len;
}

static inline TasmLexerResult ret_tok(TasmLexer* lexer, TasmToken* out, TasmTokenType tt) {
    *out = (TasmToken) {
        .type   = tt,
        .lexeme = SV_NULL,
        .span   = tasm_srcspan_make(lexer->doc, lexer->start_loc, lexer->current_loc)
    };
    return TASM_LEXRES_OK;
}

static inline TasmLexerResult ret_tok_with_lexeme(TasmLexer* lexer, TasmToken* out, TasmTokenType tt) {
    StringView lexeme = sv_slice(get_source(lexer), lexer->start_loc.offset, lexer->current_loc.offset);
    *out = (TasmToken) {
        .type   = tt,
        .lexeme = lexeme,
        .span   = tasm_srcspan_make(lexer->doc, lexer->start_loc, lexer->current_loc)
    };
    return TASM_LEXRES_OK;
}

static inline bool isident(char c) {
    return c == '_' || c == '-' || c == '$';
}

static TasmLexerResult lex_char_lit(TasmLexer* lexer, TasmToken* out_tok) {
    next(lexer); // '

    if (is_at_end(lexer)) return TASM_LEXRES_UNTERM_LITERAL;
    
    if (peek(lexer) == '\\') {
        next(lexer);
        if (is_at_end(lexer)) return TASM_LEXRES_UNTERM_LITERAL;
    }
    next(lexer);

    if (is_at_end(lexer) || peek(lexer) != '\'') return TASM_LEXRES_UNTERM_LITERAL;
    next(lexer); // '

    return ret_tok_with_lexeme(lexer, out_tok, TT_IMM_CHAR);
}

static TasmLexerResult lex_string_lit(TasmLexer* lexer, TasmToken* out_tok) {
    next(lexer); // "

    while (!is_at_end(lexer) && peek(lexer) != '"') {
        if (peek(lexer) == '\\') {
            next(lexer);
            if (is_at_end(lexer)) break;
        }
        next(lexer);
    }

    if (is_at_end(lexer)) return TASM_LEXRES_UNTERM_LITERAL;
    next(lexer); // "

    return ret_tok_with_lexeme(lexer, out_tok, TT_IMM_STR);
}

void tasm_lexer_init(TasmLexer* lexer, const TasmSourceDocument* doc) {
    lexer->doc = doc;
    lexer->start_loc   = (TasmSourceLocation) { .line = 1, .column = 1, .offset = 0 };
    lexer->current_loc = (TasmSourceLocation) { .line = 1, .column = 1, .offset = 0 };
}

TasmLexerResult tasm_lexer_next(TasmLexer* lexer, TasmToken* out_tok) {
    while (!is_at_end(lexer)) {
        lexer->start_loc = lexer->current_loc;
        char c = peek(lexer);

        if (isspace(c) && c != '\n') {
            next(lexer);
            continue;
        }

        if (c == '/') {
            if (peek_next(lexer) == '/') {
                next(lexer); // /
                next(lexer); // /
                while (!is_at_end(lexer) && peek(lexer) != '\n') {
                    next(lexer);
                }
                continue;
            } else {
                next(lexer);
                return TASM_LEXRES_UNEXPECTED_CHAR;
            }
        }

        switch (c) {
        case '.':  next(lexer); return ret_tok(lexer, out_tok, TT_DOT);
        case ':':  next(lexer); return ret_tok(lexer, out_tok, TT_COLON);
        case ',':  next(lexer); return ret_tok(lexer, out_tok, TT_COMMA);
        case '(':  next(lexer); return ret_tok(lexer, out_tok, TT_LPAREN);
        case ')':  next(lexer); return ret_tok(lexer, out_tok, TT_RPAREN);
        case '\n': next(lexer); return ret_tok(lexer, out_tok, TT_NEWLINE);
        case '!':  next(lexer); return ret_tok(lexer, out_tok, TT_EXCLAMATION);
        case '?':  next(lexer); return ret_tok(lexer, out_tok, TT_QUESTION);
        case '%':  next(lexer); return ret_tok(lexer, out_tok, TT_PERCENT);

        case '\'': return lex_char_lit(lexer, out_tok);
        case '"':  return lex_string_lit(lexer, out_tok);

        default:
            if (isdigit(c)) {
                if (c == '0' && !is_at_end(lexer)) {
                    char n = peek_next(lexer);
                    if (n == 't' || n == 'n' || n == 's') {
                        next(lexer); // 0
                        next(lexer); // t, n, s
                        while (!is_at_end(lexer)) {
                            char nc = peek(lexer);
                            if (isalnum(nc) || nc == '_' || nc == '\'') {
                                next(lexer);
                            } else {
                                break;
                            }
                        }
                        return ret_tok_with_lexeme(lexer, out_tok, TT_IMM_INT);
                    }
                }

                while (!is_at_end(lexer)) {
                    char nc = peek(lexer);
                    if (isdigit(nc) || nc == '_' || nc == '\'') {
                        next(lexer);
                    } else {
                        break;
                    }
                }
                return ret_tok_with_lexeme(lexer, out_tok, TT_IMM_INT);
            }

            if (isalpha(c) || isident(c)) {
                while (!is_at_end(lexer) && (isalnum(peek(lexer)) || isident(peek(lexer))) ) {
                    next(lexer);
                }
                return ret_tok_with_lexeme(lexer, out_tok, TT_IDENT);
            }

            next(lexer);
            return TASM_LEXRES_UNEXPECTED_CHAR;
        }
    }

    lexer->start_loc = lexer->current_loc;
    return ret_tok(lexer, out_tok, TT_EOF);
}
