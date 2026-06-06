#include <tasm/srcdoc/srcspan.h>
#include <tasm/srcdoc/srcdoc.h>

#include <tasm/util/assert.h>

TasmSourceSpan tasm_srcspan_make(const TasmSourceDocument* doc, TasmSourceLocation start, TasmSourceLocation end) {
    return (TasmSourceSpan) {
        .doc   = doc,
        .start = start,
        .end   = end,
    };
}

StringView tasm_srcspan_to_sv(TasmSourceSpan span) {
    if (!span.doc) return SV_NULL;

    StringView full_content = tasm_srcdoc_content(span.doc);

    if (span.start.offset > span.end.offset) return SV_NULL;
    if (span.end.offset > full_content.len) return SV_NULL;

    return sv_slice(full_content, (usize)span.start.offset, (usize)span.end.offset);
}

TasmSourceSpan tasm_srcspan_merge(TasmSourceSpan a, TasmSourceSpan b) {
    if (!a.doc) return b;
    if (!b.doc) return a;

    TASM_ASSERT(a.doc == b.doc, "merging spans from different documents");

    TasmSourceLocation start = a.start.offset < b.start.offset ? a.start : b.start;
    TasmSourceLocation end   = a.end.offset > b.end.offset ? a.end : b.end;

    return tasm_srcspan_make(a.doc, start, end);
}

bool tasm_srcspan_is_valid(TasmSourceSpan span) {
    return span.doc != NULL;
}

bool tasm_srcspan_is_empty(TasmSourceSpan span) {
    return span.start.offset == span.end.offset;
}

