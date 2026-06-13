#include <tasm/parser/literals.h>
#include <tasm/gen/tscs.h>
#include <ctype.h>
#include <stdint.h>

static uint32_t decode_utf8(const char* data, usize len, usize* out_bytes) {
    if (len == 0) return 0;
    uint8_t b = (uint8_t)data[0];
    if (b < 0x80) {
        *out_bytes = 1;
        return b;
    }
    if ((b & 0xE0) == 0xC0) {
        if (len < 2) { *out_bytes = len; return 0xFFFD; }
        *out_bytes = 2;
        return ((b & 0x1F) << 6) | (data[1] & 0x3F);
    }
    if ((b & 0xF0) == 0xE0) {
        if (len < 3) { *out_bytes = len; return 0xFFFD; }
        *out_bytes = 3;
        return ((b & 0x0F) << 12) | ((data[1] & 0x3F) << 6) | (data[2] & 0x3F);
    }
    if ((b & 0xF8) == 0xF0) {
        if (len < 4) { *out_bytes = len; return 0xFFFD; }
        *out_bytes = 4;
        return ((b & 0x07) << 18) | ((data[1] & 0x3F) << 12) | ((data[2] & 0x3F) << 6) | (data[3] & 0x3F);
    }
    *out_bytes = 1;
    return 0xFFFD;
}

TasmEscapeResult tasm_parse_escape(TasmDiagEngine* diag, TasmSourceSpan span, const char* data, usize len) {
    if (len < 2 || data[0] != '\\') return (TasmEscapeResult){ -1, 0, false };

    char c = data[1];
    switch (c) {
        case 'n':  return (TasmEscapeResult){ 4, 2, true };  // EOL
        case 'r':  return (TasmEscapeResult){ 5, 2, true };  // CR
        case 't':  return (TasmEscapeResult){ 3, 2, true };  // TAB
        case 'a':  return (TasmEscapeResult){ 1, 2, true };  // BEL
        case 'b':  return (TasmEscapeResult){ 2, 2, true };  // BS
        case 'e':  return (TasmEscapeResult){ 6, 2, true };  // ESC
        case '0':  return (TasmEscapeResult){ 0, 2, true };  // NUL
        case '\\': return (TasmEscapeResult){ tscs_from_ascii['\\'], 2, true };
        case '\'': return (TasmEscapeResult){ tscs_from_ascii['\''], 2, true };
        case '\"': return (TasmEscapeResult){ tscs_from_ascii['\"'], 2, true };

        case 'u': // \u{XXXX} (Unicode hex)
        case 's': // \s{...}  (TSCS index heptavintimal)
        case 'c': // \c{...}  (TSCS code ternary)
        {
            if (len < 4 || data[2] != '{') {
                tasm_report_error(diag, span, "expected '{' after '\\%c'", c);
                return (TasmEscapeResult){ -1, 2, false };
            }
            usize i = 3;
            uint32_t val = 0;
            int base = (c == 'u') ? 16 : (c == 's' ? 27 : 3);
            bool found_digit = false;
            while (i < len && data[i] != '}') {
                int digit = -1;
                char dc = data[i];
                if (isdigit(dc)) digit = dc - '0';
                else if (isalpha(dc)) {
                    if (dc >= 'a' && dc <= 'z') digit = dc - 'a' + 10;
                    else if (dc >= 'A' && dc <= 'Z') digit = dc - 'A' + 10;
                }
                if (digit == -1 || digit >= base) {
                    tasm_report_error(diag, span, "invalid digit '%c' in base %d escape", data[i], base);
                    return (TasmEscapeResult){ -1, i + 1, false };
                }
                val = val * base + digit;
                found_digit = true;
                i++;
            }
            if (i == len) {
                tasm_report_error(diag, span, "unterminated braced escape sequence");
                return (TasmEscapeResult){ -1, i, false };
            }
            if (!found_digit) {
                tasm_report_error(diag, span, "empty braced escape sequence");
                return (TasmEscapeResult){ -1, i + 1, false };
            }

            i++; // consume '}'

            if (c == 'u') {
                int32_t tscs_index = tscs_find_unicode(val);
                if (tscs_index == -1) {
                    tasm_report_error(diag, span, "unicode character U+%04X has no TSCS mapping", val);
                    return (TasmEscapeResult){ -1, i, false };
                }
                return (TasmEscapeResult){ tscs_index, i, true };
            } else {
                if (val >= 444) {
                    tasm_report_error(diag, span, "TSCS index %u out of range (0-443)", val);
                    return (TasmEscapeResult){ -1, i, false };
                }
                return (TasmEscapeResult){ (int32_t)val, i, true };
            }
        }

        default:
            tasm_report_error(diag, span, "unknown escape sequence '\\%c'", c);
            return (TasmEscapeResult){ -1, 2, false };
    }
}

