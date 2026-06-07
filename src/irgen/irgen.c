#include <tasm/irgen/irgen.h>

#include <tasm/irgen/instr-map.h>
#include <tasm/irgen/pred-map.h>

#include <tc48/cpu/regs.h>
#include <strlib/sv.h>

static inline TasmToken peek(TasmIRGen* irgen) {
    if (!irgen->has_lookahead) {
        tasm_lexer_next(irgen->lexer, &irgen->lookahead);
        irgen->has_lookahead = true;
    }
    return irgen->lookahead;
}

static inline TasmToken advance(TasmIRGen* irgen) {
    TasmToken tok = peek(irgen);
    irgen->has_lookahead = false;
    return tok;
}

static inline bool check(TasmIRGen* irgen, TasmTokenType type) {
    return peek(irgen).type == type;
}

static inline bool match(TasmIRGen* irgen, TasmTokenType type) {
    if (check(irgen, type)) {
        advance(irgen);
        return true;
    }
    return false;
}

static inline TasmToken expect(TasmIRGen* irgen, TasmTokenType type, const char* msg) {
    if (check(irgen, type)) {
        return advance(irgen);
    }

    TasmToken tok = peek(irgen);
    StringView expected_sv = tasm_token_type_to_string(type);
    StringView actual_sv = tasm_token_type_to_string(tok.type);

    tasm_report_error(
        irgen->diag, tok.span, "expected "SV_FMT", got "SV_FMT"%s%s",
        SV_FARG(expected_sv), SV_FARG(actual_sv),
        msg != NULL ? ": " : "", msg != NULL ? msg : ""
    );

    return tok;
}

void tasm_irgen_init(TasmIRGen* irgen, TasmLexer* lexer, TasmDiagEngine* diag) {
    irgen->lexer = lexer;
    irgen->diag = diag;
    irgen->has_lookahead = false;
}

// TODO: stub or something
static bool parse_register(TasmToken tok, tc48_reg_id* out) {
    StringView sv = tok.lexeme;
    if (sv_eql(sv, SV("rip"))) { *out = TC48_WHOLE_REG(TC48_CPU_REG_IP); return true; }
    if (sv_eql(sv, SV("rcf"))) { *out = TC48_WHOLE_REG(TC48_CPU_REG_CF); return true; }
    if (sv_eql(sv, SV("rsp"))) { *out = TC48_WHOLE_REG(TC48_CPU_REG_SP); return true; }
    if (sv_eql(sv, SV("raz"))) { *out = TC48_WHOLE_REG(TC48_CPU_REG_AZ); return true; }

    if (sv.len > 1 && sv.data[0] == 'r' && isdigit(sv.data[1])) {
        // TODO: actual parsing logic
        *out = TC48_WHOLE_REG(TC48_CPU_GPR_BASE); return true;
    }

    return false;
}

TasmIRItem tasm_irgen_item(TasmIRGen* irgen) {
    if (match(irgen, TT_DOT)) {
        TasmToken name  = expect(irgen, TT_IDENT, "expected label name after '.' token");
        TasmToken colon = expect(irgen, TT_COLON, NULL);

        return (TasmIRItem) {
            .kind = TASM_IR_LABEL,
            .as.label = {
                .name = name.lexeme,
                .is_local = true,
            }
        };
    }

    if (check(irgen, TT_IDENT)) {
        TasmToken name = advance(irgen);
        if (check(irgen, TT_COLON)) {
            TasmToken colon = advance(irgen);
            return (TasmIRItem) {
                .kind = TASM_IR_LABEL,
                .as.label = {
                    .name = name.lexeme,
                    .is_local = false,
                }
            };
        }
    }

    tasm_fail(peek(irgen).span, "unimplemented");
}

TasmIR tasm_irgen(TasmIRGen* irgen) {
    TasmIR ir;
    tasm_ir_init(&ir);
    while (true) {
        while (match(irgen, TT_NEWLINE));
        if (check(irgen, TT_EOF)) break;

        TasmIRItem item = tasm_irgen_item(irgen);
        tasm_ir_add(&ir, &item);
    }
    return ir;
}
