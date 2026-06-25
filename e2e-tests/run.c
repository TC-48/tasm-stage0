// this test runner actually runs exactly one test at a time
/// because iterating over directories etc. in C is a pain. so this
/// is run for every test separately from the Makefile. second advantage
/// of this approach is that make supports concurrency by default
/// so i guess it will be faster.

// TODO: make the test runner cross-platform

#define _DEFAULT_SOURCE
#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <linux/limits.h>

#include <tc48/system.h>
#include <tc48/mem.h>
#include <tc48/cpu.h>
#include <tc48/bus.h>
#include <tc48/util.h>

#include <tasm/util/argparse.h>

#include <tobj/link.h>
#include <tobj/merge.h>

#include <vector/vector.h>

void print_pass(const char* test_name) {
    bool ansi = isatty(STDERR_FILENO);
    const char* green = ansi ? "\033[32m" : "";
    const char* reset = ansi ? "\033[0m"  : "";
    fprintf(stderr, "%s[PASS]%s test '%s' passed\n", green, reset, test_name);
}
void print_fail(const char* test_name, tc48_word tret) {
    bool ansi = isatty(STDERR_FILENO);
    const char* red   = ansi ? "\033[31m" : "";
    const char* reset = ansi ? "\033[0m"  : "";
    fprintf(stderr, "%s[FAIL]%s test '%s' failed with code %llu\n", red, reset, test_name, (unsigned long long)tret);
}

int print_error(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    bool ansi = isatty(STDERR_FILENO);
    const char* red   = ansi ? "\033[31m" : "";
    const char* reset = ansi ? "\033[0m"  : "";
    fprintf(stderr, "%serror:%s ", red, reset);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
    return 0;
}


// it's probably too much but we have
// gigabytes of RAM anyway so who cares
#define MEM_SIZE 531441

#define RET_PASS 0
#define RET_FAIL 1
#define RET_ERR  2
#define RET_TIME 3

int compile(const char* tasm_path, const char* input_path, const char* output_path, TasmOutputFormat format) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        if (format == TASM_FORMAT_TOBJ) {
            const char* args[] = { tasm_path, "-f", "tobj", input_path, "-o", output_path, NULL };
            execve(tasm_path, (char* const*)args, NULL);
        } else {
            const char* args[] = { tasm_path, input_path, "-o", output_path, NULL };
            execve(tasm_path, (char* const*)args, NULL);
        }
        perror("execve");
        exit(1);
    } else {
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            return 1;
        }
        if (WIFEXITED(status)) {
            return WEXITSTATUS(status);
        }
        return 1;
    }
}

void replace_extension(char* dest, size_t size, const char* src, const char* new_ext) {
    const char* dot = strrchr(src, '.');
    size_t base_len =
        dot != NULL
            ? (size_t)(dot - src)
            : strlen(src);

    snprintf(dest, size, "%.*s.%s", (int)base_len, src, new_ext);
}

#define TRET_UNSET 9999999999999999

static tc48_word tret;
static bool finish;

static tc48_word test_dev_in(tc48_device* self, tc48_word offset, tc48_width width) {
    return (void) self, (void) offset, (void) width, 0;
}
static void test_dev_out(tc48_device* self, tc48_word offset, tc48_width width, tc48_word value) {
    (void)self, (void) offset, (void) width;
    tret = value;
    finish = true;
}

tc48_device_class test_dev_class = {
    .name = "Test device",
    .type_id = 2934874575,
    .ports = 1,

    .init = NULL, .deinit = NULL,
    .in = test_dev_in, .out = test_dev_out,
};

// QQQQQQQQQ in septemvigesimal (i love this name)
#define TEST_DEV_ADDR 7625597484986

#define MAX_STEPS 1000000
int run(const char* exe_path, const char* test_name) {
    tc48_system sys;
    tc48_system_init(&sys, MEM_SIZE);
    tc48_mem_open(sys.bus.mem, exe_path);

    tc48_bus_register_device(&sys.bus, &test_dev_class, TEST_DEV_ADDR);

    tret = TRET_UNSET;
    finish = false;

    tc48_word steps = 0;
    while (!finish) {
        tc48_system_step(&sys);
        if (++steps >= MAX_STEPS) {
            print_error("test '%s' exceeded the maximum execution steps", test_name);
            tc48_system_deinit(&sys);
            return RET_TIME;
        }
    }

    tc48_system_deinit(&sys);

    if (tret == TRET_UNSET) {
        print_error("the test didn't return");
        return RET_ERR;
    } else if (tret == 0) {
        print_pass(test_name);
        return RET_PASS;
    } else {
        print_fail(test_name, tret);
        return RET_FAIL;
    }
}

