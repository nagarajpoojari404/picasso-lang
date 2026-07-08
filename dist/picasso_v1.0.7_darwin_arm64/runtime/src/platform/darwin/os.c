#include "os.h"

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <crt_externs.h>
#include <time.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <sys/attr.h>
#include <fcntl.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "str.h"

/* Function Prototypes */
/**
 * @brief Get current errno value.
 * @return errno.
 */
int __public__os_errno(void) {
    return errno;
}

/**
 * @brief Get current process ID.
 * @return PID.
 */
int __public__os_getpid(void) {
    return getpid();
}

/**
 * @brief Get parent process ID.
 * @return Parent PID.
 */
int __public__os_getppid(void) {
    return getppid();
}

/**
 * @brief Get current thread ID.
 * @note macOS implementation uses pthread_threadid_np().
 * @return Thread ID.
 */
uint64_t __public__os_gettid(void) {
    uint64_t tid = 0;
    pthread_threadid_np(NULL, &tid);
    return tid;
}

/**
 * @brief Terminate the current process.
 * @param code Exit status.
 */
void __public__os_exit(int code) {
    _exit(code);
}

/**
 * @brief Create a child process.
 * @return 0 in child, child PID in parent, -1 on error.
 */
int __public__os_fork(void) {
    return fork();
}

/**
 * @brief Wait for a child process.
 * @param pid     Process ID.
 * @param status  Exit status.
 * @param options Wait options.
 * @return PID or -1 on error.
 */
__public__os_waitpid_rt_t __public__os_waitpid(int pid, int options) {
    int64_t status;
    int64_t res = waitpid(pid, &status, options);
    return (__public__os_waitpid_rt_t){res, status};
}

/**
 * @brief Send a signal to a process.
 * @param pid Process ID.
 * @param sig Signal number.
 * @return 0 on success, -1 on error.
 */
int __public__os_kill(int pid, int sig) {
    return kill(pid, sig);
}

/**
 * @brief Execute a program.
 * @param path Executable path.
 * @param argv Argument vector (array of __public__string_t).
 * @param envp Environment (array of __public__string_t).
 * @return -1 on error.
 */
int __public__os_execve(__public__string_t *path, __public__array_t* argv, __public__array_t* envp) {
    // Convert array of __public__string_t to char*[]
    size_t argc = argv->length;
    size_t envc = envp->length;
    
    char **argv_constructed = (char **)malloc((argc + 1) * sizeof(char *));
    char **envp_constructed = (char **)malloc((envc + 1) * sizeof(char *));
    
    if (!argv_constructed || !envp_constructed) {
        free(argv_constructed);
        free(envp_constructed);
        return -1;
    }
    
    // Extract char* from each __public__string_t in argv
    __public__string_t **argv_strings = (__public__string_t **)argv->data;
    for (size_t i = 0; i < argc; i++) {
        argv_constructed[i] = argv_strings[i]->data;
    }
    argv_constructed[argc] = NULL;
    
    // Extract char* from each __public__string_t in envp
    __public__string_t **envp_strings = (__public__string_t **)envp->data;
    for (size_t i = 0; i < envc; i++) {
        envp_constructed[i] = envp_strings[i]->data;
    }
    envp_constructed[envc] = NULL;
    
    int result = execve(path->data, argv_constructed, envp_constructed);
    
    // execve only returns on error, but free anyway for completeness
    free(argv_constructed);
    free(envp_constructed);
    
    return result;
}

/**
 * @brief Execute a program using PATH lookup.
 * @param file Executable name.
 * @param argv Argument vector (array of __public__string_t).
 * @return -1 on error.
 */
int __public__os_execvp(__public__string_t *file, __public__array_t* argv) {
    // Convert array of __public__string_t to char*[]
    size_t argc = argv->length;
    
    char **argv_constructed = (char **)malloc((argc + 1) * sizeof(char *));
    if (!argv_constructed) {
        return -1;
    }
    
    // Extract char* from each __public__string_t
    __public__string_t **argv_strings = (__public__string_t **)argv->data;
    for (size_t i = 0; i < argc; i++) {
        argv_constructed[i] = argv_strings[i]->data;
    }
    argv_constructed[argc] = NULL;
    
    int result = execvp(file->data, argv_constructed);
    
    // execvp only returns on error, but free anyway for completeness
    free(argv_constructed);
    
    return result;
}

/**
 * @brief Get environment variable array.
 * @return Environment pointer.
 */
