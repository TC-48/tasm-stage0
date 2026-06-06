#pragma once

#include <tasm/defs/ints.h>

typedef struct TasmSourceLocation {
    usize line, column;
    usize offset;
} TasmSourceLocation;

#define TASM_SOURCE_LOC_ZERO ((TasmSourceLocation) { 0 })

