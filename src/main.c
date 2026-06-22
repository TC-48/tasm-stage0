#include <tasm/util/argparse.h>

#include <tasm/srcdoc/srcdoc.h>
#include <tasm/lexer/lexer.h>
#include <tasm/parser/parser.h>

#include <tasm/asr/buf.h>
#include <tasm/util/diag.h>

#include <tasm/asm/backends/raw.h>

#include <tasm/asr/dump.h>

int main(int argc, const char* argv[]) {
    TasmCmdline cmdline;
    TasmArgparseResult result = tasm_parse_args(argc, argv, &cmdline);
    if (result == TASM_ARGPARSE_FAIL) return 2;
    if (result == TASM_ARGPARSE_SKIP) return 0;

    if (VECTOR_SIZE(&cmdline.input_files) != 1) {
        tasm_print_error("mulitiple input files not yet supported");
        return 1;
    }
    if (cmdline.format != TASM_FORMAT_RAW) {
        tasm_print_error("emitting TOBJ file not yet supported");
        return 1;
    }

    const char* input_path  = cmdline.input_files.begin[0];
    const char* output_path = cmdline.output != NULL ? cmdline.output : "out.t48b";

    TasmSourceDocument source;
    tasm_srcdoc_init_from_file(&source, input_path);

    TasmLexer lexer;
    tasm_lexer_init(&lexer, &source);

    TasmDiagEngine diag = {0};

    TasmParser parser;
    tasm_parser_init(&parser, &lexer, &diag);

    TasmAsrBuf asr;
    tasm_asr_init(&asr);
    tasm_parse(&parser, &asr);

    TasmBackendRaw as;
    tasm_raw_asm_init(&as, &asr, &diag);

    tc48_memory* prog = tasm_assemble_to_raw(&as);
    if (diag.error_count == 0) {
        tc48_mem_save(prog, output_path);
    } else {
        tasm_print_error("assembly failed with %u errors", (unsigned)diag.error_count);
    }

    tasm_raw_asm_free(&as);
    tasm_asr_free(&asr);
    tasm_srcdoc_free(&source);

    return diag.error_count == 0 ? 0 : 1;
}
