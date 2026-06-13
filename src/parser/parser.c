#include <tasm/parser/parser.h>

#include <tasm/parser/literals.h>

#include <tasm/parser/instr-map.h>
#include <tasm/parser/width-map.h>
#include <tasm/parser/pred-map.h>
#include <tasm/parser/dir-map.h>

#include <tasm/asr/buf.h>

#include <tc48/cpu/regs.h>
#include <strlib/sv.h>

static void fill_lookahead(TasmParser* parser) {
    while (parser->lookahead_count < TASM_PARSER_LOOKAHEAD_SIZE) {
        tasm_lexer_next(parser->lexer, &parser->lookahead[parser->lookahead_count++]);
    }
}

static inline TasmToken peek_at(TasmParser* parser, usize n) {
    fill_lookahead(parser);
    if (n >= parser->lookahead_count) return parser->lookahead[parser->lookahead_count - 1];
    return parser->lookahead[n];
}

static inline TasmToken peek(TasmParser* parser) {
    return peek_at(parser, 0);
}

static inline TasmToken advance(TasmParser* parser) {
    TasmToken tok = peek(parser);
    for (usize i = 0; i < parser->lookahead_count - 1; ++i) {
        parser->lookahead[i] = parser->lookahead[i + 1];
    }
    parser->lookahead_count--;
    return tok;
}

static inline bool check(TasmParser* parser, TasmTokenType type) {
    return peek(parser).type == type;
}

static inline bool match(TasmParser* parser, TasmTokenType type) {
    if (check(parser, type)) {
        advance(parser);
        return true;
    }
    return false;
}

static inline TasmToken expect(TasmParser* parser, TasmTokenType type, const char* msg) {
    if (check(parser, type)) {
        return advance(parser);
    }

    TasmToken tok = peek(parser);
    StringView expected_sv = tasm_token_type_to_string(type);
    StringView actual_sv = tasm_token_type_to_string(tok.type);

    tasm_report_error(
        parser->diag, tok.span, "expected "SV_FMT", got "SV_FMT"%s%s",
        SV_FARG(expected_sv), SV_FARG(actual_sv),
        msg != NULL ? ": " : "", msg != NULL ? msg : ""
    );

    return tok;
}

void tasm_parser_init(TasmParser* parser, TasmLexer* lexer, TasmDiagEngine* diag) {
    parser->lexer = lexer;
    parser->diag = diag;
    parser->lookahead_count = 0;
}

// TODO: sublanes
static bool parse_register(TasmParser* parser, TasmToken tok, TasmRegister* out, TasmSourceSpan* out_span) {
    tc48_reg_id reg_id;
    StringView sv = tok.lexeme;

    bool found_base = true;
    if      (sv_eql_icase(sv, SV("rip"))) { reg_id = TC48_WHOLE_REG(TC48_CPU_REG_IP); }
    else if (sv_eql_icase(sv, SV("rcf"))) { reg_id = TC48_WHOLE_REG(TC48_CPU_REG_CF); }
    else if (sv_eql_icase(sv, SV("rsp"))) { reg_id = TC48_WHOLE_REG(TC48_CPU_REG_SP); }
    else if (sv_eql_icase(sv, SV("raz"))) { reg_id = TC48_WHOLE_REG(TC48_CPU_REG_AZ); }
    else if (sv.len > 1 && (sv.data[0] == 'r' || sv.data[0] == 'R') && isdigit(sv.data[1])) {
        int num = 0;
        usize i = 1;
        while (i < sv.len && isdigit(sv.data[i])) {
            num = num * 10 + (sv.data[i] - '0');
            i++;
        }
        if (i == sv.len && num >= 0 && num < TC48_CPU_GPR_COUNT) {
            reg_id = TC48_WHOLE_REG(TC48_CPU_GPR_BASE + num);
        } else {
            found_base = false;
        }
    } else {
        found_base = false;
    }

    if (!found_base) return false;

    out->id = reg_id;
    out->width = TASM_WIDTH_48;
    *out_span = tok.span;

    if (match(parser, TT_COLON)) {
        TasmToken suffix_tok = expect(parser, TT_IDENT, "expected register width suffix");
        *out_span = tasm_srcspan_merge(*out_span, suffix_tok.span);
        StringView ssv = suffix_tok.lexeme;
        if (ssv.len == 0) return true;

        char width_char = tolower(ssv.data[0]);
        int lane = -1;
        if (ssv.len > 1 && isdigit(ssv.data[1])) {
            lane = ssv.data[1] - '0';
        }

        switch (width_char) {
        case 'w':
            out->width = TASM_WIDTH_48;
            if (ssv.len > 1) {
                tasm_report_error(parser->diag, suffix_tok.span, "width 'w' does not support lane index");
            }
            break;
        case 'h':
            out->width = TASM_WIDTH_24;
            if (ssv.len != 2 || lane < 0 || lane > 1) {
                tasm_report_error(parser->diag, suffix_tok.span, "width 'h' requires a lane index 0-1");
            } else {
                out->id.lane = (tc48_doublet)lane;
            }
            break;
        case 'q':
            out->width = TASM_WIDTH_12;
            if (ssv.len != 2 || lane < 0 || lane > 3) {
                tasm_report_error(parser->diag, suffix_tok.span, "width 'q' requires a lane index 0-3");
            } else {
                out->id.lane = (tc48_doublet)lane;
            }
            break;
        case 't':
            out->width = TASM_WIDTH_6;
            if (ssv.len != 2 || lane < 0 || lane > 7) {
                tasm_report_error(parser->diag, suffix_tok.span, "width 't' requires a lane index 0-7");
            } else {
                out->id.lane = (tc48_doublet)lane;
            }
            break;
        default:
            tasm_report_error(parser->diag, suffix_tok.span, "invalid register width suffix '"SV_FMT"'", SV_FARG(ssv));
            break;
        }
    }

    return true;
}

