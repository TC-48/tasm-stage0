#include <tasm/lexer/lexer.h>

#include <stdbool.h>
#include <ctype.h>

static inline char peek(TasmLexer* lexer) {
    if (lexer->index >= lexer->source.len) return '\0';
    return lexer->source.data[lexer->index];
}

static inline char next(TasmLexer* lexer) {
    if (lexer->index >= lexer->source.len) return '\0';
    return lexer->source.data[lexer->index++];
}

static inline bool is_at_end(TasmLexer* lexer) {
    return lexer->index >= lexer->source.len;
}

static inline TasmLexerResult ret_tok(TasmToken* out, TasmTokenType tt) {
    *out = (TasmToken) { .type = tt, .lexeme = SV_NULL };
    return TASM_LEXRES_OK;
}
static inline TasmLexerResult ret_tok_with_lexeme(TasmToken* out, TasmTokenType tt, StringView lexeme) {
    *out = (TasmToken) { .type = tt, .lexeme = lexeme };
    return TASM_LEXRES_OK;
}

static inline bool isident(char c) {
    return c == '_' || c == '-' || c == '$';
}

static TasmLexerResult lex_char_lit(TasmLexer* lexer, TasmToken* out_tok) {
    usize start = lexer->index;
    next(lexer); // '

    if (is_at_end(lexer)) return TASM_LEXRES_UNTERM_LITERAL;
    
    if (peek(lexer) == '\\') {
        next(lexer);
        if (is_at_end(lexer)) return TASM_LEXRES_UNTERM_LITERAL;
    }
    next(lexer);

    if (is_at_end(lexer) || peek(lexer) != '\'') return TASM_LEXRES_UNTERM_LITERAL;
    next(lexer); // '

    return ret_tok_with_lexeme(out_tok, TT_IMM_CHAR, sv_slice(lexer->source, start, lexer->index));
}

static TasmLexerResult lex_string_lit(TasmLexer* lexer, TasmToken* out_tok) {
    usize start = lexer->index;
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

    return ret_tok_with_lexeme(out_tok, TT_IMM_STR, sv_slice(lexer->source, start, lexer->index));
}

void tasm_lexer_init(TasmLexer* lexer, StringView source) {
    lexer->source = source;
    lexer->index = 0;
}

TasmLexerResult tasm_lexer_next(TasmLexer* lexer, TasmToken* out_tok) {
    while (!is_at_end(lexer)) {
        char c = peek(lexer);

        if (isspace(c) && c != '\n') {
            next(lexer);
            continue;
        }

        if (c == '/') {
            if (next(lexer) == '/') {
                while (!is_at_end(lexer) && peek(lexer) != '\n') {
                    next(lexer);
                }
                continue;
            } else {
                return TASM_LEXRES_UNEXPECTED_CHAR;
            }
        }

        switch (c) {
        case '.':  next(lexer); return ret_tok(out_tok, TT_DOT);
        case ':':  next(lexer); return ret_tok(out_tok, TT_COLON);
        case ',':  next(lexer); return ret_tok(out_tok, TT_COMMA);
        case '(':  next(lexer); return ret_tok(out_tok, TT_LPAREN);
        case ')':  next(lexer); return ret_tok(out_tok, TT_RPAREN);
        case '\n': next(lexer); return ret_tok(out_tok, TT_NEWLINE);
        case '!':  next(lexer); return ret_tok(out_tok, TT_EXCLAMATION);
        case '?':  next(lexer); return ret_tok(out_tok, TT_QUESTION);

        case '\'': return lex_char_lit(lexer, out_tok);
        case '"':  return lex_string_lit(lexer, out_tok);

        default:
            if (isdigit(c)) {
                usize start = lexer->index;
                while (!is_at_end(lexer) && isdigit(peek(lexer))) {
                    next(lexer);
                }
                return ret_tok_with_lexeme(out_tok, TT_IMM_INT, sv_slice(lexer->source, start, lexer->index));
            }

            if (isalpha(c) || isident(c)) {
                usize start = lexer->index;
                while (!is_at_end(lexer) && (isalnum(peek(lexer)) || isident(peek(lexer))) ) {
                    next(lexer);
                }
                return ret_tok_with_lexeme(out_tok, TT_IDENT, sv_slice(lexer->source, start, lexer->index));
            }

            next(lexer);
            return TASM_LEXRES_UNEXPECTED_CHAR;
        }
    }

    return ret_tok(out_tok, TT_EOF);
}
