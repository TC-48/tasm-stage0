#include <tasm/parser/instr-map.h>

#define MAP(M, O) \
    { .mnemonic = SV(M), .opcode = O }

TasmOpcodeMapping tasm_opcode_map[] = {
    MAP("nop", TASM_OP_NOP),
    MAP("halt", TASM_OP_HALT),

    MAP("min", TASM_OP_MIN),
    MAP("max", TASM_OP_MAX),
    MAP("rot", TASM_OP_ROT),
    MAP("shl", TASM_OP_SHL),
    MAP("shr", TASM_OP_SHR),

    MAP("not", TASM_OP_NOT),
    MAP("neg", TASM_OP_NEG),

    MAP("add", TASM_OP_ADD),
    MAP("sub", TASM_OP_SUB),
    MAP("umul", TASM_OP_UMUL),
    MAP("udiv", TASM_OP_UDIV),
    MAP("smul", TASM_OP_SMUL),
    MAP("sdiv", TASM_OP_SDIV),

    MAP("in", TASM_OP_IN),
    MAP("out", TASM_OP_OUT),
    MAP("load", TASM_OP_LOAD),
    MAP("store", TASM_OP_STORE),

    MAP("din", TASM_OP_DIN),
    MAP("iout", TASM_OP_IOUT),
    MAP("dload", TASM_OP_DLOAD),
    MAP("istore", TASM_OP_ISTORE),

    MAP("set", TASM_OP_SET),
    MAP("inc", TASM_OP_INC),
    MAP("dec", TASM_OP_DEC),
    MAP("cmp", TASM_OP_CMP),
    MAP("jmp", TASM_OP_JMP),

    MAP("push", TASM_OP_PUSH),
    MAP("pop", TASM_OP_POP),
};

const usize tasm_opcode_map_size
    = sizeof(tasm_opcode_map) / sizeof(TasmOpcodeMapping);

bool tasm_parse_mnemonic(StringView m, TasmOpcode* out) {
    for (usize i = 0; i < tasm_opcode_map_size; ++i) {
        if (sv_eql_icase(m, tasm_opcode_map[i].mnemonic)) {
            *out = tasm_opcode_map[i].opcode;
            return true;
        }
    }
    return false;
}
