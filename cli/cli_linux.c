#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <limits.h>
#include <errno.h>

#include <time.h>
#include <stdarg.h>

typedef enum {
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARN = 2,
    LOG_ERROR = 3,
    LOG_FATAL = 4
} LogLevel;

static LogLevel g_log_level = LOG_INFO;

#define COLOR_RESET   "\033[0m"
#define COLOR_DEBUG   "\033[36m"  /* Cyan */
#define COLOR_INFO    "\033[32m"  /* Green */
#define COLOR_WARN    "\033[33m"  /* Yellow */
#define COLOR_ERROR   "\033[31m"  /* Red */
#define COLOR_FATAL   "\033[35m"  /* Magenta */

static const char* log_level_string(LogLevel level) {
    switch (level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO:  return "INFO";
        case LOG_WARN:  return "WARN";
        case LOG_ERROR: return "ERROR";
        case LOG_FATAL: return "FATAL";
        default:        return "UNKNOWN";
    }
}

static const char* log_level_color(LogLevel level) {
    switch (level) {
        case LOG_DEBUG: return COLOR_DEBUG;
        case LOG_INFO:  return COLOR_INFO;
        case LOG_WARN:  return COLOR_WARN;
        case LOG_ERROR: return COLOR_ERROR;
        case LOG_FATAL: return COLOR_FATAL;
        default:        return COLOR_RESET;
    }
}

static void log(LogLevel level, const char *fmt, ...) {
    if (level < g_log_level) return;

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_buf[32];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);

    FILE *stream = (level >= LOG_ERROR) ? stderr : stdout;

    fprintf(stream, "%s[%s] [%s]%s ",
            log_level_color(level),
            time_buf,
            log_level_string(level),
            COLOR_RESET);

    va_list args;
    va_start(args, fmt);
    vfprintf(stream, fmt, args);
    va_end(args);

    fprintf(stream, "\n");
    fflush(stream);
}

static void init_logging() {
    const char *log_level_env = getenv("PICASSO_LOG_LEVEL");
    if (log_level_env) {
        if (strcmp(log_level_env, "DEBUG") == 0) g_log_level = LOG_DEBUG;
        else if (strcmp(log_level_env, "INFO") == 0) g_log_level = LOG_INFO;
        else if (strcmp(log_level_env, "WARN") == 0) g_log_level = LOG_WARN;
        else if (strcmp(log_level_env, "ERROR") == 0) g_log_level = LOG_ERROR;
        else if (strcmp(log_level_env, "FATAL") == 0) g_log_level = LOG_FATAL;
    }
}

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

static void get_tool_root(char *out, size_t sz) {
    const char *runfiles = getenv("RUNFILES_DIR");
    if (runfiles) {
        if (snprintf(out, sz, "%s/_main", runfiles) >= (int)sz)
            die("toolRoot path too long");
    } else {
        if (getcwd(out, sz) == NULL) {
            perror("getcwd");
            exit(1);
        }
    }
}

static void run_cmd(char *const argv[]) {
    pid_t pid = fork();
    if (pid < 0)
        die("fork");

    if (pid == 0) {
        execvp(argv[0], argv);
        perror(argv[0]);
        _exit(127);
    }

    int status;
    if (waitpid(pid, &status, 0) < 0)
        die("waitpid");

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        log(LOG_ERROR, "command failed: %s", argv[0]);
        exit(1);
    }
}