char **__public__os_environ(void) {
    return *_NSGetEnviron();
}

/**
 * @brief Get environment variable value.
 * @param key Variable name.
 * @return Value or NULL.
 */
const char *__public__os_getenv(__public__string_t *key) {
    return getenv(key->data);
}

/**
 * @brief Set environment variable.
 * @param key       Variable name.
 * @param value     Value.
 * @param overwrite Overwrite if exists.
 * @return 0 on success, -1 on error.
 */
int __public__os_setenv(__public__string_t *key, __public__string_t *value, int overwrite) {
    return setenv(key->data, value->data, overwrite);
}

/**
 * @brief Remove environment variable.
 * @param key Variable name.
 * @return 0 on success, -1 on error.
 */
int __public__os_unsetenv(__public__string_t *key) {
    return unsetenv(key->data);
}


/**
 * @brief Get current working directory.
 * @param buf  Output buffer.
 * @param size Buffer size.
 * @return 0 on success, -1 on error.
 */
int __public__os_getcwd(char *buf, int64_t size) {
    return getcwd(buf, (size_t)size) ? 0 : -1;
}

/**
 * @brief Change mode of given path.
 * @param path Directory/File path.
 * @param mode access mode.
 * @return 0 on success, -1 on error.
 */
int __public__os_chmod(__public__string_t *path, int64_t mode) {
    return fchmodat(AT_FDCWD, path->data, (mode_t)mode, 0);
}

/**
 * @brief Chown changes the numeric uid and gid of the named file.
 * If the file is a symbolic link, it changes the uid and gid of the link's target.
 * A uid or gid of -1 means to not change that value.
 * @param name Directory/File path.
 * @param uid uid.
 * @param gid uid.
 * @return 0 on success, -1 on error.
 */
int __public__os_chown(__public__string_t *name, int64_t uid, int64_t gid) {
    return fchownat(AT_FDCWD, name->data, (uid_t)uid, (gid_t)gid, 0);
}

/**
 * @brief Change working directory.
 * @param path Directory path.
 * @return 0 on success, -1 on error.
 */
int __public__os_chdir(__public__string_t *path) {
    return chdir(path->data);
}

/**
 * @brief Get user ID.
 * @return UID.
 */
int __public__os_getuid(void)  { return (int)getuid();  }
/**
 * @brief Get effective user ID.
 * @return Effective UID.
 */
int __public__os_geteuid(void) { return (int)geteuid(); }
/**
 * @brief Get group ID.
 * @return GID.
 */
int __public__os_getgid(void)  { return (int)getgid();  }
/**
 * @brief Get effective group ID.
 * @return Effective GID.
 */
int __public__os_getegid(void) { return (int)getegid(); }

/**
 * @brief Set user ID.
 * @param uid User ID.
 * @return 0 on success, -1 on error.
 */
int __public__os_setuid(int uid) { return setuid((uid_t)uid); }
/**
 * @brief Set group ID.
 * @param gid Group ID.
 * @return 0 on success, -1 on error.
 */
int __public__os_setgid(int gid) { return setgid((gid_t)gid); }

/**
 * @brief Set process group ID.
 * @param pid  Process ID.
 * @param pgid Process group ID.
 * @return 0 on success, -1 on error.
 */
int __public__os_setpgid(int pid, int pgid) { return setpgid(pid, pgid); }

/**
 * @brief Get process group ID.
 * @param pid Process ID.
 * @return Process group ID or -1 on error.
 */
int __public__os_getpgid(int pid) { return getpgid(pid); }

/**
 * @brief Get process group ID of calling process.
 * @return Process group ID.
 */
int __public__os_getpgrp(void) { return getpgrp(); }

/**
 * @brief Create a new session.
 * @return Session ID or -1 on error.
 */
int __public__os_setsid(void) { return setsid(); }

/**
 * @brief Get resource limits.
 * @param resource Resource type.
 * @param rlim     Output rlimit structure.
 * @return 0 on success, -1 on error.
 */
__public__os_getrlimit_rt_t __public__os_getrlimit(int resource) {
    struct rlimit rlim;
    int result = getrlimit(resource, &rlim);
    
    __public__os_getrlimit_rt_t ret;
    ret.result = (int64_t)result;
    ret.cur = (int64_t)rlim.rlim_cur;
    ret.max = (int64_t)rlim.rlim_max;
    
    return ret;
}

