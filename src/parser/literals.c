#include <tasm/parser/literals.h>

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
