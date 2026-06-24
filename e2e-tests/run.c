// this test runner actually runs exactly one test at a time
/// because iterating over directories etc. in C is a pain. so this
/// is run for every test separately from the Makefile. second advantage
/// of this approach is that make supports concurrency by default
/// so i guess it will be faster.

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#include <unistd.h>
#include <sys/wait.h>

#include <tc48/system.h>
#include <tc48/mem.h>
#include <tc48/cpu.h>
#include <tc48/bus.h>
#include <tc48/util.h>

#include <tobj/link.h>

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

int compile(const char* tasm_path, const char* input_path, const char* output_path, bool tobj) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        if (tobj) {
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

static void object_path_from_exe(char* obj_path, size_t obj_path_size, const char* exe_path) {
    size_t len = strlen(exe_path);
    if (len > 5 && strcmp(exe_path + len - 5, ".t48b") == 0) {
        snprintf(obj_path, obj_path_size, "%.*s.obj.t48b", (int)(len - 5), exe_path);
    } else {
        snprintf(obj_path, obj_path_size, "%s.obj.t48b", exe_path);
    }
}

static int link_tobj(const char* obj_path, const char* exe_path) {
    tc48_memory* obj = tc48_load_t48b(obj_path);
    if (obj == NULL) {
        return RET_ERR;
    }

    tc48_memory* exe = NULL;
    tobj_link_result res = tobj_to_raw_exe((tobj_param){ .data = obj }, &exe);
    if (res.code != TOBJ_LINK_SUCCESS) {
        tobj_print_link_error(res, &print_error);
        tc48_mem_free(obj);
        return RET_ERR;
    }

    tc48_mem_save(exe, exe_path);
    tc48_mem_free(exe);
    tc48_mem_free(obj);
    return RET_PASS;
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
int run(const char* temp_path, const char* test_name) {
    tc48_system sys;
    tc48_system_init(&sys, MEM_SIZE);
    tc48_mem_open(sys.bus.mem, temp_path);

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

int main(int argc, const char* argv[]) {
    if (argc != 6) {
        fprintf(stderr, "usage: %s <path-to-tasm> <input.tasm> <temp.t48b> <test-name> <raw|tobj>\n", argv[0]);
        return 1;
    }

    const char* tasm_path  = argv[1];
    const char* input_path = argv[2];
    const char* temp_path  = argv[3];
    const char* test_name  = argv[4];
    const char* mode       = argv[5];

    bool tobj = strcmp(mode, "tobj") == 0;
    if (!tobj && strcmp(mode, "raw") != 0) {
        print_error("invalid mode '%s' (expected 'raw' or 'tobj')", mode);
        return RET_ERR;
    }

    char obj_path[4096];
    const char* compile_out = temp_path;
    if (tobj) {
        object_path_from_exe(obj_path, sizeof obj_path, temp_path);
        compile_out = obj_path;
    }

    if (compile(tasm_path, input_path, compile_out, tobj) != 0) {
        return RET_ERR;
    }

    if (tobj && link_tobj(obj_path, temp_path) != RET_PASS) {
        return RET_ERR;
    }

    return run(temp_path, test_name);
}
