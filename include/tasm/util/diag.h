#pragma once

#include <tasm/srcdoc/srcspan.h>

extern bool tasm_ansi_enabled;
bool tasm_color_auto(FILE* stream);

typedef struct TasmDiagEngine {
    usize error_count, warn_count;
} TasmDiagEngine;

void tasm_print_error(const char* fmt, ...);

void tasm_report_warn (TasmDiagEngine* engine, TasmSourceSpan span, const char* fmt, ...);
void tasm_report_error(TasmDiagEngine* engine, TasmSourceSpan span, const char* fmt, ...);

_Noreturn void tasm_fail(TasmSourceSpan span, const char* fmt, ...);
