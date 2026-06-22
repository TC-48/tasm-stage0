#include <tasm/srcdoc/srcdoc.h>
#include <tasm/lexer/lexer.h>
#include <tasm/parser/parser.h>
#include <tasm/asr/buf.h>

#include <tasm/asm/backends/raw.h>

#include <tasm/asr/dump.h>
#include <stdio.h>

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        fputs("usage: tasm src.tasm [out.t48b]\n", stderr);
        return 1;
    }

    const char* input_path = argv[1];
    const char* output_path = (argc >= 3) ? argv[2] : "out.t48b";

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
        fprintf(stderr, "assembly failed with %d errors\n", (int)diag.error_count);
    }

    tasm_raw_asm_free(&as);
    tasm_asr_free(&asr);
    tasm_srcdoc_free(&source);

    return diag.error_count == 0 ? 0 : 1;
}
