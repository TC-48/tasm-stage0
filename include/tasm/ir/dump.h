#pragma once

#include <tasm/ir/ir.h>
#include <stdio.h>

void tasm_dump_ir(const TasmIR* ir, FILE* out);
