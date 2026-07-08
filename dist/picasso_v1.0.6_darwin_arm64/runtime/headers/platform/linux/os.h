#ifndef OS_H
#define OS_H

#include "platform.h"
#include "str.h"
#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sched.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <dirent.h>
#include <linux/futex.h>

#ifndef O_DIRECT
#define O_DIRECT 0
#endif

/** Operation would block */
const int __public__os_EAGAIN = EAGAIN;
/** Interrupted system call */
const int __public__os_EINTR  = EINTR;
/** Invalid argument */
const int __public__os_EINVAL = EINVAL;
/** Permission denied */
const int __public__os_EPERM  = EPERM;
/** No such file or process */
const int __public__os_ENOENT = ENOENT;
/** Out of memory */
const int __public__os_ENOMEM = ENOMEM;


const int __public__os_WNOHANG   = WNOHANG;
const int __public__os_WUNTRACED = WUNTRACED;
const int __public__os_WCONTINUED = WCONTINUED;

const int __public__os_SIGINT  = SIGINT;
const int __public__os_SIGTERM = SIGTERM;
const int __public__os_SIGKILL = SIGKILL;
const int __public__os_SIGSEGV = SIGSEGV;
const int __public__os_SIGABRT = SIGABRT;
const int __public__os_SIGCHLD = SIGCHLD;
const int __public__os_SIGPIPE = SIGPIPE;
const int __public__os_SIGALRM = SIGALRM;
const int __public__os_SIGUSR1 = SIGUSR1;
const int __public__os_SIGUSR2 = SIGUSR2;

const int __public__os_RLIMIT_CPU    = RLIMIT_CPU;
const int __public__os_RLIMIT_FSIZE = RLIMIT_FSIZE;
const int __public__os_RLIMIT_DATA  = RLIMIT_DATA;
const int __public__os_RLIMIT_STACK = RLIMIT_STACK;
const int __public__os_RLIMIT_CORE  = RLIMIT_CORE;
const int __public__os_RLIMIT_NOFILE = RLIMIT_NOFILE;
const int __public__os_RLIMIT_AS    = RLIMIT_AS;

/* Standard file descriptor numbers */
/** Standard input */
const int __public__os_STDIN_FD  = 0;
/** Standard output */
const int __public__os_STDOUT_FD = 1;
/** Standard error */
const int __public__os_STDERR_FD = 2;

/*open() flags*/
const int __public__os_O_RDONLY   = O_RDONLY;
const int __public__os_O_WRONLY   = O_WRONLY;
const int __public__os_O_RDWR     = O_RDWR;
const int __public__os_O_APPEND   = O_APPEND;
const int __public__os_O_CREAT    = O_CREAT;
const int __public__os_O_EXCL     = O_EXCL;
const int __public__os_O_TRUNC    = O_TRUNC;
const int __public__os_O_CLOEXEC  = O_CLOEXEC;
const int __public__os_O_NONBLOCK = O_NONBLOCK;
const int __public__os_O_SYNC     = O_SYNC;
const int __public__os_O_DSYNC    = O_DSYNC;
const int __public__os_O_DIRECT   = O_DIRECT;

/*seek constants*/
const int __public__os_SEEK_SET = SEEK_SET;
const int __public__os_SEEK_CUR = SEEK_CUR;
const int __public__os_SEEK_END = SEEK_END;

/*fcntl commands*/
const int __public__os_F_DUPFD        = F_DUPFD;
const int __public__os_F_DUPFD_CLOEXEC = F_DUPFD_CLOEXEC;
const int __public__os_F_GETFD        = F_GETFD;
const int __public__os_F_SETFD        = F_SETFD;
const int __public__os_F_GETFL        = F_GETFL;
const int __public__os_F_SETFL        = F_SETFL;

/*FD flags*/
const int __public__os_FD_CLOEXEC = FD_CLOEXEC;