/**
 * @brief Set resource limits.
 * @param resource Resource type.
 * @param cur      Soft limit.
 * @param max      Hard limit.
 * @return 0 on success, -1 on error.
 */
int __public__os_setrlimit(int resource, int64_t cur, int64_t max) {
    struct rlimit rlim;
    rlim.rlim_cur = (rlim_t)cur;
    rlim.rlim_max = (rlim_t)max;
    return setrlimit(resource, &rlim);
}

/**
 * @brief Open a file.
 * @param path  File path.
 * @param flags Open flags.
 * @param mode  File mode.
 * @return File descriptor or -1.
 */
int __public__os_open(__public__string_t *path, int flags, int mode) {
    int fd = open(path->data, flags, mode);

    /*
     * macOS has no O_DIRECT.
     * F_NOCACHE disables buffer cache for this FD.
     */
    if (fd != -1 && (flags & __public__os_O_DIRECT)) {
        fcntl(fd, F_NOCACHE, 1);
    }

    return fd;
}

/**
 * @brief Close a file descriptor.
 * @param fd File descriptor.
 * @return 0 on success, -1 on error.
 */
int __public__os_close(int fd) {
    return close(fd);
}

/**
 * @brief Read from a file descriptor.
 * @param fd  File descriptor.
 * @param buf Buffer.
 * @param n   Bytes to read.
 * @return Bytes read or -1.
 */
int64_t __public__os_read(int fd, void *buf, int64_t n) {
    return (int64_t)read(fd, buf, (size_t)n);
}

/**
 * @brief Write to a file descriptor.
 * @param fd  File descriptor.
 * @param buf Buffer.
 * @param n   Bytes to write.
 * @return Bytes written or -1.
 */
int64_t __public__os_write(int fd, const void *buf, int64_t n) {
    return (int64_t)write(fd, buf, (size_t)n);
}

/**
 * @brief Reposition file offset.
 * @param fd     File descriptor.
 * @param offset Offset.
 * @param whence Seek mode.
 * @return New offset or -1.
 */
int64_t __public__os_lseek(int fd, int64_t offset, int whence) {
    return (int64_t)lseek(fd, (off_t)offset, whence);
}

/**
 * @brief Get file status.
 * @param fd File descriptor.
 * @return Stat structure with result.
 */
__public__os_fstat_rt_t __public__os_fstat(int fd) {
    struct stat st;
    int result = fstat(fd, &st);
    
    __public__os_fstat_rt_t ret;
    ret.result = (int64_t)result;
    ret.dev = (int64_t)st.st_dev;
    ret.ino = (int64_t)st.st_ino;
    ret.mode = (int64_t)st.st_mode;
    ret.nlink = (int64_t)st.st_nlink;
    ret.uid = (int64_t)st.st_uid;
    ret.gid = (int64_t)st.st_gid;
    ret.rdev = (int64_t)st.st_rdev;
    ret.size = (int64_t)st.st_size;
    ret.blksize = (int64_t)st.st_blksize;
    ret.blocks = (int64_t)st.st_blocks;
    ret.atime = (int64_t)st.st_atime;
    ret.mtime = (int64_t)st.st_mtime;
    ret.ctime = (int64_t)st.st_ctime;
    
    return ret;
}

/**
 * @brief Duplicate a file descriptor.
 * @param fd File descriptor.
 * @return New FD or -1.
 */
int __public__os_dup(int fd) {
    return dup(fd);
}

/**
 * @brief Duplicate a file descriptor to a specific value.
 *
 * @param oldfd Existing FD.
 * @param newfd Target FD.
 *
 * @return New FD or -1 on error.
 */
int __public__os_dup2(int oldfd, int newfd) {
    return dup2(oldfd, newfd);
}

/**
 * @brief Control file descriptor behavior.
 *
 * @param fd  File descriptor.
 * @param cmd Command (OS_F_*).
 * @param arg Command-specific argument.
 *
 * @return Result or -1 on error.
 */
int64_t __public__os_fcntl(int fd, int cmd, int64_t arg) {
    return (int64_t)fcntl(fd, cmd, (long)arg);
}

#ifndef RENAME_EXCL
#define RENAME_EXCL 0x00000001
#endif

#ifndef RENAME_SWAP
#define RENAME_SWAP 0x00000002
#endif


/**
 * @brief Create a directory.
 * @param path Directory path.
 * @param mode Permissions.
 * @return 0 on success, -1 on error.
 */
int __public__os_mkdir(__public__string_t *path, int mode) {
    return mkdirat(AT_FDCWD, path->data, (mode_t)mode);
}

