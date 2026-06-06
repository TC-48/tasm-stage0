#pragma once

#include <tasm/defs/source-loc.h>
#include <strlib/sv.h>

typedef struct TasmSourceDocument TasmSourceDocument;

typedef struct TasmSourceSpan {
    const TasmSourceDocument* doc;
    TasmSourceLocation        start;
    TasmSourceLocation        end;
} TasmSourceSpan;

#define TASM_SOURCE_SPAN_NULL                                                   \
    ((TasmSourceSpan){                                                          \
        .doc = NULL, .start = TASM_SOURCE_LOC_ZERO, .end = TASM_SOURCE_LOC_ZERO \
    })

TasmSourceSpan tasm_srcspan_make(const TasmSourceDocument* doc, TasmSourceLocation start, TasmSourceLocation end);

/// Returns a string view of the content covered by the span.
StringView tasm_srcspan_to_sv(TasmSourceSpan span);

/// Merges two spans into one that covers both.
/// Spans must be from the same document.
TasmSourceSpan tasm_srcspan_merge(TasmSourceSpan a, TasmSourceSpan b);

/// Returns true if the span is not null and has a document.
bool tasm_srcspan_is_valid(TasmSourceSpan span);

/// Returns true if the span is empty (start == end).
bool tasm_srcspan_is_empty(TasmSourceSpan span);

