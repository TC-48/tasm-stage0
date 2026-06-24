#include <tasm/util/diag.h>
#include <tasm/asr/buf.h>

bool tasm_validate_and_inspect(
    TasmDiagEngine* diag, const TasmInstr* instr,
    enum tc48_instr_format* fmt, enum tc48_operand_width* width
);

tc48_word tasm_get_instr_size(TasmDiagEngine* daig, const TasmInstr* instr);
tc48_word tasm_get_directive_size(TasmDiagEngine* diag, const TasmAsrDir* dir);
bool tasm_get_instr_imm_offset(const TasmInstr* instr, tc48_half* out_off);