/*stat mode bits*/
const int __public__os_S_IFREG = S_IFREG;
const int __public__os_S_IFDIR = S_IFDIR;
const int __public__os_S_IFCHR = S_IFCHR;
const int __public__os_S_IFBLK = S_IFBLK;
const int __public__os_S_IFIFO = S_IFIFO;
const int __public__os_S_IFLNK = S_IFLNK;
const int __public__os_S_IFSOCK = S_IFSOCK;

const int __public__os_S_IRUSR = S_IRUSR;
const int __public__os_S_IWUSR = S_IWUSR;
const int __public__os_S_IXUSR = S_IXUSR;
const int __public__os_S_IRGRP = S_IRGRP;
const int __public__os_S_IWGRP = S_IWGRP;
const int __public__os_S_IXGRP = S_IXGRP;
const int __public__os_S_IROTH = S_IROTH;
const int __public__os_S_IWOTH = S_IWOTH;
const int __public__os_S_IXOTH = S_IXOTH;

/* Errors (FD-relevant subset)*/
const int __public__os_EBADF  = EBADF;
const int __public__os_EPIPE  = EPIPE;
const int __public__os_EIO    = EIO;
const int __public__os_ENOSPC = ENOSPC;

/*Special directory FDs*/
/** Current working directory */
const int __public__os_AT_FDCWD = AT_FDCWD;

/*unlinkat / renameat flags*/
const int __public__os_AT_REMOVEDIR = AT_REMOVEDIR;
const int __public__os_AT_SYMLINK_FOLLOW = AT_SYMLINK_FOLLOW;

/*link / rename flags*/
const int __public__os_RENAME_NOREPLACE = 1; // RENAME_NOREPLACE
const int __public__os_RENAME_EXCHANGE  = 2; // RENAME_EXCHANGE
const int __public__os_RENAME_WHITEOUT  = 4; // RENAME_WHITEOUT

/*Access mode flags*/
const int __public__os_F_OK = F_OK;
const int __public__os_R_OK = R_OK;
const int __public__os_W_OK = W_OK;
const int __public__os_X_OK = X_OK;

/*Directory entry types (d_type)*/
const int __public__os_DT_UNKNOWN = DT_UNKNOWN;
const int __public__os_DT_FIFO    = DT_FIFO;
const int __public__os_DT_CHR     = DT_CHR;
const int __public__os_DT_DIR     = DT_DIR;
const int __public__os_DT_BLK     = DT_BLK;
const int __public__os_DT_REG     = DT_REG;
const int __public__os_DT_LNK     = DT_LNK;
const int __public__os_DT_SOCK    = DT_SOCK;
const int __public__os_DT_WHT     = DT_WHT;


/*Memory protection flags*/
const int __public__os_PROT_NONE  = PROT_NONE;
const int __public__os_PROT_READ  = PROT_READ;
const int __public__os_PROT_WRITE = PROT_WRITE;
const int __public__os_PROT_EXEC  = PROT_EXEC;

/*mmap flags*/
const int __public__os_MAP_SHARED    = MAP_SHARED;
const int __public__os_MAP_PRIVATE   = MAP_PRIVATE;
const int __public__os_MAP_FIXED     = MAP_FIXED;
const int __public__os_MAP_ANONYMOUS = MAP_ANONYMOUS;
const int __public__os_MAP_STACK     = MAP_STACK;
const int __public__os_MAP_NORESERVE = MAP_NORESERVE;
const int __public__os_MAP_POPULATE  = MAP_POPULATE;
const int __public__os_MAP_GROWSDOWN = MAP_GROWSDOWN;

