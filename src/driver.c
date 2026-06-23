#include <tasm/driver/driver.h>

#include <tasm/util/argparse.h>

#include <tasm/srcdoc/srcdoc.h>
#include <tasm/lexer/lexer.h>
#include <tasm/parser/parser.h>

#include <tasm/asr/buf.h>
#include <tasm/util/diag.h>

#include <tasm/asm/backends/raw.h>

#include <tasm/asr/dump.h>

void tasm_driver_init(TasmDriver* driver, const TasmCmdline* cmdline) {
    driver->cmdline = cmdline;
    driver->diag = (TasmDiagEngine) {0};
}

static tc48_memory* dispatch_compilation(TasmDriver* driver, TasmAsrBuf* asr) {
    switch (driver->cmdline->format) {
    case TASM_FORMAT_RAW: {
        TasmBackendRaw as;
        tasm_raw_asm_init(&as, asr, &driver->diag);
        tc48_memory* out = tasm_assemble_to_raw(&as);
        tasm_raw_asm_free(&as);
        return out;
    }
    case TASM_FORMAT_TOBJ:
        tasm_print_error("emitting TOBJ file not yet supported");
        return NULL;
    }
}

int tasm_driver_run(TasmDriver* driver) {
    tasm_ansi_enabled = tasm_color_auto(stderr);

    if (VECTOR_SIZE(&driver->cmdline->input_files) != 1) {
        tasm_print_error("mulitiple input files not yet supported");
        return 1;
    }

    const char* input_path  = driver->cmdline->input_files.begin[0];
    const char* output_path = driver->cmdline->output != NULL ? driver->cmdline->output : "out.t48b";

    TasmSourceDocument source;
    tasm_srcdoc_init_from_file(&source, input_path);

    TasmLexer lexer;
    tasm_lexer_init(&lexer, &source);

    TasmParser parser;
    tasm_parser_init(&parser, &lexer, &driver->diag);

    TasmAsrBuf asr;
    tasm_asr_init(&asr);
    tasm_parse(&parser, &asr);

    TasmBackendRaw as;
    tasm_raw_asm_init(&as, &asr, &driver->diag);

    tc48_memory* prog = dispatch_compilation(driver, &asr);
    if (driver->diag.error_count == 0) {
        tc48_mem_save(prog, output_path);
    } else {
        tasm_print_error("assembly failed with %u errors", (unsigned)driver->diag.error_count);
    }

    tasm_asr_free(&asr);
    tasm_srcdoc_free(&source);

    return driver->diag.error_count == 0 ? 0 : 1;
}
