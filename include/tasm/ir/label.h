#pragma once

#include <tasm/srcdoc/srcspan.h>

#include <strlib/sv.h>
#include <stdbool.h>

typedef struct TasmLabel {
    StringView     name;
    TasmSourceSpan span;
    bool           is_local;
} TasmLabel;