/*madvise advice*/
const int __public__os_MADV_NORMAL     = MADV_NORMAL;
const int __public__os_MADV_RANDOM     = MADV_RANDOM;
const int __public__os_MADV_SEQUENTIAL = MADV_SEQUENTIAL;
const int __public__os_MADV_WILLNEED   = MADV_WILLNEED;
const int __public__os_MADV_DONTNEED   = MADV_DONTNEED;
const int __public__os_MADV_FREE       = MADV_FREE;
const int __public__os_MADV_DONTFORK   = MADV_DONTFORK;
const int __public__os_MADV_DOFORK     = MADV_DOFORK;
const int __public__os_MADV_MERGEABLE  = MADV_MERGEABLE;
const int __public__os_MADV_UNMERGEABLE = MADV_UNMERGEABLE;
const int __public__os_MADV_HUGEPAGE   = MADV_HUGEPAGE;
const int __public__os_MADV_NOHUGEPAGE = MADV_NOHUGEPAGE;

const int __public__os_MCL_CURRENT = MCL_CURRENT;
const int __public__os_MCL_FUTURE  = MCL_FUTURE;

const int __public__os_EFAULT = EFAULT;
const int __public__os_EACCES = EACCES;


/**
 * @brief Get the current thread-local errno value.
 * @return Current errno.
 */
int __public__os_errno(void);
/**
 * @brief Get current process ID.
 * @return Process ID.
 */
int __public__os_getpid(void);

/**
 * @brief Get parent process ID.
 * @return Parent process ID.
 */
int __public__os_getppid(void);

/**
 * @brief Get calling thread ID.
 * @return Thread ID.
 */
int __public__os_gettid(void);

/**
 * @brief Terminate the current process immediately.
 * @param code Exit status.
 */
void __public__os_exit(int code);

/**
 * @brief Create a new process.
 * @return 0 in child, child PID in parent, or -1 on error.
 */
int __public__os_fork(void);

/**
 * @brief Wait for process state change.
 * @param pid     Process ID to wait for.
 * @param status  Pointer to store exit status.
 * @param options wait options.
 * @return PID of child or -1 on error.
 */
int __public__os_waitpid(int pid, int *status, int options);

/**
 * @brief Send a signal to a process.
 * @param pid Target process ID.
 * @param sig Signal number.
 * @return 0 on success or -1 on error.
 */
int __public__os_kill(int pid, int sig);

/**
 * @brief Execute a program with explicit environment.
 * @param path Path to executable.
 * @param argv Argument vector.
 * @param envp Environment vector.
 * @return -1 on error (does not return on success).
 */
int __public__os_execve(__public__string_t *path, char *const argv[], char *const envp[]);

/**
 * @brief Execute a program using current environment.
 * @param file Executable file.
 * @param argv Argument vector.
 * @return -1 on error (does not return on success).
 */
int __public__os_execvp(__public__string_t *file, char *const argv[]);

extern char **environ;

/**
 * @brief Get pointer to environment array.
 * @return Pointer to environment vector.
 */
char **__public__os_environ(void);

/**
 * @brief Get environment variable value.
 * @param key Environment variable name.
 * @return Value string or NULL if not found.
 */
const char *__public__os_getenv(__public__string_t *key);

/**
 * @brief Set an environment variable.
 * @param key        Variable name.
 * @param value      Variable value.
 * @param overwrite Whether to overwrite existing value.
 * @return 0 on success or -1 on error.
 */
int __public__os_setenv(__public__string_t *key, __public__string_t *value, int overwrite);

/**
 * @brief Remove an environment variable.
 * @param key Variable name.
 * @return 0 on success or -1 on error.
 */
int __public__os_unsetenv(__public__string_t *key);

/**
 * @brief Get current working directory.
 * @param buf  Buffer to store path.
 * @param size Buffer size.
 * @return Length of path or -1 on error.
 */
int __public__os_getcwd(char *buf, size_t size);

/**
 * @brief Change mode of given path.
 * @param path Directory/File path.
 * @param mode access mode.
 * @return 0 on success, -1 on error.
 */
int __public__os_chmod(__public__string_t *path, int64_t mode);

/**
 * @brief Chown changes the numeric uid and gid of the named file.
 * If the file is a symbolic link, it changes the uid and gid of the link's target.
 * A uid or gid of -1 means to not change that value.
 * @param name Directory/File path.
 * @param uid uid.
 * @param gid uid.
 * @return 0 on success, -1 on error.
 */