static TasmOperand parse_operand(TasmParser* parser) {
    if (check(parser, TT_DOT)) {
        TasmToken dot  = advance(parser);
        TasmToken name = expect(parser, TT_IDENT, "expected label name after '.' token");
        return (TasmOperand) {
            .kind = TASM_OPERAND_LABEL,
            .span = tasm_srcspan_merge(dot.span, name.span),
            .label = { .name = name.lexeme, .span = name.span, .is_local = true }
        };
    }

    TasmToken tok = advance(parser);
    if (tok.type == TT_IMM_INT) {
        tc48_i128b imm = 0;
        if (!tasm_parse_lit_int(tok.lexeme, &imm)) {
            tasm_report_error(parser->diag, tok.span, "invalid integer literal '"SV_FMT"'", SV_FARG(tok.lexeme));
        }
        return (TasmOperand) {
            .kind = TASM_OPERAND_IMM,
            .span = tok.span,
            .imm = imm
        };
    }

    if (tok.type == TT_IMM_CHAR) {
        tc48_i128b imm = 0;
        tasm_parse_lit_char(parser->diag, tok, &imm);
        return (TasmOperand) {
            .kind = TASM_OPERAND_IMM,
            .span = tok.span,
            .imm = imm,
        };
    }

    if (tok.type == TT_IDENT) {
        TasmRegister reg;
        TasmSourceSpan reg_span = tok.span;
        if (parse_register(parser, tok, &reg, &reg_span)) {
            return (TasmOperand) {
                .kind = TASM_OPERAND_REG,
                .span = reg_span,
                .reg = reg
            };
        }
        return (TasmOperand) {
            .kind = TASM_OPERAND_LABEL,
            .span = tok.span,
            .label = { .name = tok.lexeme, .span = tok.span, .is_local = false }
        };
    }

    tasm_report_error(parser->diag, tok.span, "expected operand");
    return (TasmOperand) { .span = tok.span };
}

