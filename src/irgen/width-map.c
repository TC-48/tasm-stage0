#include <tasm/irgen/width-map.h>

#define MAP(S, N, P) \
    (TasmWidthMapping) { .name = SV(N), .size = S, .size_str = SV(#S), .width = P }

TasmWidthMapping tasm_width_map[] = {
    MAP(6,  "tryte",   TASM_WIDTH_6),
    MAP(12, "quarter", TASM_WIDTH_12),
    MAP(24, "half",    TASM_WIDTH_24),
    MAP(48, "word",    TASM_WIDTH_48),
};

const usize tasm_width_map_size = 4;

bool tasm_parse_width(StringView m, TasmWidth* out) {
    for (usize i = 0; i < tasm_width_map_size; ++i) {
        if (sv_eql_icase(m, tasm_width_map[i].name) || sv_eql(m, tasm_width_map[i].size_str)) {
            *out = tasm_width_map[i].width;
            return true;
        }
    }
    return false;
}
