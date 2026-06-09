#include <tasm/parser/pred-map.h>

#define MAP(N, S, P) \
    (TasmPredMapping) { .name = SV(N), .shrt = SV(S), .pred = P }

TasmPredMapping tasm_pred_map[] = {
    MAP("always", "aw", TC48_PRED_AW),

    // S
    MAP("equal",         "eq", TC48_PRED_EQ),
    MAP("not-equal",     "ne", TC48_PRED_NE),
    MAP("less-than",     "lt", TC48_PRED_LT),
    MAP("greater-than",  "gt", TC48_PRED_GT),
    MAP("less-equal",    "le", TC48_PRED_LE),
    MAP("greater-equal", "ge", TC48_PRED_GE),
    MAP("zero",          "zr", TC48_PRED_ZR),
    MAP("non-zero",      "nz", TC48_PRED_NZ),

    // C
    MAP("carry-set",     "cs", TC48_PRED_CS),
    MAP("carry-clear",   "cc", TC48_PRED_CC),
    MAP("borrow-set",    "bs", TC48_PRED_BS),
    MAP("borrow-clear",  "bc", TC48_PRED_BC),

    // V
    MAP("overflow-set",      "vs", TC48_PRED_VS),
    MAP("overflow-clear",    "vc", TC48_PRED_VC),
    MAP("overflow-positive", "vp", TC48_PRED_VP),
    MAP("overflow-negative", "vn", TC48_PRED_VN),
};

const usize tasm_pred_map_size
    = sizeof(tasm_pred_map) / sizeof(TasmPredMapping);

bool tasm_parse_pred(StringView m, TasmPred* out) {
    for (usize i = 0; i < tasm_pred_map_size; ++i) {
        if (sv_eql_icase(m, tasm_pred_map[i].name) || sv_eql_icase(m, tasm_pred_map[i].shrt)) {
            *out = tasm_pred_map[i].pred;
            return true;
        }
    }
    return false;
}
