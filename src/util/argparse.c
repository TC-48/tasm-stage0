#include <tasm/util/argparse.h>

VECTOR_DEFINE(TasmPaths, tasm_paths, const char*);

void tasm_print_help(const char* progname) {
    printf(
        "Usage: %s [-f|--format raw|tobj] [-o|--output exe.t48b] [--help] file1.tasm [file2.tasm ...]\n",
        progname
    );
}

static inline bool streql(const char* a, const char* b) {
    return strcmp(a, b) == 0;
}

bool tasm_ap_parse_format(const char* arg, TasmOutputFormat* out) {
    if (streql(arg, "raw")) {
        *out = TASM_FORMAT_RAW;
    } else if (streql(arg, "tobj") || streql(arg, "obj")) {
        *out = TASM_FORMAT_TOBJ;
    } else {
        tasm_print_error("invalid format '%s' (expected 'raw' or 'tobj')", arg);
        return false;
    }
    return true;
}

bool tasm_ap_parse_preference(const char* arg, TasmPreference* out) {
    if      (streql(arg, "never"))  *out = TASM_PREF_NEVER;
    else if (streql(arg, "auto"))   *out = TASM_PREF_AUTO;
    else if (streql(arg, "always")) *out = TASM_PREF_ALWAYS;
    else return false;
    return true;
}

#define ADVANCE_ARG(THING) do {                                   \
    if (++i >= argc) {                                            \
        tasm_print_help(argv[0]);                                 \
        tasm_print_error("missing "THING" after '%s' flag", arg); \
        return TASM_ARGPARSE_FAIL;                                \
    }                                                             \
} while (0)

TasmArgparseResult tasm_parse_args(int argc, const char* argv[], TasmCmdline* out) {
    out->format = TASM_FORMAT_RAW;
    out->input_files = (TasmPaths) {0};
    out->output = NULL;

    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];

        if (streql(arg, "--help")) {
            tasm_print_help(argv[0]);
            return TASM_ARGPARSE_SKIP;
        } else if (streql(arg, "-f") || streql(arg, "--format")) {
            ADVANCE_ARG("format");
            if (!tasm_ap_parse_format(argv[i], &out->format)) {
                return TASM_ARGPARSE_FAIL;
            }
        } else if (streql(arg, "-o") || streql(arg, "--output")) {
            ADVANCE_ARG("path");
            out->output = argv[i];
        } else if (streql(arg, "--color")) {
            ADVANCE_ARG("argument");
            if (!tasm_ap_parse_preference(argv[i], &out->color)) {
                return TASM_ARGPARSE_FAIL;
            }
        } else if (arg[0] == '-') {
            tasm_print_help(argv[0]);
            tasm_print_error("unknown option '%s'", arg);
            return TASM_ARGPARSE_FAIL;
        } else {
            tasm_paths_push(&out->input_files, argv[i]);
        }
    }

    if (out->input_files.begin == out->input_files.end) {
        tasm_print_error("no input files specified");
        return TASM_ARGPARSE_FAIL;
    }

    return TASM_ARGPARSE_OK;
}