int __public__os_chown(__public__string_t *name, int64_t uid, int64_t gid);

/**
 * @brief Change current working directory.
 * @param path New working directory.
 * @return 0 on success or -1 on error.
 */
int __public__os_chdir(__public__string_t *path);

/**
 * @brief Get real user ID.
 */
int __public__os_getuid(void);

/**
 * @brief Get effective user ID.
 */
int __public__os_geteuid(void);

/**
 * @brief Get real group ID.
 */
int __public__os_getgid(void);

/**
 * @brief Get effective group ID.
 */
int __public__os_getegid(void);

/**
 * @brief Set user ID.
 */
int __public__os_setuid(int uid);

/**
 * @brief Set group ID.
 */
int __public__os_setgid(int gid);

/**
 * @brief Set process group ID.
 */
int __public__os_setpgid(int pid, int pgid);

/**
 * @brief Get process group ID.
 */
int __public__os_getpgid(int pid);

/**
 * @brief Get current process group ID.
 */
int __public__os_getpgrp(void);

/**
 * @brief Create a new session.
 */
int __public__os_setsid(void);

/**
 * @brief Get resource limit.
 * @param resource Resource type.
 * @param rlim     Pointer to rlimit struct.
 */
int __public__os_getrlimit(int resource, void *rlim);

/**
 * @brief Set resource limit.
 * @param resource Resource type.
 * @param rlim     Pointer to rlimit struct.
 */
int __public__os_setrlimit(int resource, const void *rlim);

/**
 * @brief Install a signal handler.
 * @param sig     Signal number.
 * @param handler Signal handler function.
 * @return 0 on success or -1 on error.
 */
int __public__os_signal_install(int sig, void (*handler)(int));

/**
 * @brief Open a file.
 * @param path  File path.
 * @param flags Open flags (OS_O_*).
 * @param mode  File mode (for create).
 * @return File descriptor or -1 on error.
 */
int __public__os_open(__public__string_t *path, int flags, int mode);

/**
 * @brief Close a file descriptor.
 * @param fd File descriptor.
 * @return 0 on success or -1 on error.
 */
int __public__os_close(int fd);

/**
 * @brief Read from a file descriptor.
 * @param fd  File descriptor.
 * @param buf Buffer to fill.
 * @param n   Number of bytes.
 * @return Bytes read, 0 on EOF, or -1 on error.
 */
ssize_t __public__os_read(int fd, void *buf, size_t n);

/**
 * @brief Write to a file descriptor.
 * @param fd  File descriptor.
 * @param buf Data buffer.
 * @param n   Number of bytes.
 * @return Bytes written or -1 on error.
 */
ssize_t __public__os_write(int fd, const void *buf, size_t n);

/**
 * @brief Reposition file offset.
 * @param fd     File descriptor.
 * @param offset Offset.
 * @param whence OS_SEEK_*.
 * @return New offset or -1 on error.
 */
off_t __public__os_lseek(int fd, off_t offset, int whence);

/**
 * @brief Get file status.
 * @param fd File descriptor.
 * @param st Pointer to struct stat.
 * @return 0 on success or -1 on error.
 */
int __public__os_fstat(int fd, struct stat *st);

/**
 * @brief Duplicate a file descriptor.
 * @param fd File descriptor.
 * @return New file descriptor or -1 on error.
 */
int __public__os_dup(int fd);

/**
 * @brief Duplicate a file descriptor to a specific value.
 * @param oldfd Existing FD.
 * @param newfd Target FD.
 * @return New FD or -1 on error.
 */
int __public__os_dup2(int oldfd, int newfd);

/**
 * @brief Control file descriptor behavior.
 * @param fd  File descriptor.
 * @param cmd Command (OS_F_*).
 * @param arg Command-specific argument.
 * @return Result or -1 on error.
 */
int __public__os_fcntl(int fd, int cmd, long arg);

