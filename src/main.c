#include <tasm/lexer/lexer.h>

int main() {
    StringView the_source_code = SV(
        "start:\n"
        "    set r1, 100\n"
        ".loop: // a comment btw\n"
        "    dec r1\n"
        "    cmp r1, 0\n"
        "    if(eq) halt\n"
        "    jmp .loop\n"
    );

    TasmLexer lexer;
    tasm_lexer_init(&lexer, the_source_code);

    TasmToken tok;
    while (tasm_lexer_next(&lexer, &tok) == TASM_LEXRES_OK) {
        if (tok.type == TT_EOF) break;

        tasm_token_print(&tok, stdout);
        putchar('\n');
    }
}
