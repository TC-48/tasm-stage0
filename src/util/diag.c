#define _POSIX_C_SOURCE 200809L

#include <tasm/util/diag.h>

#include <tasm/srcdoc/srcspan.h>
#include <tasm/srcdoc/srcdoc.h>

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#  include <io.h>
#  define isatty _isatty
#  define fileno _fileno
#else
#  include <unistd.h>
#endif

bool tasm_ansi_enabled = false;

bool tasm_color_auto(FILE* stream) {
    const char* no_color = getenv("NO_COLOR");
    if (no_color && no_color[0] != '\0') {
        return false;
    }
    return isatty(fileno(stream));
}

static void print_span(TasmSourceSpan span) {
    if (tasm_srcspan_is_valid(span)) {
        fprintf(stderr, SV_FMT":%zu:%zu: ", SV_FARG(span.doc->filename), span.start.line, span.start.column);
    } else {
        fprintf(stderr, "<unknown>: ");
    }
}

#define STUFF(FMT)               \
    va_list args;                \
    va_start(args, FMT);         \
    vfprintf(stderr, FMT, args); \
    va_end(args);                \
    fprintf(stderr, "\n");       \

void tasm_print_error(const char* fmt, ...) {
    if (tasm_ansi_enabled) fprintf(stderr, "\033[1;31merror:\033[0m ");
    else                   fprintf(stderr, "error: ");

    STUFF(fmt);
}

void tasm_report_warn(TasmDiagEngine* engine, TasmSourceSpan span, const char* fmt, ...) {
    if (engine == NULL) return;
    print_span(span);

    if (tasm_ansi_enabled) fprintf(stderr, "\033[1;35mwarning:\033[0m ");
    else                   fprintf(stderr, "warning: ");

    STUFF(fmt);
}

void tasm_report_error(TasmDiagEngine* engine, TasmSourceSpan span, const char* fmt, ...) {
    if (engine == NULL) return;
    print_span(span);

    if (tasm_ansi_enabled) fprintf(stderr, "\033[1;31merror:\033[0m ");
    else                   fprintf(stderr, "error: ");

    STUFF(fmt);
}

_Noreturn void tasm_fail(TasmSourceSpan span, const char* fmt, ...) {
    print_span(span);

    if (tasm_ansi_enabled) fprintf(stderr, "\033[1;31mfatal error:\033[0m ");
    else                   fprintf(stderr, "fatal error: ");

    STUFF(fmt);
    exit(1);
}