static const char* find_clang() {
    static char clang_path[PATH_MAX] = {0};
    if (clang_path[0] != '\0') return clang_path;

    /* Check environment variable first */
    const char *env_clang = getenv("PICASSO_CLANG");
    if (env_clang && access(env_clang, X_OK) == 0) {
        strncpy(clang_path, env_clang, sizeof(clang_path) - 1);
        return clang_path;
    }

    /* Try common locations for clang */
    const char *candidates[] = {
        "clang-16",
        "clang-15",
        "clang-14",
        "clang",
        "/usr/bin/clang-16",
        "/usr/bin/clang-15",
        "/usr/bin/clang-14",
        "/usr/bin/clang",
        NULL
    };

    for (int i = 0; candidates[i] != NULL; i++) {
        if (access(candidates[i], X_OK) == 0 || strchr(candidates[i], '/') == NULL) {
            /* If no slash, it's in PATH, just use it */
            strncpy(clang_path, candidates[i], sizeof(clang_path) - 1);
            log(LOG_DEBUG, "Using clang: %s", clang_path);
            return clang_path;
        }
    }

    log(LOG_ERROR, "Could not find clang. Set PICASSO_CLANG environment variable.");
    exit(1);
}

static const char* find_llvm_tool(const char *tool_name) {
    static char tool_paths[3][PATH_MAX] = {{0}};
    static int tool_index = 0;
    
    char *result = tool_paths[tool_index % 3];
    tool_index++;
    
    if (result[0] != '\0') return result;

    /* Check environment variable first */
    char env_var[64];
    snprintf(env_var, sizeof(env_var), "PICASSO_%s", tool_name);
    /* Convert to uppercase */
    for (char *p = env_var; *p; p++) {
        if (*p >= 'a' && *p <= 'z') *p = *p - 'a' + 'A';
        if (*p == '-') *p = '_';
    }
    
    const char *env_tool = getenv(env_var);
    if (env_tool && access(env_tool, X_OK) == 0) {
        strncpy(result, env_tool, PATH_MAX - 1);
        return result;
    }

    /* Try versioned and unversioned tools */
    char candidates[8][PATH_MAX];
    snprintf(candidates[0], PATH_MAX, "%s-16", tool_name);
    snprintf(candidates[1], PATH_MAX, "%s-15", tool_name);
    snprintf(candidates[2], PATH_MAX, "%s-14", tool_name);
    snprintf(candidates[3], PATH_MAX, "%s", tool_name);
    snprintf(candidates[4], PATH_MAX, "/usr/bin/%s-16", tool_name);
    snprintf(candidates[5], PATH_MAX, "/usr/bin/%s-15", tool_name);
    snprintf(candidates[6], PATH_MAX, "/usr/bin/%s-14", tool_name);
    snprintf(candidates[7], PATH_MAX, "/usr/bin/%s", tool_name);

    for (int i = 0; i < 8; i++) {
        if (access(candidates[i], X_OK) == 0 || strchr(candidates[i], '/') == NULL) {
            strncpy(result, candidates[i], PATH_MAX - 1);
            log(LOG_DEBUG, "Using %s: %s", tool_name, result);
            return result;
        }
    }

    log(LOG_ERROR, "Could not find %s. Set %s environment variable.", tool_name, env_var);
    exit(1);
}

static void find_lib_paths(char *lib_paths, size_t size) {
    /* Check environment variable first */
    const char *env_libs = getenv("PICASSO_LIB_PATHS");
    if (env_libs) {
        strncpy(lib_paths, env_libs, size - 1);
        return;
    }

    /* Detect architecture and set appropriate library paths */
    const char *arch_lib = NULL;
    
    #if defined(__aarch64__) || defined(__arm64__)
        arch_lib = "/usr/lib/aarch64-linux-gnu";
    #elif defined(__x86_64__)
        arch_lib = "/usr/lib/x86_64-linux-gnu";
    #elif defined(__i386__)
        arch_lib = "/usr/lib/i386-linux-gnu";
    #else
        arch_lib = "/usr/lib";
    #endif

    /* Build library path string */
    snprintf(lib_paths, size, "-L%s -L/usr/lib -L/usr/local/lib", arch_lib);
}

