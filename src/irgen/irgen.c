#include <tasm/irgen/irgen.h>

#include <tasm/irgen/instr-map.h>
#include <tasm/irgen/width-map.h>
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

static inline TasmToken expect2(TasmIRGen* irgen, TasmTokenType type1, TasmTokenType type2, const char* msg) {
    if (check(irgen, type1) || check(irgen, type2)) {
        return advance(irgen);
    }

    TasmToken tok = peek(irgen);
    StringView expected1_sv = tasm_token_type_to_string(type1);
    StringView expected2_sv = tasm_token_type_to_string(type2);
    StringView actual_sv = tasm_token_type_to_string(tok.type);

    tasm_report_error(
        irgen->diag, tok.span, "expected "SV_FMT" or "SV_FMT", got "SV_FMT"%s%s",
        SV_FARG(expected1_sv), SV_FARG(expected2_sv), SV_FARG(actual_sv),
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

TasmIRItem parse_instruction(TasmIRGen* irgen, TasmToken ident) {
    TasmPred pred = TC48_PRED_AW;
    bool had_if = false;
    if (sv_eql(ident.lexeme, SV("if"))) {
        had_if = true;
        expect(irgen, TT_LPAREN, "expected '(' after 'if' token");

        TasmToken pred_tok = expect(irgen, TT_IDENT,  "expected predicate");
        if (!tasm_parse_pred(pred_tok.lexeme, &pred)) {
            tasm_report_error(
                irgen->diag, pred_tok.span,
                "invalid predicate '"SV_FMT"'", SV_FARG(pred_tok.lexeme)
            );
        }

        expect(irgen, TT_RPAREN, "expected ')' after predicate");
    }

    TasmOpcode opcode;
    TasmToken opcode_tok =
        had_if ? expect(irgen, TT_IDENT, "expected instruction mnemonic")
               : ident;

    if (!tasm_parse_mnemonic(opcode_tok.lexeme, &opcode)) {
        tasm_report_error(
            irgen->diag, opcode_tok.span,
            "invalid predicate '"SV_FMT"'", SV_FARG(opcode_tok.lexeme)
        );
    }

    TasmWidth width = TASM_WIDTH_NONE;
    if (match(irgen, TT_DOT)) {
        TasmToken width_tok = expect2(irgen, TT_IMM_INT, TT_IDENT, "expected operand width after '.' token");
        if (!tasm_parse_width(width_tok.lexeme, &width)) {
            tasm_report_error(
                irgen->diag, width_tok.span,
                "invalid operand width '"SV_FMT"'", SV_FARG(width_tok.lexeme)
            );
        }
    }

    TasmWCFR wcfr = TC48_WCFR_NONE;
    if (match(irgen, TT_QUESTION)) {
        wcfr = TC48_WCFR_STAT;
    } else if (match(irgen, TT_EXCLAMATION)) {
        wcfr = TC48_WCFR_FULL;
    }

    // TODO: parsing operands

    return (TasmIRItem) {
        .kind = TASM_IR_INSTR,
        .as.instr = {
            .opcode = opcode, .pred = pred, .width = width, .wcfr = wcfr,
        }
    };
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

        return parse_instruction(irgen, name);
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
