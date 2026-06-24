#include <tasm/parser/dir-map.h>

#define MAP(M, K) \
    { .mnemonic = SV(M), .kind = K }

TasmAsrDirMapping tasm_directive_map[] = {
    MAP("word",    TASM_DIR_WORD),
    MAP("half",    TASM_DIR_HALF),
    MAP("quarter", TASM_DIR_QUARTER),
    MAP("tryte",   TASM_DIR_TRYTE),
    MAP("string",  TASM_DIR_STRING),

    MAP("org",     TASM_DIR_ORG),
    MAP("section", TASM_DIR_SECTION),
    MAP("extern",  TASM_DIR_EXTERN),
    MAP("global",  TASM_DIR_GLOBAL),
    MAP("local",   TASM_DIR_LOCAL),
    MAP("weak",    TASM_DIR_WEAK),
};

const usize tasm_directive_map_size
    = sizeof(tasm_directive_map) / sizeof(TasmAsrDirMapping);

bool tasm_parse_directive_kind(StringView m, TasmAsrDirKind* out) {
    for (usize i = 0; i < tasm_directive_map_size; ++i) {
        if (sv_eql_icase(m, tasm_directive_map[i].mnemonic)) {
            *out = tasm_directive_map[i].kind;
            return true;
        }
    }
    return false;
}
