#pragma once

#include <strlib/sv.h>
#include <stdbool.h>

typedef struct TasmLabel {
    StringView name;
    bool       is_local;
} TasmLabel;
