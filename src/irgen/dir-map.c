#include <tasm/irgen/dir-map.h>

#define MAP(M, K) \
    (TasmDirectiveMapping) { .mnemonic = SV(M), .kind = K }

TasmDirectiveMapping tasm_directive_map[] = {
    MAP("word",    TASM_DIR_WORD),
    MAP("half",    TASM_DIR_HALF),
    MAP("quarter", TASM_DIR_QUARTER),
    MAP("tryte",   TASM_DIR_TRYTE),
    MAP("org",     TASM_DIR_ORG),
};

const usize tasm_directive_map_size
    = sizeof(tasm_directive_map) / sizeof(TasmDirectiveMapping);

bool tasm_parse_directive_kind(StringView m, TasmDirectiveKind* out) {
    for (usize i = 0; i < tasm_directive_map_size; ++i) {
        if (sv_eql_icase(m, tasm_directive_map[i].mnemonic)) {
            *out = tasm_directive_map[i].kind;
            return true;
        }
    }
    return false;
}
