#pragma once

#include <tasm/srcdoc/error.h>
#include <tasm/defs/ints.h>

#include <strlib/sb.h>
#include <stdio.h>

typedef struct TasmSourceDocument {
    StringBuf  content;
    StringView filename;
} TasmSourceDocument;

TasmSrcdocResult tasm_srcdoc_init_empty(TasmSourceDocument* srcdoc, StringView filename);
TasmSrcdocResult tasm_srcdoc_init_from_str(TasmSourceDocument* srcdoc, StringView sv, StringView filename);
TasmSrcdocResult tasm_srcdoc_init_from_file(TasmSourceDocument* srcdoc, const char* path);
TasmSrcdocResult tasm_srcdoc_init_from_sb(TasmSourceDocument* srcdoc, const StringBuf* buf, StringView filename);
void             tasm_srcdoc_init_from_sb_move(TasmSourceDocument* srcdoc, StringBuf* buf, StringView filename);

TasmSrcdocResult tasm_srcdoc_copy(const TasmSourceDocument* src, TasmSourceDocument* dst);
void             tasm_srcdoc_move(TasmSourceDocument* src, TasmSourceDocument* dst);

void             tasm_srcdoc_free(TasmSourceDocument* srcdoc);
void             tasm_srcdoc_clear(TasmSourceDocument* srcdoc);

TasmSrcdocResult tasm_srcdoc_append_str(TasmSourceDocument* srcdoc, StringView sv);

StringView       tasm_srcdoc_content(const TasmSourceDocument* srcdoc);
usize            tasm_srcdoc_length(const TasmSourceDocument* srcdoc);
TasmSrcdocResult tasm_srcdoc_print(const TasmSourceDocument* srcdoc, FILE* out);

