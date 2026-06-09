#include <tasm/asr/dump.h>
#include <tasm/asr/tostr.h>
#include <tasm/asr/defs.h>
#include <tasm/asr/instr.h>
#include <tasm/asr/label.h>

#include <tasm/util/assert.h>
#include <strlib/sv.h>

#include <inttypes.h>
#include <stdio.h>

static void print_i128(FILE* out, tc48_i128b val) {
    if (val == 0) {
        fprintf(out, "0");
        return;
    }
    if (val < 0) {
        fprintf(out, "-");
        val = -val;
    }
    char buf[45];
    int idx = 0;
    while (val > 0) {
        buf[idx++] = '0' + (int)(val % 10);
        val /= 10;
    }
    for (int i = idx - 1; i >= 0; --i) {
        fputc(buf[i], out);
    }
}

static void tasm_dump_operand(const TasmOperand* op, FILE* out) {
    switch (op->kind) {
        case TASM_OPERAND_REG: {
            fprintf(out, "REG(base: %d, lane: %d)", (int)op->reg.base, (int)op->reg.lane);
            break;
        }
        case TASM_OPERAND_IMM: {
            fprintf(out, "IMM(");
            print_i128(out, op->imm);
            fprintf(out, ")");
            break;
        }
        case TASM_OPERAND_LABEL: {
            fprintf(out, "LABEL_REF(name: " SV_FMT ", is_local: %s)",
                    SV_FARG(op->label.name),
                    op->label.is_local ? "true" : "false");
            break;
        }
    }
}

void tasm_dump_instr(const TasmInstr* instr, FILE* out) {
    StringView op_sv    = tasm_opcode_to_str(instr->opcode);
    StringView pred_sv  = tasm_pred_to_str(instr->pred);
    StringView width_sv = tasm_width_to_str(instr->width);
    StringView wcfr_sv  = tasm_wcfr_to_str(instr->wcfr);

    fprintf(out, "INSTR: opcode="SV_FMT", pred="SV_FMT", wcfr="SV_FMT", width="SV_FMT"\n",
            SV_FARG(op_sv), SV_FARG(pred_sv), SV_FARG(wcfr_sv), SV_FARG(width_sv));

    for (tc48_u8b op_idx = 0; op_idx < instr->num_operands; ++op_idx) {
        fprintf(out, "  operand[%d]: ", op_idx);
        tasm_dump_operand(&instr->operands[op_idx], out);
        fprintf(out, "\n");
    }
}

void tasm_dump_directive(const TasmAsrDir* dir, FILE* out) {
    const char* kind_str = "unknown";
    switch (dir->kind) {
        case TASM_DIR_WORD:    kind_str = "word";    break;
        case TASM_DIR_HALF:    kind_str = "half";    break;
        case TASM_DIR_QUARTER: kind_str = "quarter"; break;
        case TASM_DIR_TRYTE:   kind_str = "tryte";   break;
        case TASM_DIR_ORG:     kind_str = "org";     break;
    }
    fprintf(out, "DIRECTIVE: kind=%s, value=", kind_str);
    tasm_dump_operand(&dir->value, out);
    fprintf(out, "\n");
}

void tasm_asr_dump(const TasmAsrBuf* asr, FILE* out) {
    TASM_ASSERT(asr != NULL && out != NULL, "tasm_asr_dump called with NULL argument");

    for (usize i = 0; i < asr->count; ++i) {
        const TasmAsrItem* item = &asr->items[i];
        switch (item->kind) {
        case TASM_IR_LABEL:
            fprintf(
                out, "LABEL: name="SV_FMT", is_local=%s\n",
                SV_FARG(item->as.label.name),
                item->as.label.is_local ? "true" : "false"
            );
            continue;
        case TASM_IR_DIRECTIVE:
            tasm_dump_directive(&item->as.directive, out);
            continue;
        case TASM_IR_INSTR:
            tasm_dump_instr(&item->as.instr, out);
            continue;
        }

        TASM_UNREACHABLE_ENUM_VAL(TasmAsrItemKind, item->kind);
    }
}
