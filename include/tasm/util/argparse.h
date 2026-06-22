#include <tasm/util/diag.h>

#include <vector/vector.h>

#include <stdbool.h>

typedef enum TasmArgparseResult {
    TASM_ARGPARSE_OK,
    TASM_ARGPARSE_FAIL,
    TASM_ARGPARSE_SKIP,
} TasmArgparseResult;

typedef enum TasmOutputFormat {
    TASM_FORMAT_RAW,
    TASM_FORMAT_TOBJ,
} TasmOutputFormat;

typedef enum TasmPreference {
    TASM_PREF_NEVER,
    TASM_PREF_AUTO,
    TASM_PREF_ALWAYS,
} TasmPreference;

VECTOR_DECLARE(TasmPaths, tasm_paths, const char*);

typedef struct TasmCmdline {
    TasmPreference   color;
    TasmOutputFormat format;

    TasmPaths input_files;
    const char* output;
} TasmCmdline;

void tasm_print_help(const char* progname);

TasmArgparseResult tasm_parse_args(int argc, const char* argv[], TasmCmdline* out);