static void generate_ffi_irs_from_root( const char *ffiRoot, const char *tmpDir, const char *ffiObjDir, const char *runtimeIncDir, int gen_objects) {
    DIR *d = opendir(ffiRoot);
    if (!d && !runtimeIncDir) return;
    if (!d) die("opendir ffiRoot");

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue;

        char tuDir[PATH_MAX];
        if (snprintf(tuDir, sizeof(tuDir), "%s/%s", ffiRoot, ent->d_name) >= (int)sizeof(tuDir))
            die("tuDir path too long");

        struct stat st;
        if (stat(tuDir, &st) != 0 || !S_ISDIR(st.st_mode)) continue;

        char stub[PATH_MAX];
        if (snprintf(stub, sizeof(stub), "%s/ffi_stub.c", tuDir) >= (int)sizeof(stub))
            die("stub path too long");
            
        if (access(stub, R_OK) != 0) {
            if (snprintf(stub, sizeof(stub), "%s/ffi_stub_linux.c", tuDir) >= (int)sizeof(stub))
                die("stub path too long");

            if(access(stub, R_OK) != 0)  {
                log(LOG_WARN, "%s missing ffi_stub.c, skipping", tuDir);
                continue;
            }
        }

        char outLL[PATH_MAX];
        if (snprintf(outLL, sizeof(outLL), "%s/%s.ll", tmpDir, ent->d_name) >= (int)sizeof(outLL))
            die("outLL path too long");

        /* clang -S -emit-llvm */
        const char *clang = find_clang();
        
        char *clang_ll[20];
        int i = 0;

        clang_ll[i++] = (char *)clang;
        clang_ll[i++] = "-S";
        clang_ll[i++] = "-emit-llvm";
        clang_ll[i++] = "-g0";

        /* runtime headers first */
        if (runtimeIncDir) {
            clang_ll[i++] = "-I";
            clang_ll[i++] = (char *)runtimeIncDir;
        }

        /* TU-local headers */
        clang_ll[i++] = "-I";
        clang_ll[i++] = (char *)tuDir;

        clang_ll[i++] = (char *)stub;
        clang_ll[i++] = "-o";
        clang_ll[i++] = outLL;
        clang_ll[i++] = NULL;

        run_cmd(clang_ll);

        if (!gen_objects) continue;

        /* compile other .c → native .o */
        DIR *cd = opendir(tuDir);
        if (!cd) die("opendir tuDir");

        struct dirent *cent;
        while ((cent = readdir(cd)) != NULL) {
            if (!strstr(cent->d_name, ".c")) continue;
            if (!strcmp(cent->d_name, "ffi_stub.c")) continue;
            if (!strcmp(cent->d_name, "ffi_stub_linux.c")) continue;

            char cfile[PATH_MAX];
            if (snprintf(cfile, sizeof(cfile), "%s/%s", tuDir, cent->d_name) >= (int)sizeof(cfile))
                die("cfile path too long");

            char obj[PATH_MAX];
            if (snprintf(obj, sizeof(obj), "%s/%s_%s.o", ffiObjDir, ent->d_name, cent->d_name) >= (int)sizeof(obj))
                die("obj path too long");

            char *cc_obj[16];
            i = 0;
            cc_obj[i++] = "cc";
            cc_obj[i++] = "-c";
            cc_obj[i++] = "-fPIC";
            cc_obj[i++] = "-I";
            cc_obj[i++] = (char *)tuDir;
            if (runtimeIncDir) {
                cc_obj[i++] = "-I";
                cc_obj[i++] = (char *)runtimeIncDir;
            }
            cc_obj[i++] = cfile;
            cc_obj[i++] = "-o";
            cc_obj[i++] = obj;
            cc_obj[i++] = NULL;

            run_cmd(cc_obj);
        }
        closedir(cd);
    }
    closedir(d);
}