/**
 * @brief Create a directory.
 * @param path Directory path.
 * @param mode Permission bits.
 * @return 0 on success or -1 on error.
 */
int __public__os_mkdir(__public__string_t *path, int mode);

/**
 * @brief Create a temporary directory.
 * @param path Directory template (must end with XXXXXX).
 * @param mode Permissions.
 * @return 0 on success, -1 on error.
 */
int __public__os_mkdir_temp(__public__string_t *path, int mode);

/**
 * @brief Remove an empty directory.
 * @param path Directory path.
 * @return 0 on success or -1 on error.
 */
int __public__os_rmdir(__public__string_t *path);

/**
 * @brief Remove a file.
 * @param path File path.
 * @return 0 on success or -1 on error.
 */
int __public__os_unlink(__public__string_t *path);

/**
 * @brief Rename a filesystem object.
 * @param oldpath Source path.
 * @param newpath Destination path.
 * @return 0 on success or -1 on error.
 */
int __public__os_rename(__public__string_t *oldpath, __public__string_t *newpath);

/**
 * @brief Rename with flags.
 * @param oldpath Source path.
 * @param newpath Destination path.
 * @param flags   OS_RENAME_* flags.
 * @return 0 on success or -1 on error.
 */
int __public__os_renameat2(__public__string_t *oldpath, __public__string_t *newpath, int flags);

/**
 * @brief Create a hard link.
 * @param oldpath Existing file.
 * @param newpath New link path.
 * @return 0 on success or -1 on error.
 */
int __public__os_link(__public__string_t *oldpath, __public__string_t *newpath);

/**
 * @brief Create a symbolic link.
 * @param target Target path.
 * @param linkpath Symlink path.
 * @return 0 on success or -1 on error.
 */
int __public__os_symlink(__public__string_t *target, __public__string_t *linkpath);

/**
 * @brief Read a symbolic link.
 * @param path Symlink path.
 * @param buf  Buffer to receive target.
 * @param size Buffer size.
 * @return Number of bytes written or -1 on error.
 */
ssize_t __public__os_readlink(__public__string_t *path, char *buf, size_t size);

/**
 * @brief Get file metadata (follow symlinks).
 * @param path File path.
 * @param st   Stat buffer.
 * @return 0 on success or -1 on error.
 */
int __public__os_stat(__public__string_t *path, struct stat *st);

/**
 * @brief Get file metadata (do not follow symlinks).
 * @param path File path.
 * @param st   Stat buffer.
 * @return 0 on success or -1 on error.
 */
int __public__os_lstat(__public__string_t *path, struct stat *st);

/**
 * @brief Check access permissions.
 * @param path File path.
 * @param mode OS_*_OK flags.
 * @return 0 on success or -1 on error.
 */
int __public__os_access(__public__string_t *path, int mode);

/**
 * @brief Read directory entries.
 * This is a low-level primitive. The runtime must parse
 * linux_dirent64 structures manually.
 * @param fd   Directory file descriptor.
 * @param buf  Buffer for entries.
 * @param size Buffer size.
 *
 * @return Number of bytes read or -1 on error.
 */
int __public__os_getdents64(int fd, void *buf, size_t size);

/**
 * @brief Map virtual memory.
 * @param addr  Requested address (or NULL).
 * @param len   Length in bytes.
 * @param prot  Protection flags (OS_PROT_*).
 * @param flags Mapping flags (OS_MAP_*).
 * @param fd    File descriptor or -1.
 * @param off   File offset.
 * @return Pointer to mapped memory or MAP_FAILED.
 */
void *__public__os_mmap(void *addr, size_t len, int prot,
              int flags, int fd, size_t off);

/**
 * @brief Unmap virtual memory.
 * @param addr Mapped address.
 * @param len  Length in bytes.
 * @return 0 on success or -1 on error.
 */
int __public__os_munmap(void *addr, size_t len);

/**
 * @brief Change memory protection.
 * @param addr Mapped address.
 * @param len  Length in bytes.
 * @param prot New protection flags.
 * @return 0 on success or -1 on error.
 */
