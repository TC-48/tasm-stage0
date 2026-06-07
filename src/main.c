#include <tasm/srcdoc/srcdoc.h>
#include <tasm/lexer/lexer.h>
#include <tasm/irgen/irgen.h>
#include <tasm/ir/dump.h>

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        fputs("usage: tasm src.tasm", stderr);
        return 1;
    }

    TasmSourceDocument source;
    tasm_srcdoc_init_from_file(&source, argv[1]);

    TasmLexer lexer;
    tasm_lexer_init(&lexer, &source);

    //TasmToken tok;
    //while (tasm_lexer_next(&lexer, &tok) == TASM_LEXRES_OK) {
    //    if (tok.type == TT_EOF) break;
    //
    //    tasm_token_print(&tok, stdout);
    //    putchar('\n');
    //}

    TasmDiagEngine diag = {0};

    TasmIRGen irgen;
    tasm_irgen_init(&irgen, &lexer, &diag);

    TasmIR ir = tasm_irgen(&irgen);
    tasm_dump_ir(&ir, stdout);

    tasm_srcdoc_free(&source);
}