TasmAsrItem parse_instruction(TasmParser* parser, TasmToken ident) {
    TasmSourceSpan span = ident.span;
    TasmPred pred = TC48_PRED_AW;
    bool had_if = false;

    if (sv_eql(ident.lexeme, SV("if"))) {
        had_if = true;
        expect(parser, TT_LPAREN, "expected '(' after 'if' token");

        TasmToken pred_tok = expect(parser, TT_IDENT,  "expected predicate");
        if (!tasm_parse_pred(pred_tok.lexeme, &pred)) {
            tasm_report_error(
                parser->diag, pred_tok.span,
                "invalid predicate '"SV_FMT"'", SV_FARG(pred_tok.lexeme)
            );
        }

        TasmToken rparen = expect(parser, TT_RPAREN, "expected ')' after predicate");
        span = tasm_srcspan_merge(span, rparen.span);
    }

    TasmOpcode opcode;
    TasmToken opcode_tok =
        had_if ? expect(parser, TT_IDENT, "expected instruction mnemonic")
               : ident;
    span = tasm_srcspan_merge(span, opcode_tok.span);

    if (!tasm_parse_mnemonic(opcode_tok.lexeme, &opcode)) {
        tasm_report_error(
            parser->diag, opcode_tok.span,
            "invalid mnemonic '"SV_FMT"'", SV_FARG(opcode_tok.lexeme)
        );
    }

    TasmWidth width = TASM_WIDTH_NONE;
    if (check(parser, TT_DOT) && peek_at(parser, 1).type == TT_IMM_INT) {
        advance(parser); // consume '.'
        TasmToken width_tok = advance(parser); // consume integer
        span = tasm_srcspan_merge(span, width_tok.span);
        if (!tasm_parse_width(width_tok.lexeme, &width)) {
            tasm_report_error(
                parser->diag, width_tok.span,
                "invalid operand width '"SV_FMT"'", SV_FARG(width_tok.lexeme)
            );
        }
    }

    TasmWCFR wcfr = TC48_WCFR_NONE;
    if (check(parser, TT_QUESTION)) {
        TasmToken tok = advance(parser);
        span = tasm_srcspan_merge(span, tok.span);
        wcfr = TC48_WCFR_STAT;
    } else if (check(parser, TT_EXCLAMATION)) {
        TasmToken tok = advance(parser);
        span = tasm_srcspan_merge(span, tok.span);
        wcfr = TC48_WCFR_FULL;
    }

    TasmInstr instr = {
        .opcode = opcode, .pred = pred,
        .width = width, .wcfr = wcfr,
        .num_operands = 0, .operands = {},
        .span = span,
    };

    if (!check(parser, TT_NEWLINE) && !check(parser, TT_EOF)) {
        TasmOperand op = parse_operand(parser);
        instr.operands[instr.num_operands++] = op;
        span = tasm_srcspan_merge(span, op.span);

        while (match(parser, TT_COMMA)) {
            if (instr.num_operands >= 3) {
                tasm_report_error(parser->diag, peek(parser).span, "too many operands");
                parse_operand(parser);
                continue;
            }
            TasmOperand op = parse_operand(parser);
            instr.operands[instr.num_operands++] = op;
            span = tasm_srcspan_merge(span, op.span);
        }
    }

    instr.span = span;

    return (TasmAsrItem) {
        .kind = TASM_IR_INSTR,
        .span = span,
        .as.instr = instr,
    };
}

static TasmAsrItem parse_directive(TasmParser* parser) {
    TasmToken percent = advance(parser);
    TasmToken name = expect(parser, TT_IDENT, "expected directive name after '%' token");

    TasmAsrDirKind kind = TASM_DIR_WORD;
    if (!tasm_parse_directive_kind(name.lexeme, &kind)) {
        tasm_report_error(
            parser->diag, name.span,
            "invalid directive '"SV_FMT"'", SV_FARG(name.lexeme)
        );
    }

    TasmOperand value = parse_operand(parser);
    TasmSourceSpan span = tasm_srcspan_merge(percent.span, value.span);

    return (TasmAsrItem) {
        .kind = TASM_IR_DIRECTIVE,
        .span = span,
        .as.directive = {
            .kind = kind,
            .span = span,
            .value = value,
        }
    };
}

TasmAsrItem tasm_parser_item(TasmParser* parser) {
    if (check(parser, TT_PERCENT)) {
        return parse_directive(parser);
    }

    if (check(parser, TT_DOT)) {
        TasmToken dot   = advance(parser);
        TasmToken name  = expect(parser, TT_IDENT, "expected label name after '.' token");
        TasmToken colon = expect(parser, TT_COLON, NULL);

        TasmSourceSpan span = tasm_srcspan_merge(dot.span, colon.span);

        return (TasmAsrItem) {
            .kind = TASM_IR_LABEL,
            .span = span,
            .as.label = {
                .name = name.lexeme,
                .span = name.span,
                .is_local = true,
            }
        };
    }

    if (check(parser, TT_IDENT)) {
        TasmToken name = advance(parser);
        if (check(parser, TT_COLON)) {
            TasmToken colon = advance(parser);
            return (TasmAsrItem) {
                .kind = TASM_IR_LABEL,
                .span = tasm_srcspan_merge(name.span, colon.span),
                .as.label = {
                    .name = name.lexeme,
                    .span = name.span,
                    .is_local = false,
                }
            };
        }

        return parse_instruction(parser, name);
    }

    tasm_fail(peek(parser).span, "unimplemented");
}

void tasm_parse(TasmParser* parser, TasmAsrBuf* buf) {
    while (true) {
        while (match(parser, TT_NEWLINE));
        if (check(parser, TT_EOF)) break;

        TasmAsrItem item = tasm_parser_item(parser);
        tasm_asr_add(buf, &item);
    }
}
