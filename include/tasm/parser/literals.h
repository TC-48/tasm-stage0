#include <strlib/sv.h>
#include <tc48/defs.h>

#include <tasm/lexer/token.h>
#include <tasm/util/diag.h>

typedef struct TasmEscapeResult {
    int32_t tscs_index;
    usize   consumed;
    bool    success;
} TasmEscapeResult;

TasmEscapeResult tasm_parse_escape(TasmDiagEngine* diag, TasmSourceSpan span, const char* data, usize len);

bool tasm_parse_lit_int(StringView lexeme, tc48_i128b* out);
bool tasm_parse_lit_char(TasmDiagEngine* diag, TasmToken tok, tc48_i128b* out);
bool tasm_parse_lit_string_chars(TasmDiagEngine* diag, TasmSourceSpan span, StringView sv, int32_t* out_chars, usize* out_count);