/**
 * @brief Create a temporary directory.
 * @param path Directory template (must end with XXXXXX).
 * @param mode Permissions.
 * @return 0 on success, -1 on error.
 */
int __public__os_mkdir_temp(__public__string_t *path, int mode) {
    char *dir = mkdtemp(path->data);
    if (!dir) return -1;

    // mkdtemp creates dir with 0700, fix permissions
    if (chmod(dir, (mode_t)mode) != 0) return -1;
    printf("dir: %s \n", dir);
    return 0;
}
/**
 * @brief Remove a directory.
 * @param path Directory path.
 * @return 0 on success, -1 on error.
 */
int __public__os_rmdir(__public__string_t *path) {
    return unlinkat(AT_FDCWD, path->data, AT_REMOVEDIR);
}

/**
 * @brief Delete a file.
 * @param path File path.
 * @return 0 on success, -1 on error.
 */
int __public__os_unlink(__public__string_t *path) {
    return unlinkat(AT_FDCWD, path->data, 0);
}

/**
 * @brief Rename a file or directory.
 * @param oldpath Old path.
 * @param newpath New path.
 * @return 0 on success, -1 on error.
 */
int __public__os_rename(__public__string_t *oldpath, __public__string_t *newpath) {
    return renameat(AT_FDCWD, oldpath->data, AT_FDCWD, newpath->data);
}

/*
 * Linux renameat2() compatibility shim.
 * macOS provides renameatx_np().
 */
int __public__os_renameat2(__public__string_t *oldpath,
                           __public__string_t *newpath,
                           int flags) {
    unsigned int native = 0;
    if (flags & __public__os_RENAME_NOREPLACE) native |= RENAME_EXCL;
    if (flags & __public__os_RENAME_EXCHANGE)  native |= RENAME_SWAP;

    return renameatx_np(AT_FDCWD, oldpath->data,
                        AT_FDCWD, newpath->data,
                        native);
}

/**
 * @brief Create a hard link.
 * @param oldpath Existing path.
 * @param newpath Link path.
 * @return 0 on success, -1 on error.
 */
int __public__os_link(__public__string_t *oldpath, __public__string_t *newpath) {
    return linkat(AT_FDCWD, oldpath->data, AT_FDCWD, newpath->data, 0);
}

/**
 * @brief Create a symbolic link.
 * @param target   Target path.
 * @param linkpath Link path.
 * @return 0 on success, -1 on error.
 */
int __public__os_symlink(__public__string_t *target, __public__string_t *linkpath) {
    return symlinkat(target->data, AT_FDCWD, linkpath->data);
}

/**
 * @brief Read symbolic link contents.
 * @param path Symbolic link path.
 * @param buf  Output buffer.
 * @param size Buffer size.
 * @return Bytes read or -1 on error.
 */
int64_t __public__os_readlink(__public__string_t *path, char *buf, int64_t size) {
    return (int64_t)readlinkat(AT_FDCWD, path->data, buf, (size_t)size);
}

/**
 * @brief Get file status.
 * @param path File path.
 * @return Stat structure with result.
 */
__public__os_stat_rt_t __public__os_stat(__public__string_t *path) {
    struct stat st;
    int result = fstatat(AT_FDCWD, path->data, &st, 0);
    
    __public__os_stat_rt_t ret;
    ret.result = (int64_t)result;
    ret.dev = (int64_t)st.st_dev;
    ret.ino = (int64_t)st.st_ino;
    ret.mode = (int64_t)st.st_mode;
    ret.nlink = (int64_t)st.st_nlink;
    ret.uid = (int64_t)st.st_uid;
    ret.gid = (int64_t)st.st_gid;
    ret.rdev = (int64_t)st.st_rdev;
    ret.size = (int64_t)st.st_size;
    ret.blksize = (int64_t)st.st_blksize;
    ret.blocks = (int64_t)st.st_blocks;
    ret.atime = (int64_t)st.st_atime;
    ret.mtime = (int64_t)st.st_mtime;
    ret.ctime = (int64_t)st.st_ctime;
    
    return ret;
}

/**
 * @brief Get file status (don't follow symlinks).
 * @param path File path.
 * @return Stat structure with result.
 */
