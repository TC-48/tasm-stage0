#include <tasm/asr/tostr.h>
#include <tasm/asr/buf.h>

StringView tasm_opcode_to_str(TasmOpcode op) {
    switch (op) {
    case TASM_OP_NOP:    return SV("nop");
    case TASM_OP_HALT:   return SV("halt");
    case TASM_OP_MIN:    return SV("min");
    case TASM_OP_MAX:    return SV("max");
    case TASM_OP_ROT:    return SV("rot");
    case TASM_OP_SHL:    return SV("shl");
    case TASM_OP_SHR:    return SV("shr");
    case TASM_OP_NOT:    return SV("not");
    case TASM_OP_NEG:    return SV("neg");
    case TASM_OP_ADD:    return SV("add");
    case TASM_OP_SUB:    return SV("sub");
    case TASM_OP_UMUL:   return SV("umul");
    case TASM_OP_UDIV:   return SV("udiv");
    case TASM_OP_SMUL:   return SV("smul");
    case TASM_OP_SDIV:   return SV("sdiv");
    case TASM_OP_IN:     return SV("in");
    case TASM_OP_OUT:    return SV("out");
    case TASM_OP_LOAD:   return SV("load");
    case TASM_OP_STORE:  return SV("store");
    case TASM_OP_DIN:    return SV("din");
    case TASM_OP_IOUT:   return SV("iout");
    case TASM_OP_DLOAD:  return SV("dload");
    case TASM_OP_ISTORE: return SV("istore");
    case TASM_OP_SET:    return SV("set");
    case TASM_OP_INC:    return SV("inc");
    case TASM_OP_DEC:    return SV("dec");
    case TASM_OP_CMP:    return SV("cmp");
    case TASM_OP_JMP:    return SV("jmp");
    case TASM_OP_PUSH:   return SV("push");
    case TASM_OP_POP:    return SV("pop");
    }
    return SV_NULL;
}

StringView tasm_pred_to_str(TasmPred pred) {
    switch (pred) {
    case TC48_PRED_AW: return SV("aw");
    case TC48_PRED_EQ: return SV("eq");
    case TC48_PRED_NE: return SV("ne");
    case TC48_PRED_LT: return SV("lt");
    case TC48_PRED_GT: return SV("gt");
    case TC48_PRED_LE: return SV("le");
    case TC48_PRED_GE: return SV("ge");
    case TC48_PRED_ZR: return SV("zr");
    case TC48_PRED_NZ: return SV("nz");
    case TC48_PRED_CS: return SV("cs");
    case TC48_PRED_CC: return SV("cc");
    case TC48_PRED_BS: return SV("bs");
    case TC48_PRED_BC: return SV("bc");
    case TC48_PRED_VS: return SV("vs");
    case TC48_PRED_VP: return SV("vp");
    case TC48_PRED_VN: return SV("vn");
    case TC48_PRED_VC: return SV("vc");
    }
    return SV_NULL;
}

StringView tasm_width_to_str(TasmWidth w) {
    switch (w) {
    case TASM_WIDTH_NONE: return SV("none");
    case TASM_WIDTH_6:    return SV("6 (tryte)");
    case TASM_WIDTH_12:   return SV("12 (quarter)");
    case TASM_WIDTH_24:   return SV("24 (half)");
    case TASM_WIDTH_48:   return SV("48 (word)");
    }
    return SV_NULL;
}

StringView tasm_wcfr_to_str(TasmWCFR wcfr) {
    switch (wcfr) {
    case TC48_WCFR_NONE: return SV("none");
    case TC48_WCFR_STAT: return SV("stat");
    case TC48_WCFR_FULL: return SV("full");
    }
    return SV_NULL;
}