bool tasm_parse_lit_int(StringView lexeme, tc48_i128b* out) {
    int base = 10;
    tc48_usize start = 0;

    if (lexeme.len >= 2 && lexeme.data[0] == '0') {
        if (lexeme.data[1] == 't') {
            base = 3;
            start = 2;
        } else if (lexeme.data[1] == 'n') {
            base = 9;
            start = 2;
        } else if (lexeme.data[1] == 's') {
            base = 27;
            start = 2;
        }
    }

    tc48_i128b val = 0;
    bool has_digits = false;
    for (tc48_usize i = start; i < lexeme.len; ++i) {
        char c = lexeme.data[i];
        if (c == '_' || c == '\'') continue;

        int digit = -1;
        if (isdigit(c)) {
            digit = c - '0';
        } else if (isalpha(c)) {
            if (c >= 'A' && c <= 'Z') {
                digit = c - 'A' + 10;
            } else if (c >= 'a' && c <= 'z') {
                digit = c - 'a' + 10;
            }
        }

        if (digit == -1 || digit >= base) {
            return false;
        }

        val = val * base + digit;
        has_digits = true;
    }

    if (!has_digits) return false;

    *out = val;
    return true;
}

bool tasm_parse_lit_char(TasmDiagEngine* diag, TasmToken tok, tc48_i128b* out) {
    StringView sv = tok.lexeme;
    if (sv.len < 2 || sv.data[0] != '\'' || sv.data[sv.len - 1] != '\'') return false;

    StringView inner = sv_slice(sv, 1, sv.len - 1);
    if (inner.len == 0) {
        tasm_report_error(diag, tok.span, "empty character literal");
        return false;
    }

    int32_t tscs_index = -1;
    if (inner.data[0] == '\\') {
        TasmEscapeResult res = tasm_parse_escape(diag, tok.span, inner.data, inner.len);
        if (!res.success) return false;
        if (res.consumed < inner.len) {
            tasm_report_error(diag, tok.span, "character literal contains multiple characters");
            return false;
        }
        tscs_index = res.tscs_index;
    } else {
        usize bytes = 0;
        uint32_t codepoint = decode_utf8(inner.data, inner.len, &bytes);
        if (codepoint == 0xFFFD) {
            tasm_report_error(diag, tok.span, "invalid UTF-8 sequence");
            return false;
        }
        if (bytes < inner.len) {
            tasm_report_error(diag, tok.span, "character literal contains multiple characters");
            return false;
        }
        tscs_index = tscs_find_unicode(codepoint);
        if (tscs_index == -1) {
            if (codepoint < 32 || (codepoint >= 127 && codepoint < 160)) {
                tasm_report_error(diag, tok.span, "character U+%04X has no TSCS mapping", codepoint);
            } else {
                tasm_report_error(diag, tok.span, "character '"SV_FMT"' (U+%04X) has no TSCS mapping", SV_FARG(inner), codepoint);
            }
            return false;
        }
    }

    *out = (tc48_i128b)tscs_index;
    return true;
}