__public__os_stat_rt_t __public__os_lstat(__public__string_t *path) {
    struct stat st;
    int result = fstatat(AT_FDCWD, path->data, &st, AT_SYMLINK_NOFOLLOW);
    
    __public__os_stat_rt_t ret;
    ret.result = (int64_t)result;
    ret.dev = (int64_t)st.st_dev;
    ret.ino = (int64_t)st.st_ino;
    ret.mode = (int64_t)st.st_mode;
    ret.nlink = (int64_t)st.st_nlink;
    ret.uid = (int64_t)st.st_uid;
    ret.gid = (int64_t)st.st_gid;
    ret.rdev = (int64_t)st.st_rdev;
    ret.size = (int64_t)st.st_size;
    ret.blksize = (int64_t)st.st_blksize;
    ret.blocks = (int64_t)st.st_blocks;
    ret.atime = (int64_t)st.st_atime;
    ret.mtime = (int64_t)st.st_mtime;
    ret.ctime = (int64_t)st.st_ctime;
    
    return ret;
}

/*
 * Darwin directory reading.
 * WARNING: layout is struct dirent (NOT linux_dirent64).
 */
int __public__os_getdents64(int fd, void *buf, int64_t size) {
    DIR *dir;
    struct dirent *entry;
    char *ptr = buf;
    size_t remaining = size;

    // Convert fd to DIR* (fdopendir duplicates fd, so we need to close it)
    dir = fdopendir(fd);
    if (!dir) return -1;

    while ((entry = readdir(dir)) != NULL) {
        size_t reclen = entry->d_reclen;

        if (reclen > remaining)
            break;

        memcpy(ptr, entry, reclen);
        ptr += reclen;
        remaining -= reclen;
    }

    closedir(dir);

    return (int)(size - remaining);  // bytes written
}

/**
 * @brief Map memory.
 *
 * @param addr  Address hint.
 * @param len   Length.
 * @param prot  Protection flags.
 * @param flags Mapping flags.
 * @param fd    File descriptor.
 * @param off   Offset.
 *
 * @return Mapped address or MAP_FAILED.
 */
void *__public__os_mmap(void *addr,
                        int64_t len,
                        int prot,
                        int flags,
                        int fd,
                        int64_t off) {
    return mmap(addr, (size_t)len, prot, flags, fd, (off_t)off);
}

/**
 * @brief Unmap memory.
 * @param addr Address.
 * @param len  Length.
 * @return 0 on success, -1 on error.
 */
int __public__os_munmap(void *addr, int64_t len) {
    return munmap(addr, (size_t)len);
}

int64_t __public__os_page_size(void) {
    return (int64_t)sysconf(_SC_PAGESIZE);
}

int __public__os_mprotect(void *addr, int64_t len, int prot) {
    return mprotect(addr, (size_t)len, prot);
}

int __public__os_madvise(void *addr, int64_t len, int advice) {
    return madvise(addr, (size_t)len, advice);
}

/*
 * Verified on Darwin 21–23 (macOS 12–14)
 */
#define SYS_ulock_wait 515
#define SYS_ulock_wake 516

#define UL_COMPARE_AND_WAIT 1
#define UL_WAKE_ALL         0x00000100

int __public__os_futex_wait(int *uaddr,
                            int val,
                            const void *timeout) {
    const struct timespec *ts = (const struct timespec *)timeout;
    uint32_t timeout_us = 0;

    if (ts) {
        timeout_us = (uint32_t)(
            ts->tv_sec * 1000000 +
            ts->tv_nsec / 1000
        );
    }

    return (int)syscall(SYS_ulock_wait,
                        UL_COMPARE_AND_WAIT,
                        uaddr,
                        (uint64_t)val,
                        timeout_us);
}

int __public__os_futex_wake_one(int *uaddr) {
    return (int)syscall(SYS_ulock_wake,
                        UL_COMPARE_AND_WAIT,
                        uaddr,
                        0);
}

int __public__os_futex_wake_all(int *uaddr) {
    return (int)syscall(SYS_ulock_wake,
                        UL_COMPARE_AND_WAIT | UL_WAKE_ALL,
                        uaddr,
                        0);
}

int __public__os_signal_install(int sig, void (*handler)(int)) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = handler;
    sa.sa_flags   = SA_RESTART;

    sigemptyset(&sa.sa_mask);
    return sigaction(sig, &sa, NULL);
}

/**
 * @brief Check file accessibility.
 * @param path File path.
 * @param mode Access mode.
 * @return 0 on success, -1 on error.
 */
int __public__os_access(__public__string_t *path, int mode) {
    return access(path->data, mode);
}
