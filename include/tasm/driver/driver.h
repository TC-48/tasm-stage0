#pragma once
#include <tasm/util/argparse.h>

typedef struct TasmDriver {
    const TasmCmdline* cmdline;
    TasmDiagEngine diag;
} TasmDriver;

void tasm_driver_init(TasmDriver* driver, const TasmCmdline* cmdline);
int tasm_driver_run(TasmDriver* driver);
