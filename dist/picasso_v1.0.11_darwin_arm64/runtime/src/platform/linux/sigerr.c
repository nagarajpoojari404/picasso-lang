#include "sigerr.h"

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <libunwind.h>
#include <stdio.h>

#include <assert.h>
#include "str.h"

static void write_str(const char *s) {
    write(STDERR_FILENO, s, strlen(s));
}

static void print_stacktrace_internal(void) {
    unw_cursor_t cursor;
    unw_context_t context;

    if (unw_getcontext(&context) < 0) return;
    if (unw_init_local(&cursor, &context) < 0) return;

    write_str("===== STACK TRACE =====\n");

    int i = 0;
    while (unw_step(&cursor) > 0) {
        unw_word_t ip;
        char name[256];

        unw_get_reg(&cursor, UNW_REG_IP, &ip);

        if (unw_get_proc_name(&cursor, name, sizeof(name), NULL) == 0) {
            char buf[512];
            int n = snprintf(buf, sizeof(buf),
                             "\t#%d %p %s\n",i++, (void *)ip, name);
            if (n > 0) write(STDERR_FILENO, buf, (size_t)n);
        }
    }
}

static void crash_handler(int sig, siginfo_t *si, void *ctx) {
    (void)si;
    (void)ctx;

    print_stacktrace_internal();
    _exit(128 + sig);
}

/**
 * @brief Install fatal signal handlers
 */
void init_error_handlers(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = crash_handler;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;

    int sigs[] = { SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT };
    for (size_t i = 0; i < sizeof(sigs)/sizeof(sigs[0]); i++) {
        sigaction(sigs[i], &sa, NULL);
    }
}

/**
 * @brief Raise a runtime error and print stack trace
 * @param msg Error message to display
 */
void __public__runtime_error(const char *msg) {
    if (msg) {
        write(STDERR_FILENO, msg, strlen(msg));
        write(STDERR_FILENO, "\n", 1);
    }

    print_stacktrace_internal();
    _exit(1);
}

void __public__rterr_die(__public__string_t* fmt) {
    assert(fmt != NULL);
    assert(fmt->data != NULL);
    __public__runtime_error(fmt->data);
}