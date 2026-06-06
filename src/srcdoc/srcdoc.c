#include <tasm/srcdoc/srcdoc.h>

#include <strlib/sb.h>

#include <inttypes.h>
#include <stdio.h>
#include <stddef.h>

TasmSrcdocResult _tasm_srcdoc_ret_err(bool result) {
    if (!result) return TASM_SRCDOC_ERR_ALLOC;
    return TASM_SRCDOC_SUCCESS;
}

bool _tasm_srcdoc_get_file_size(FILE* f, usize* out_size) {
    if (fseek(f, 0, SEEK_END) != 0) {
        return false;
    }
    long signed_size = ftell(f);
    if (signed_size == -1L) {
        return false;
    }

    if (signed_size < 0 || (usize)signed_size > SIZE_MAX) {
        return false;
    }
    *out_size = (usize)signed_size;

    if (fseek(f, 0, SEEK_SET) != 0) {
        return false;
    }
    return true;
}

TasmSrcdocResult tasm_srcdoc_init_empty(TasmSourceDocument* srcdoc, StringView filename) {
    srcdoc->filename = filename;
    return _tasm_srcdoc_ret_err(sb_init(&srcdoc->content));
}
TasmSrcdocResult tasm_srcdoc_init_from_str(TasmSourceDocument* srcdoc, StringView sv, StringView filename) {
    srcdoc->filename = filename;
    return _tasm_srcdoc_ret_err(sb_init_from(&srcdoc->content, sv));
}

TasmSrcdocResult tasm_srcdoc_init_from_file(TasmSourceDocument* srcdoc, const char* path) {
    TasmSrcdocResult err = TASM_SRCDOC_SUCCESS;
    srcdoc->filename = sv_from_cstr(path);

    FILE* f = fopen(path, "rb");
    if (f == NULL) {
        err = TASM_SRCDOC_ERR_FOPEN;
        goto fail;
    }

    usize size;
    if (!_tasm_srcdoc_get_file_size(f, &size)) {
        err = TASM_SRCDOC_ERR_FTELL;
        goto fail;
    }

    if (!sb_init_with_cap(&srcdoc->content, size)) {
        err = TASM_SRCDOC_ERR_ALLOC;
        goto fail;
    }

    if (!sb_resize(&srcdoc->content, size)) {
        err = TASM_SRCDOC_ERR_ALLOC;
        sb_free(&srcdoc->content);
        goto fail;
    }

    usize readed = fread(srcdoc->content.data, 1, size, f);
    if (readed != size) {
        err = TASM_SRCDOC_ERR_FREAD;
        sb_free(&srcdoc->content);
        goto fail;
    }

    goto end;

fail:
    if (f != NULL) {
        fclose(f);
    }
end:
    return err;
}

TasmSrcdocResult tasm_srcdoc_copy(const TasmSourceDocument* src, TasmSourceDocument* dst) {
    return _tasm_srcdoc_ret_err(tasm_srcdoc_init_from_sb(dst, &src->content, src->filename));
}
void tasm_srcdoc_move(TasmSourceDocument* src, TasmSourceDocument* dst) {
    return tasm_srcdoc_init_from_sb_move(dst, &src->content, src->filename);
}

TasmSrcdocResult tasm_srcdoc_init_from_sb(TasmSourceDocument* srcdoc, const StringBuf* buf, StringView filename) {
    srcdoc->filename = filename;
    return _tasm_srcdoc_ret_err(sb_copy(buf, &srcdoc->content));
}
void tasm_srcdoc_init_from_sb_move(TasmSourceDocument* srcdoc, StringBuf* buf, StringView filename) {
    srcdoc->filename = filename;
    return sb_move(buf, &srcdoc->content);
}

void tasm_srcdoc_free(TasmSourceDocument* srcdoc)  { sb_free(&srcdoc->content); }
void tasm_srcdoc_clear(TasmSourceDocument* srcdoc) { sb_clear(&srcdoc->content); }

TasmSrcdocResult tasm_srcdoc_append_str(TasmSourceDocument* srcdoc, StringView sv) {
    return _tasm_srcdoc_ret_err(sb_append(&srcdoc->content, sv));
}

StringView tasm_srcdoc_content(const TasmSourceDocument* srcdoc) {
    return sb_view(&srcdoc->content);
}

usize tasm_srcdoc_length(const TasmSourceDocument* srcdoc) {
    return srcdoc->content.len;
}

TasmSrcdocResult tasm_srcdoc_print(const TasmSourceDocument* srcdoc, FILE* out) {
    if (sv_print(tasm_srcdoc_content(srcdoc), out) != srcdoc->content.len) {
        return TASM_SRCDOC_ERR_FWRITE;
    }
    return TASM_SRCDOC_SUCCESS;
}