int __public__os_mprotect(void *addr, size_t len, int prot);

/**
 * @brief Advise kernel about memory usage.
 * @param addr   Address range.
 * @param len    Length in bytes.
 * @param advice OS_MADV_*.
 * @return 0 on success or -1 on error.
 */
int __public__os_madvise(void *addr, size_t len, int advice);

/**
 * @brief Lock memory into RAM.
 * @param addr Address range.
 * @param len  Length in bytes.
 * @return 0 on success or -1 on error.
 */
int __public__os_mlock(void *addr, size_t len);

/**
 * @brief Unlock memory.
 * @param addr Address range.
 * @param len  Length in bytes.
 * @return 0 on success or -1 on error.
 */
int __public__os_munlock(void *addr, size_t len);

/**
 * @brief Control future/current memory locking.
 * @param flags OS_MCL_* flags.
 * @return 0 on success or -1 on error.
 */
int __public__os_mlockall(int flags);

/**
 * @brief Unlock all locked memory.
 * @return 0 on success or -1 on error.
 */
int __public__os_munlockall(void);

/**
 * @brief Get system page size.
 * @return Page size in bytes.
 */
size_t __public__os_page_size(void);

/** @futex: */

/**
 * @brief Wait on a futex word.
 * The calling thread sleeps if *uaddr == val.
 * @param uaddr Pointer to futex word.
 * @param val Expected value.
 * @param timeout Optional timeout (NULL for infinite wait).
 * @return 0 on success or -1 on error.
 */
int __public__os_futex_wait(int *uaddr, int val, const struct timespec *timeout);

/**
 * @brief Wake up threads waiting on a futex word.
 * @param uaddr Pointer to futex word.
 * @param count Maximum number of waiters to wake.
 * @return Number of woken threads or -1 on error.
 */
int __public__os_futex_wake(int *uaddr, int count);

/**
 * @brief Wait on a futex word with bitmask.
 * The calling thread sleeps if (*uaddr & mask) == val.
 * @param uaddr Pointer to futex word.
 * @param val Expected value.
 * @param timeout Optional timeout.
 * @param mask Bitmask.
 * @return 0 on success or -1 on error.
 */
int __public__os_futex_wait_bitset(int *uaddr,int val,const struct timespec *timeout,int mask);

/**
 * @brief Wake threads waiting on a futex word using a bitmask.
 * @param uaddr Pointer to futex word.
 * @param count Maximum number of waiters to wake.
 * @param mask Bitmask.
 * @return Number of woken threads or -1 on error.
 */
int __public__os_futex_wake_bitset(int *uaddr, int count, int mask);

/**
 * @brief Requeue waiters from one futex to another.
 * Wakes up to wake_count waiters and requeues the rest to uaddr2.
 * @param uaddr Source futex.
 * @param wake_count Number of waiters to wake.
 * @param requeue_count Number of waiters to requeue.
 * @param uaddr2 Target futex.
 * @return Number of affected waiters or -1 on error.
 */
int __public__os_futex_requeue( int *uaddr, int wake_count, int requeue_count, int *uaddr2);

/**
 * @brief Wake one waiter and requeue remaining waiters.
 * @param uaddr Source futex.
 * @param uaddr2 Target futex.
 * @param wake_count Number of waiters to wake.
 * @param requeue_count Number of waiters to requeue.
 * @return Number of affected waiters or -1 on error.
 */
int __public__os_futex_cmp_requeue( int *uaddr, int *uaddr2, int wake_count, int requeue_count, int val);


/**
 * @brief Wake a single waiter (optimized common case).
 * @param uaddr Pointer to futex word.
 * @return Number of woken threads or -1 on error.
 */
int __public__os_futex_wake_one(int *uaddr);

/**
 * @brief Wake all waiters.
 * @param uaddr Pointer to futex word.
 * @return Number of woken threads or -1 on error.
 */
int __public__os_futex_wake_all(int *uaddr);
#endif