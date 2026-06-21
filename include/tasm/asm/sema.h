#include <tasm/asm/asm.h>

bool tasm_validate_and_inspect(
    TasmAssembler* as, const TasmInstr* instr,
    enum tc48_instr_format* fmt, enum tc48_operand_width* width
);

tc48_word tasm_get_instr_size(TasmAssembler* as, const TasmInstr* instr);
tc48_word tasm_get_directive_size(TasmAssembler* as, const TasmAsrDir* dir);
