// this test runner actually runs exactly one test at a time
/// because iterating over directories etc. in C is a pain. so this
/// is run for every test separately from the Makefile. second advantage
/// of this approach is that make supports concurrency by default
/// so i guess it will be faster.

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/wait.h>

#include <tc48/system.h>
#include <tc48/mem.h>
#include <tc48/cpu.h>
#include <tc48/bus.h>

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

void print_error(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    bool ansi = isatty(STDERR_FILENO);
    const char* red   = ansi ? "\033[31m" : "";
    const char* reset = ansi ? "\033[0m"  : "";
    fprintf(stderr, "%serror:%s ", red, reset);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
    va_end(ap);
}


// it's probably too much but we have
// gigabytes of RAM anyway so who cares
#define MEM_SIZE 531441

#define RET_PASS 0
#define RET_FAIL 1
#define RET_ERR  2

int compile(const char* tasm_path, const char* input_path, const char* temp_path) {
    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        const char* args[] = { tasm_path, input_path, temp_path, NULL };
        // this cast is probably unsafe but since execve api is shit,
        // i guess there is no better way. if it works don't touch it or sth.
        execve(tasm_path, (char* const*)args, NULL);
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

#define TRET_UNSET 9999999999999999

static tc48_word tret;
static bool finish;

static tc48_word test_dev_in(tc48_device* self, tc48_word offset, tc48_width width) {
    return (void) self, (void) offset, (void) width, 0;
}
static void test_dev_out(tc48_device* self, tc48_word offset, tc48_width width, tc48_word value) {
    (void) offset, (void) width;
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

int run(const char* temp_path, const char* test_name) {
    tc48_system sys;
    tc48_system_init(&sys, MEM_SIZE);
    tc48_mem_open(sys.bus.mem, temp_path);

    tc48_bus_register_device(&sys.bus, &test_dev_class, TEST_DEV_ADDR);

    tret = TRET_UNSET;
    finish = false;
    while (!finish) {
        tc48_system_step(&sys);
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
    if (argc != 5) {
        fprintf(stderr, "usage: %s <path-to-tasm> <input.tasm> <temp.t48b> <test-name>\n", argv[0]);
        return 1;
    }

    const char* tasm_path  = argv[1];
    const char* input_path = argv[2];
    const char* temp_path  = argv[3];
    const char* test_name  = argv[4];

    if (compile(tasm_path, input_path, temp_path) != 0) {
        return RET_ERR;
    }

    return run(temp_path, test_name);
}
