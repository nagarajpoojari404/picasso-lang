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

static void init_logging(void) {
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
    log(LOG_FATAL, "die: %s", msg);
    perror(msg);
    exit(1);
}

static void get_tool_root(char *out, size_t sz) {
    const char *home = getenv("PICASSO_HOME");
    if (home) {
        snprintf(out, sz, "%s", home);
        log(LOG_INFO, "PICASSO_HOME found: %s", home);
        return;
    }

    log(LOG_WARN, "PICASSO_HOME not not found, defaulting to RUNFILES_DIR");
    const char *runfiles = getenv("RUNFILES_DIR");
    if (runfiles) {
        snprintf(out, sz, "%s/_main", runfiles);
        log(LOG_INFO, "RUNFILES_DIR found: %s", runfiles);
        return;
    }
    
    if (getcwd(out, sz) == NULL) {
        log(LOG_FATAL, "src files not found");
        perror("getcwd");
        exit(1);
    }
}

static void get_irgen(char *out, size_t sz) {
    const char *home = getenv("PICASSO_IRGEN");
    if (home) {
        snprintf(out, sz, "%s", home);
        log(LOG_INFO, "PICASSO_IRGEN found: %s", home);
        return;
    }
    log(LOG_WARN, "PICASSO_IRGEN not not found, defaulting to IRGEN_BIN");
    log(LOG_INFO, "IRGEN_BIN found: %s", IRGEN_BIN);
    snprintf(out, sz, "%s", IRGEN_BIN);
}