VECTOR_DECLARE(object_list, objects, tc48_memory*);
VECTOR_DEFINE (object_list, objects, tc48_memory*);

int compile_and_link_tobjs(const char* exe_path, const char* tasm_path, const char* input_path, bool is_dir) {
    tc48_memory* object;
    if (is_dir) {
        object_list objects = {0};

        DIR* dir = opendir(input_path);
        struct dirent* ent;
        while ((ent = readdir(dir)) != NULL) {
            char actual_path[PATH_MAX];
            snprintf(actual_path, PATH_MAX, "%s/%s", input_path, ent->d_name);

            // "extension" yeah it is definitely not just extension but it should work
            char extension_in_quotation[PATH_MAX];
            snprintf(extension_in_quotation, PATH_MAX, "%s.obj.t48b", ent->d_name);

            char obj_path[PATH_MAX];
            replace_extension(obj_path, sizeof obj_path, exe_path, extension_in_quotation);

            int ret = compile(tasm_path, actual_path, obj_path, TASM_FORMAT_TOBJ);
            if (ret != 0) return ret;

            tc48_memory* obj = tc48_load_t48b(obj_path);
            if (obj == NULL) return RET_ERR;
            objects_push(&objects, obj);
        }

        size_t count = VECTOR_SIZE(&objects);

        // maybe VLAs aren't the best solution
        // but if it works, don't touch it!
        tobj_param params[count];
        for (size_t i = 0; i < count; ++i) {
            params[i] = (tobj_param) { .data = objects.begin[i] };
        }

        object = tobj_merge(params, (tc48_half)count);
        for (size_t i = 0; i < count; ++i) {
            tc48_mem_free(objects.begin[i]);
        }
        objects_free(&objects);
    } else {
        char obj_path[PATH_MAX];
        replace_extension(obj_path, PATH_MAX, exe_path, "obj.t48b");

        int ret = compile(tasm_path, input_path, obj_path, TASM_FORMAT_TOBJ);
        if (ret != 0) return ret;

        object = tc48_load_t48b(obj_path);
    }


    if (object == NULL) return RET_ERR;

    tc48_memory* exe = NULL;
    tobj_link_result res = tobj_to_raw_exe((tobj_param){ .data = object }, &exe);
    if (res.code != TOBJ_LINK_SUCCESS) {
        tobj_print_link_error(res, &print_error);
        tc48_mem_free(object);
        return RET_ERR;
    }

    tc48_mem_save(exe, exe_path);
    tc48_mem_free(exe);
    tc48_mem_free(object);
    return RET_PASS;
}

int compile_and_link_if_needed(const char* exe_path, const char* tasm_path, const char* input_path, TasmOutputFormat format) {
    struct stat st;
    if (stat(input_path, &st) == -1) {
        perror("stat");
        return RET_ERR;
    }

    switch (format) {
    case TASM_FORMAT_TOBJ:
        return compile_and_link_tobjs(exe_path, tasm_path, input_path, S_ISDIR(st.st_mode));
    case TASM_FORMAT_RAW:
        return compile(tasm_path, input_path, exe_path, TASM_FORMAT_RAW);
    }

    // to make the compiler happy
    return RET_ERR;
}

int main(int argc, const char* argv[]) {
    if (argc != 6) {
        fprintf(stderr, "usage: %s <path-to-tasm> <input.tasm> <temp.t48b> <test-name> <raw|tobj>\n", argv[0]);
        return 1;
    }

    const char* tasm_path  = argv[1];
    const char* input_path = argv[2];
    const char* exe_path  = argv[3];
    const char* test_name  = argv[4];
    const char* mode       = argv[5];

    TasmOutputFormat format;
    if (!tasm_ap_parse_format(mode, &format)) {
        print_error("invalid mode '%s' (expected 'raw' or 'tobj')", mode);
        return RET_ERR;
    }

    int ret = compile_and_link_if_needed(exe_path, tasm_path, input_path, format);
    if (ret != RET_PASS) return ret;

    return run(exe_path, test_name);
}
