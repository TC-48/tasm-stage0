#include <tasm/srcdoc/srcdoc.h>
#include <tasm/lexer/lexer.h>

int main(int argc, const char* argv[]) {
    if (argc != 2) {
        fputs("usage: tasm src.tasm", stderr);
        return 1;
    }

    TasmSourceDocument source;
    tasm_srcdoc_init_from_file(&source, argv[1]);

    TasmLexer lexer;
    tasm_lexer_init(&lexer, tasm_srcdoc_content(&source));

    TasmToken tok;
    while (tasm_lexer_next(&lexer, &tok) == TASM_LEXRES_OK) {
        if (tok.type == TT_EOF) break;

        tasm_token_print(&tok, stdout);
        putchar('\n');
    }

    tasm_srcdoc_free(&source);
}
