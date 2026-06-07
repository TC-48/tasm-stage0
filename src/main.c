#include <tasm/srcdoc/srcdoc.h>
#include <tasm/lexer/lexer.h>
#include <tasm/irgen/irgen.h>
#include <tasm/asm/asm.h>
#include <tasm/asm/emit.h>
#include <tasm/ir/dump.h>
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

    TasmIRGen irgen;
    tasm_irgen_init(&irgen, &lexer, &diag);

    TasmIR ir = tasm_irgen(&irgen);

    TasmAssembler as;
    tasm_asm_init(&as, &ir, &diag);

    TasmLIR lir = tasm_assemble(&as);

    if (diag.error_count == 0) {
        tasm_emit(&lir, output_path);
    } else {
        fprintf(stderr, "assembly failed with %d errors\n", (int)diag.error_count);
    }

    tasm_lir_free(&lir);
    tasm_asm_free(&as);
    tasm_ir_free(&ir);
    tasm_srcdoc_free(&source);

    return diag.error_count == 0 ? 0 : 1;
}