static void generate_ffi_irs(const char *dir, const char *buildDir) {
    char ffiRoot[PATH_MAX];
    snprintf(ffiRoot, sizeof(ffiRoot), "%s/c/ffi", dir);

    char tmpDir[PATH_MAX];
    snprintf(tmpDir, sizeof(tmpDir), "%s/tmp", buildDir);

    char ffiObjDir[PATH_MAX];
    snprintf(ffiObjDir, sizeof(ffiObjDir), "%s/tmp/ffi-obj", buildDir);

    run_cmd((char *[]){"mkdir", "-p", tmpDir, NULL});
    run_cmd((char *[]){"mkdir", "-p", ffiObjDir, NULL});

    generate_ffi_irs_from_root(
        ffiRoot,
        tmpDir,
        ffiObjDir,
        NULL,
        1
    );
}

static void print_help(void) {
    printf("picasso - Picasso language compiler and build tool\n\n");
    printf("Usage:\n");
    printf("  picasso build <project-dir>   Compile a Picasso project\n");
    printf("  picasso exec  <project-dir>   Run the built project executable\n");
    printf("  picasso clean <project-dir>   Remove build artifacts\n\n");
    printf("Flags:\n");
    printf("  --help       Show this help message\n");
    printf("  --version    Show version information\n\n");
    printf("Environment variables (set automatically by brew):\n");
    printf("  PICASSO_HOME         Runtime and stdlib root directory\n");
    printf("  PICASSO_IRGEN        Path to the IR generator binary\n");
    printf("  PICASSO_RUNTIME_LIB  Path to the runtime static library\n");
    printf("  PICASSO_CLANG        Clang binary used for FFI compilation\n");
    printf("  PICASSO_VERSION      Installed version\n");
}

static void print_version(void) {
    const char *ver = getenv("PICASSO_VERSION");
    if (ver && ver[0] != '\0') {
        printf("picasso %s\n", ver);
    } else {
        printf("picasso (version unknown)\n");
    }
}