static void get_runtimelib(char *out, size_t sz) {
    const char *home = getenv("PICASSO_RUNTIME_LIB");
    if (home) {
        snprintf(out, sz, "%s", home);
        log(LOG_INFO, "PICASSO_RUNTIME_LIB found: %s", home);
        return;
    }
    log(LOG_WARN, "PICASSO_RUNTIME_LIB not not found, defaulting to RUNTIME_LIB_PATH");
    log(LOG_INFO, "IRGEN_BIN found: %s", RUNTIME_LIB_PATH);
    snprintf(out, sz, "%s", RUNTIME_LIB_PATH);
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

static const char* find_clang(void) {
    static char clang_path[PATH_MAX] = {0};
    if (clang_path[0] != '\0') return clang_path;

    /* Check environment variable first */
    const char *env_clang = getenv("PICASSO_CLANG");
    if (env_clang && access(env_clang, X_OK) == 0) {
        strncpy(clang_path, env_clang, sizeof(clang_path) - 1);
        return clang_path;
    }

    /* Try common locations for LLVM 14+ */
    const char *candidates[] = {
        "/opt/homebrew/opt/llvm@14/bin/clang",
        "/usr/local/opt/llvm@14/bin/clang",
        "clang",  /* fallback to PATH */
        NULL
    };

    for (int i = 0; candidates[i] != NULL; i++) {
        if (access(candidates[i], X_OK) == 0) {
            strncpy(clang_path, candidates[i], sizeof(clang_path) - 1);
            log(LOG_DEBUG, "Found clang at: %s", clang_path);
            return clang_path;
        }
    }

    log(LOG_ERROR, "Could not find clang. Set PICASSO_CLANG environment variable.");
    exit(1);
}

static const char* get_sdk_path(void) {
    static char sdk_path[PATH_MAX] = {0};
    if (sdk_path[0] != '\0') return sdk_path;

    /* Check environment variable first */
    const char *env_sdk = getenv("PICASSO_SDK_PATH");
    if (env_sdk && access(env_sdk, R_OK) == 0) {
        strncpy(sdk_path, env_sdk, sizeof(sdk_path) - 1);
        return sdk_path;
    }

    /* Use xcrun to find SDK path dynamically */
    FILE *fp = popen("xcrun --sdk macosx --show-sdk-path 2>/dev/null", "r");
    if (fp) {
        if (fgets(sdk_path, sizeof(sdk_path), fp) != NULL) {
            /* Remove trailing newline */
            size_t len = strlen(sdk_path);
            if (len > 0 && sdk_path[len - 1] == '\n') {
                sdk_path[len - 1] = '\0';
            }
            pclose(fp);
            if (access(sdk_path, R_OK) == 0) {
                log(LOG_DEBUG, "Found SDK at: %s", sdk_path);
                return sdk_path;
            }
        }
        pclose(fp);
    }

    /* Fallback to common location */
    const char *fallback = "/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk";
    if (access(fallback, R_OK) == 0) {
        strncpy(sdk_path, fallback, sizeof(sdk_path) - 1);
        return sdk_path;
    }

    log(LOG_ERROR, "Could not find macOS SDK. Set PICASSO_SDK_PATH environment variable.");
    exit(1);
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
            if (snprintf(stub, sizeof(stub), "%s/ffi_stub_darwin.c", tuDir) >= (int)sizeof(stub))
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
        const char *sdk = get_sdk_path();
        
        char ffi_include[PATH_MAX];
        snprintf(ffi_include, sizeof(ffi_include), "%s/usr/include/ffi", sdk);

        char *clang_ll[28];
        int i = 0;

        clang_ll[i++] = (char *)clang;
        clang_ll[i++] = "-S";
        clang_ll[i++] = "-emit-llvm";
        clang_ll[i++] = "-g";
        clang_ll[i++] = "-isysroot";
        clang_ll[i++] = (char *)sdk;

        if (runtimeIncDir) {
            clang_ll[i++] = "-I";
            clang_ll[i++] = (char *)runtimeIncDir;
        }

        clang_ll[i++] = "-I";
        clang_ll[i++] = (char *)tuDir;

        clang_ll[i++] = "-I";
        clang_ll[i++] = ffi_include;

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
            if (!strcmp(cent->d_name, "ffi_stub_darwin.c")) continue;

            char cfile[PATH_MAX];
            if (snprintf(cfile, sizeof(cfile), "%s/%s", tuDir, cent->d_name) >= (int)sizeof(cfile))
                die("cfile path too long");

            char obj[PATH_MAX];
            if (snprintf(obj, sizeof(obj), "%s/%s_%s.o", ffiObjDir, ent->d_name, cent->d_name) >= (int)sizeof(obj))
                die("obj path too long");

            char *cc_obj[20];
            i = 0;
            cc_obj[i++] = (char *)clang;
            cc_obj[i++] = "-c";
            cc_obj[i++] = "-fPIC";
            cc_obj[i++] = "-isysroot";
            cc_obj[i++] = (char *)sdk;
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
    printf("  PICASSO_SDK_PATH     macOS SDK root (overrides xcrun detection)\n");
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
        
        /* Set PICASSO_INCLUDE environment variable if not already set */
        if (getenv("PICASSO_INCLUDE") == NULL) {
            char picassoInclude[PATH_MAX];
            snprintf(picassoInclude, sizeof(picassoInclude), "%s/libs", toolRoot);
            if (setenv("PICASSO_INCLUDE", picassoInclude, 0) != 0) {
                log(LOG_WARN, "Failed to set PICASSO_INCLUDE");
            } else {
                log(LOG_INFO, "PICASSO_INCLUDE set to: %s", picassoInclude);
            }
        }
        
        char irgenPath[PATH_MAX];
        get_irgen(irgenPath, sizeof(irgenPath));
        char *irgen[] = { irgenPath, "gen", (char *)dir, NULL };
        run_cmd(irgen);

        /* .ll → .bc */
        log(LOG_INFO, "Converting .ll to .bc");
        char *ll_to_bc[] = {
            "sh", "-c",
            "set -e; for f in \"$1\"/*.ll; do "
            "b=$(basename \"$f\" .ll); "
            "llvm-as \"$f\" -o \"$1/$b.bc\"; "
            "done",
            "sh",
            buildDir,
            NULL
        };
        run_cmd(ll_to_bc);

        /* .bc → .o */
        log(LOG_INFO, "Compiling .bc to .o");
        
        /* Use llc to compile .bc to .o (handles LLVM version compatibility) */
        char bc_to_o_cmd[PATH_MAX * 2];
        snprintf(bc_to_o_cmd, sizeof(bc_to_o_cmd),
            "set -e; for f in \"$1\"/*.bc; do "
            "b=$(basename \"$f\" .bc); "
            "llc -filetype=obj \"$f\" -o \"$1/$b.o\"; "
            "done");
        
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
        
        const char *sdk = get_sdk_path();
        char ffi_include[PATH_MAX];
        snprintf(ffi_include, sizeof(ffi_include), "%s/usr/include/ffi", sdk);
        
        char link_cmd[PATH_MAX * 3];
        char runtimeLib[PATH_MAX];
        get_runtimelib(runtimeLib, sizeof(runtimeLib));
        const char *clang_for_link = find_clang();
        snprintf(link_cmd, sizeof(link_cmd),
            "set -e; "
            "OBJS=$1/*.o; "
            "FFI_OBJS=\"\"; "
            "if [ -d \"$1/tmp/ffi-obj\" ] && ls \"$1/tmp/ffi-obj\"/*.o >/dev/null 2>&1; then "
            "  FFI_OBJS=$1/tmp/ffi-obj/*.o; "
            "fi; "
            "\"%s\" $OBJS $FFI_OBJS "
            "-isysroot %s "
            "%s"
            " -o $1/a.out "
            " -rdynamic "
            " -I%s "
            " -lffi -lpthread -lm",
            clang_for_link, sdk, runtimeLib, ffi_include);
        
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
