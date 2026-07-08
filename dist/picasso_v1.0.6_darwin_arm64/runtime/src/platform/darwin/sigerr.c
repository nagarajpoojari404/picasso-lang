#include "sigerr.h"
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <assert.h>
#include "str.h"

#define UNW_LOCAL_ONLY
#include <libunwind.h>

static int crash_pipe[2] = { -1, -1 };
static volatile sig_atomic_t handling_crash = 0;

/* Signal-safe write helper */
static void write_str(const char *s) {
    write(STDERR_FILENO, s, strlen(s));
}

/* Print stack trace using libunwind */
static void print_stacktrace_internal(void) {
    unw_cursor_t cursor;
    unw_context_t context;
    
    write_str("===== STACK TRACE =====\n");
    
    if (unw_getcontext(&context) < 0) {
        write_str("Failed to get unwind context\n");
        return;
    }
    
    if (unw_init_local(&cursor, &context) < 0) {
        write_str("Failed to initialize unwind cursor\n");
        return;
    }
    
    int frame = 0;
    while (unw_step(&cursor) > 0 && frame < 50) {
        unw_word_t ip, offset;
        char name[256];
        
        if (unw_get_reg(&cursor, UNW_REG_IP, &ip) < 0) {
            break;
        }
        
        name[0] = '\0';
        if (unw_get_proc_name(&cursor, name, sizeof(name), &offset) == 0) {
            char buf[512];
            int n = snprintf(buf, sizeof(buf), "\t#%d  0x%016lx in %s + 0x%lx\n", 
                           frame, (unsigned long)ip, name, (unsigned long)offset);
            if (n > 0 && n < (int)sizeof(buf)) {
                write(STDERR_FILENO, buf, (size_t)n);
            }
        } else {
            char buf[512];
            int n = snprintf(buf, sizeof(buf), "\t#%d  0x%016lx in <unknown>\n", 
                           frame, (unsigned long)ip);
            if (n > 0 && n < (int)sizeof(buf)) {
                write(STDERR_FILENO, buf, (size_t)n);
            }
        }
        frame++;
    }
}

/* Helper thread that prints stack traces from crashes */
static void *crash_thread_fn(void *arg) {
    (void)arg;
    int sig;
    
    while (read(crash_pipe[0], &sig, sizeof(sig)) > 0) {
        char buf[128];
        int n = snprintf(buf, sizeof(buf), "\n===== FATAL SIGNAL %d =====\n", sig);
        if (n > 0) write(STDERR_FILENO, buf, (size_t)n);
        
        print_stacktrace_internal();
        _exit(128 + sig);
    }
    
    return NULL;
}

/* Signal handler: minimal, writes to pipe */
static void crash_handler(int sig, siginfo_t *si, void *ctx) {
    (void)si;
    (void)ctx;
    
    if (handling_crash) {
        _exit(128 + sig);
    }
    handling_crash = 1;
    
    /* Notify helper thread via pipe */
    write(crash_pipe[1], &sig, sizeof(sig));
    
    /* Wait a bit for the helper thread to print */
    sleep(1);
    _exit(128 + sig);
}

/**
 * @brief Install fatal signal handlers
 */
void init_error_handlers(void) {
    /* Create pipe for deferred handling */
    if (pipe(crash_pipe) != 0) {
        _exit(1);
    }
    
    /* Spawn helper thread */
    pthread_t th;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    if (pthread_create(&th, &attr, crash_thread_fn, NULL) != 0) {
        pthread_attr_destroy(&attr);
        _exit(1);
    }
    
    pthread_attr_destroy(&attr);
    
    /* Setup fatal signal handlers */
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = crash_handler;
    sa.sa_flags = SA_SIGINFO | SA_RESTART;
    sigemptyset(&sa.sa_mask);
    
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
    if (handling_crash) {
        _exit(1);
    }
    handling_crash = 1;
    
    if (msg) {
        write(STDERR_FILENO, msg, strlen(msg));
        write(STDERR_FILENO, "\n", 1);
    }
    
    /* Print stack trace directly */
    print_stacktrace_internal();
    
    _exit(1);
}

void __public__rterr_die(__public__string_t* fmt) {
    assert(fmt != NULL);
    assert(fmt->data != NULL);
    __public__runtime_error(fmt->data);
}
