#include <tasm/driver/driver.h>
#include <tasm/util/argparse.h>
#include <tasm/util/diag.h>

int main(int argc, const char* argv[]) {
    tasm_ansi_enabled = tasm_color_auto(stderr);

    TasmCmdline cmdline;
    TasmArgparseResult result = tasm_parse_args(argc, argv, &cmdline);
    if (result == TASM_ARGPARSE_FAIL) return 2;
    if (result == TASM_ARGPARSE_SKIP) return 0;

    switch (cmdline.color) {
    case TASM_PREF_ALWAYS:
        tasm_ansi_enabled = true;
        break;
    case TASM_PREF_AUTO:
        // it is already set to auto at the beggining of main()
        break;
    case TASM_PREF_NEVER:
        tasm_ansi_enabled = false;
        break;
    }

    TasmDriver driver;
    tasm_driver_init(&driver, &cmdline);
    return tasm_driver_run(&driver);
}