/* main */
int main(int argc, char **argv) {
    init_logging();

    if (argc < 2) {
        print_help();
        return 1;
    }

    if (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h")) {
        print_help();
        return 0;
    }

    if (!strcmp(argv[1], "--version") || !strcmp(argv[1], "-v")) {
        print_version();
        return 0;
    }

    if (!strcmp(argv[1], "build")) {
        if (argc != 3) {
            log(LOG_ERROR, "picasso build <project root dir>");
            return 1;
        }

        const char *dir = argv[2];
        log(LOG_INFO, "Starting build for project: %s", dir);

        char buildDir[PATH_MAX];
        snprintf(buildDir, sizeof(buildDir), "%s/build", dir);

        run_cmd((char *[]){"mkdir", "-p", buildDir, NULL});

        /* project FFI IR + objects */
        log(LOG_INFO, "Generating FFI IRs for project");
        generate_ffi_irs(dir, buildDir);

        /* stdlib FFI IRs */
        char toolRoot[PATH_MAX];
        get_tool_root(toolRoot, sizeof(toolRoot));

        char libsDir[PATH_MAX];
        snprintf(libsDir, sizeof(libsDir), "%s/libs", toolRoot);

        char runtimeHdrs[PATH_MAX];
        snprintf(runtimeHdrs, sizeof(runtimeHdrs), "%s/runtime/headers", toolRoot);

        char tmpDir[PATH_MAX];
        snprintf(tmpDir, sizeof(tmpDir), "%s/build/tmp", dir);

        log(LOG_INFO, "Generating stdlib FFI IRs");
        generate_ffi_irs_from_root(
            libsDir,
            tmpDir,
            NULL,
            runtimeHdrs,
            0
        );

        /* language IR generation */
        log(LOG_INFO, "Running IR generation");
        char *irgen[] = { IRGEN_BIN, "gen", (char *)dir, NULL };
        run_cmd(irgen);

        /* .ll → .bc */
        log(LOG_INFO, "Converting .ll to .bc");
        
        const char *llvm_as = find_llvm_tool("llvm-as");
        char ll_to_bc_cmd[PATH_MAX * 2];
        snprintf(ll_to_bc_cmd, sizeof(ll_to_bc_cmd),
            "set -e; for f in \"$1\"/*.ll; do "
            "b=$(basename \"$f\" .ll); "
            "\"%s\" \"$f\" -o \"$1/$b.bc\"; "
            "done",
            llvm_as);
        
        char *ll_to_bc[] = {
            "sh", "-c",
            ll_to_bc_cmd,
            "sh",
            buildDir,
            NULL
        };
        run_cmd(ll_to_bc);

        /* .bc → .o */
        log(LOG_INFO, "Compiling .bc to .o");
        
        const char *llc = find_llvm_tool("llc");
        char bc_to_o_cmd[PATH_MAX * 2];
        snprintf(bc_to_o_cmd, sizeof(bc_to_o_cmd),
            "set -e; for f in \"$1\"/*.bc; do "
            "b=$(basename \"$f\" .bc); "
            "\"%s\" -filetype=obj \"$f\" -o \"$1/$b.o\"; "
            "done",
            llc);
        
        char *bc_to_o[] = {
            "sh", "-c",
            bc_to_o_cmd,
            "sh",
            buildDir,
            NULL
        };
        run_cmd(bc_to_o);

        /* final link */
        log(LOG_INFO, "Linking final executable");
        
        char lib_paths[512];
        find_lib_paths(lib_paths, sizeof(lib_paths));
        
        /* Detect architecture for unwind library */
        const char *unwind_arch = "";
        #if defined(__aarch64__) || defined(__arm64__)
            unwind_arch = "-lunwind-aarch64";
        #elif defined(__x86_64__)
            unwind_arch = "-lunwind-x86_64";
        #elif defined(__i386__)
            unwind_arch = "-lunwind-x86";
        #endif
        
        char link_cmd[PATH_MAX * 3];
        snprintf(link_cmd, sizeof(link_cmd),
            "set -e; "
            "OBJS=\"$1\"/*.o; "
            "FFI_OBJS=\"\"; "
            "if [ -d \"$1/tmp/ffi-obj\" ] && ls \"$1/tmp/ffi-obj\"/*.o >/dev/null 2>&1; then "
            "  FFI_OBJS=\"$1/tmp/ffi-obj\"/*.o; "
            "fi; "
            "cc $OBJS $FFI_OBJS "
            RUNTIME_LIB_PATH
            " -o \"$1/a.out\" "
            "-rdynamic "
            "%s "
            "-lffi -luring -lunwind %s "
            "-lpthread -lm",
            lib_paths, unwind_arch);
        
        char *link[] = {
            "sh", "-c",
            link_cmd,
            "sh",
            buildDir,
            NULL
        };
        run_cmd(link);

        log(LOG_INFO, "Build completed successfully");
        return 0;
    }

    if (!strcmp(argv[1], "exec")) {
        if (argc != 3) {
            log(LOG_ERROR, "picasso exec <project root dir>");
            return 1;
        }

        log(LOG_INFO, "Executing project: %s", argv[2]);
        char exe[PATH_MAX];
        snprintf(exe, sizeof(exe), "%s/build/a.out", argv[2]);
        execl(exe, exe, (char *)NULL);
        die("exec");
    }

    if (!strcmp(argv[1], "clean")) {
        if (argc != 3) {
            log(LOG_ERROR, "picasso clean <project root dir>");
            return 1;
        }

        log(LOG_INFO, "Cleaning project: %s", argv[2]);
        char buildDir[PATH_MAX];
        snprintf(buildDir, sizeof(buildDir), "%s/build", argv[2]);
        char libDir[PATH_MAX];
        snprintf(libDir, sizeof(libDir), "%s/picasso", argv[2]);
        run_cmd((char*[]){"rm", "-rf", buildDir,  NULL});
        run_cmd((char*[]){"rm", "-rf", libDir,  NULL});
        log(LOG_INFO, "Clean completed successfully");
        return 0;
    }

    log(LOG_ERROR, "unknown command: %s. Run 'picasso --help' for usage.", argv[1]);
    return 1;
}
