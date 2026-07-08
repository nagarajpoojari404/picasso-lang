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
#include <sys/wait.h>
#include <copyfile.h> // macOS specific for advanced file ops

/* * macOS does not support O_DIRECT. 
 * Direct I/O is achieved via fcntl(fd, F_NOCACHE, 1).
 */
#ifndef O_DIRECT
#define O_DIRECT 0
#endif

/* Error codes - Generally consistent with POSIX */
const int __public__os_EFAULT = EFAULT;

const int __public__os_WNOHANG    = WNOHANG;
const int __public__os_WUNTRACED  = WUNTRACED;
const int __public__os_WCONTINUED = WCONTINUED;

/* Generic / control */
const int __public__os_EPERM   = EPERM;
const int __public__os_EINVAL  = EINVAL;
const int __public__os_EAGAIN  = EAGAIN;
const int __public__os_EINTR   = EINTR;
const int __public__os_EIO     = EIO;

/* File / directory */
const int __public__os_ENOENT      = ENOENT;
const int __public__os_EEXIST      = EEXIST;
const int __public__os_ENOTDIR     = ENOTDIR;
const int __public__os_EISDIR      = EISDIR;
const int __public__os_ENOTEMPTY   = ENOTEMPTY;
const int __public__os_ENAMETOOLONG = ENAMETOOLONG;
const int __public__os_EBADF  = EBADF;
const int __public__os_EPIPE  = EPIPE;

/* Permissions */
const int __public__os_EACCES  = EACCES;
const int __public__os_EROFS   = EROFS;

/* Resources */
const int __public__os_ENOMEM = ENOMEM;
const int __public__os_ENOSPC = ENOSPC;
const int __public__os_EMFILE = EMFILE;
const int __public__os_ENFILE = ENFILE;
const int __public__os_EBUSY  = EBUSY;

/* Links / filesystem */
const int __public__os_ELOOP  = ELOOP;
const int __public__os_EXDEV  = EXDEV;


/* Signals */
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

/* Resource Limits */
const int __public__os_RLIMIT_CPU    = RLIMIT_CPU;
const int __public__os_RLIMIT_FSIZE  = RLIMIT_FSIZE;
const int __public__os_RLIMIT_DATA   = RLIMIT_DATA;
const int __public__os_RLIMIT_STACK  = RLIMIT_STACK;
const int __public__os_RLIMIT_CORE   = RLIMIT_CORE;
const int __public__os_RLIMIT_NOFILE = RLIMIT_NOFILE;
const int __public__os_RLIMIT_AS     = RLIMIT_AS;

/* Standard file descriptor numbers */
const int __public__os_STDIN_FD  = STDIN_FILENO;
const int __public__os_STDOUT_FD = STDOUT_FILENO;
const int __public__os_STDERR_FD = STDERR_FILENO;

/* open() flags */
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

/* seek constants */
const int __public__os_SEEK_SET = SEEK_SET;
const int __public__os_SEEK_CUR = SEEK_CUR;
const int __public__os_SEEK_END = SEEK_END;

/* fcntl commands */
const int __public__os_F_DUPFD         = F_DUPFD;
const int __public__os_F_DUPFD_CLOEXEC = F_DUPFD_CLOEXEC;
const int __public__os_F_GETFD         = F_GETFD;
const int __public__os_F_SETFD         = F_SETFD;
const int __public__os_F_GETFL         = F_GETFL;
const int __public__os_F_SETFL         = F_SETFL;

/* FD flags */
const int __public__os_FD_CLOEXEC = FD_CLOEXEC;

/* stat mode bits */
const int __public__os_S_IFREG  = S_IFREG;
const int __public__os_S_IFDIR  = S_IFDIR;
const int __public__os_S_IFCHR  = S_IFCHR;
const int __public__os_S_IFBLK  = S_IFBLK;
const int __public__os_S_IFIFO  = S_IFIFO;
const int __public__os_S_IFLNK  = S_IFLNK;
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


/* Special directory FDs */
#ifndef AT_FDCWD
#define AT_FDCWD -2
#endif
const int __public__os_AT_FDCWD = AT_FDCWD;

const int __public__os_AT_REMOVEDIR      = AT_REMOVEDIR;
const int __public__os_AT_SYMLINK_FOLLOW = AT_SYMLINK_FOLLOW;

/* Rename flags (macOS uses RENAME_EXCL / RENAME_SWAP via renameatx_np) */
const int __public__os_RENAME_NOREPLACE = 0x00000001; // RENAME_EXCL
const int __public__os_RENAME_EXCHANGE  = 0x00000002; // RENAME_SWAP

/* Access mode flags */
const int __public__os_F_OK = F_OK;
const int __public__os_R_OK = R_OK;
const int __public__os_W_OK = W_OK;
const int __public__os_X_OK = X_OK;

/* Directory entry types */
const int __public__os_DT_UNKNOWN = DT_UNKNOWN;
const int __public__os_DT_FIFO    = DT_FIFO;
const int __public__os_DT_CHR     = DT_CHR;
const int __public__os_DT_DIR     = DT_DIR;
const int __public__os_DT_BLK     = DT_BLK;
const int __public__os_DT_REG     = DT_REG;
const int __public__os_DT_LNK     = DT_LNK;
const int __public__os_DT_SOCK    = DT_SOCK;
const int __public__os_DT_WHT     = DT_WHT;

/* Memory protection flags */
const int __public__os_PROT_NONE  = PROT_NONE;
const int __public__os_PROT_READ  = PROT_READ;
const int __public__os_PROT_WRITE = PROT_WRITE;
const int __public__os_PROT_EXEC  = PROT_EXEC;

/* mmap flags */
const int __public__os_MAP_SHARED    = MAP_SHARED;
const int __public__os_MAP_PRIVATE   = MAP_PRIVATE;
const int __public__os_MAP_FIXED     = MAP_FIXED;
const int __public__os_MAP_ANONYMOUS = MAP_ANON; // macOS uses MAP_ANON
const int __public__os_MAP_NORESERVE = 0x0040;   /* macOS MAP_NORESERVE */

/* madvise advice */
const int __public__os_MADV_NORMAL     = MADV_NORMAL;
const int __public__os_MADV_RANDOM     = MADV_RANDOM;
const int __public__os_MADV_SEQUENTIAL = MADV_SEQUENTIAL;
const int __public__os_MADV_WILLNEED   = MADV_WILLNEED;
const int __public__os_MADV_DONTNEED   = MADV_DONTNEED;
const int __public__os_MADV_FREE       = MADV_FREE;

/* Function Prototypes */
/**
 * @brief Get current errno value.
 * @return errno.
 */
int __public__os_errno(void);

/**
 * @brief Get current process ID.
 * @return PID.
 */
int __public__os_getpid(void);

/**
 * @brief Get parent process ID.
 * @return Parent PID.
 */
int __public__os_getppid(void);

/**
 * @brief Get current thread ID.
 * @note macOS implementation uses pthread_threadid_np().
 * @return Thread ID.
 */
uint64_t __public__os_gettid(void);

/**
 * @brief Terminate the current process.
 * @param code Exit status.
 */
void __public__os_exit(int code);

/**
 * @brief Create a child process.
 * @return 0 in child, child PID in parent, -1 on error.
 */
int __public__os_fork(void);

/**
 * @brief Wait for a child process.
 * @param pid     Process ID.
 * @param status  Exit status.
 * @param options Wait options.
 * @return PID or -1 on error.
 */
typedef struct __public__os_waitpid_rt {
    int64_t pid;
    int64_t status;
} __public__os_waitpid_rt_t;
__public__os_waitpid_rt_t __public__os_waitpid(int pid, int options);

/**
 * @brief Return type for os_fstat
 */
typedef struct __public__os_fstat_rt {
    int64_t result;      // 0 on success, -1 on error
    int64_t dev;         // Device ID
    int64_t ino;         // Inode number
    int64_t mode;        // File mode
    int64_t nlink;       // Number of hard links
    int64_t uid;         // User ID
    int64_t gid;         // Group ID
    int64_t rdev;        // Device ID (if special file)
    int64_t size;        // Total size in bytes
    int64_t blksize;     // Block size for filesystem I/O
    int64_t blocks;      // Number of 512B blocks allocated
    int64_t atime;       // Access time (seconds)
    int64_t mtime;       // Modification time (seconds)
    int64_t ctime;       // Status change time (seconds)
} __public__os_fstat_rt_t;

/**
 * @brief Return type for os_stat and os_lstat
 */
typedef struct __public__os_stat_rt {
    int64_t result;      // 0 on success, -1 on error
    int64_t dev;         // Device ID
    int64_t ino;         // Inode number
    int64_t mode;        // File mode
    int64_t nlink;       // Number of hard links
    int64_t uid;         // User ID
    int64_t gid;         // Group ID
    int64_t rdev;        // Device ID (if special file)
    int64_t size;        // Total size in bytes
    int64_t blksize;     // Block size for filesystem I/O
    int64_t blocks;      // Number of 512B blocks allocated
    int64_t atime;       // Access time (seconds)
    int64_t mtime;       // Modification time (seconds)
    int64_t ctime;       // Status change time (seconds)
} __public__os_stat_rt_t;

/**
 * @brief Return type for os_getrlimit
 */
typedef struct __public__os_getrlimit_rt {
    int64_t result;      // 0 on success, -1 on error
    int64_t cur;         // Soft limit
    int64_t max;         // Hard limit (ceiling for cur)
} __public__os_getrlimit_rt_t;

/**
 * @brief Send a signal to a process.
 * @param pid Process ID.
 * @param sig Signal number.
 * @return 0 on success, -1 on error.
 */
int __public__os_kill(int pid, int sig);

/**
 * @brief Execute a program.
 * @param path Executable path.
 * @param argv Argument vector.
 * @param envp Environment.
 * @return -1 on error.
 */
int __public__os_execve(__public__string_t *path, __public__array_t* argv, __public__array_t* envp);

/**
 * @brief Execute a program using PATH lookup.
 * @param file Executable name.
 * @param argv Argument vector.
 * @return -1 on error.
 */
int __public__os_execvp(__public__string_t *file, __public__array_t* argv);

/**
 * @brief Get environment variable array.
 * @return Environment pointer.
 */
char **__public__os_environ(void);

/**
 * @brief Get environment variable value.
 * @param key Variable name.
 * @return Value or NULL.
 */
const char *__public__os_getenv(__public__string_t *key);

/**
 * @brief Set environment variable.
 * @param key       Variable name.
 * @param value     Value.
 * @param overwrite Overwrite if exists.
 * @return 0 on success, -1 on error.
 */
int __public__os_setenv(__public__string_t *key, __public__string_t *value, int overwrite);

/**
 * @brief Remove environment variable.
 * @param key Variable name.
 * @return 0 on success, -1 on error.
 */
int __public__os_unsetenv(__public__string_t *key);

/**
 * @brief Get current working directory.
 * @param buf  Output buffer.
 * @param size Buffer size.
 * @return 0 on success, -1 on error.
 */
int __public__os_getcwd(char *buf, int64_t size);

/**
 * @brief Change working directory.
 * @param path Directory path.
 * @return 0 on success, -1 on error.
 */
int __public__os_chdir(__public__string_t *path);

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
 * @brief Get user ID.
 * @return UID.
 */
int __public__os_getuid(void);

/**
 * @brief Get effective user ID.
 * @return Effective UID.
 */
int __public__os_geteuid(void);

/**
 * @brief Get group ID.
 * @return GID.
 */
int __public__os_getgid(void);

/**
 * @brief Get effective group ID.
 * @return Effective GID.
 */
int __public__os_getegid(void);

/**
 * @brief Set user ID.
 * @param uid User ID.
 * @return 0 on success, -1 on error.
 */
int __public__os_setuid(int uid);

/**
 * @brief Set group ID.
 * @param gid Group ID.
 * @return 0 on success, -1 on error.
 */
int __public__os_setgid(int gid);

/**
 * @brief Open a file.
 * @param path  File path.
 * @param flags Open flags.
 * @param mode  File mode.
 * @return File descriptor or -1.
 */
int __public__os_open(__public__string_t *path, int flags, int mode);

/**
 * @brief Close a file descriptor.
 * @param fd File descriptor.
 * @return 0 on success, -1 on error.
 */
int __public__os_close(int fd);

/**
 * @brief Read from a file descriptor.
 * @param fd  File descriptor.
 * @param buf Buffer.
 * @param n   Bytes to read.
 * @return Bytes read or -1.
 */
int64_t __public__os_read(int fd, void *buf, int64_t n);

/**
 * @brief Write to a file descriptor.
 * @param fd  File descriptor.
 * @param buf Buffer.
 * @param n   Bytes to write.
 * @return Bytes written or -1.
 */
int64_t __public__os_write(int fd, const void *buf, int64_t n);

/**
 * @brief Reposition file offset.
 * @param fd     File descriptor.
 * @param offset Offset.
 * @param whence Seek mode.
 * @return New offset or -1.
 */
int64_t __public__os_lseek(int fd, int64_t offset, int whence);

/**
 * @brief Get file status.
 * @param fd File descriptor.
 * @return Stat structure with result.
 */
__public__os_fstat_rt_t __public__os_fstat(int fd);

/**
 * @brief Duplicate a file descriptor.
 * @param fd File descriptor.
 * @return New FD or -1.
 */
int __public__os_dup(int fd);

/**
 * @brief Duplicate a file descriptor to a specific value.
 *
 * @param oldfd Existing FD.
 * @param newfd Target FD.
 *
 * @return New FD or -1 on error.
 */
int __public__os_dup2(int oldfd, int newfd);

/**
 * @brief Control file descriptor behavior.
 *
 * @param fd  File descriptor.
 * @param cmd Command (OS_F_*).
 * @param arg Command-specific argument.
 *
 * @return Result or -1 on error.
 */
int64_t __public__os_fcntl(int fd, int cmd, int64_t arg);

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
void *__public__os_mmap(void *addr, int64_t len, int prot, int flags, int fd, int64_t off);

/**
 * @brief Unmap memory.
 * @param addr Address.
 * @param len  Length.
 * @return 0 on success, -1 on error.
 */
int __public__os_munmap(void *addr, int64_t len);

/**
 * @brief Get page size.
 * @return Page size in bytes.
 */
int64_t __public__os_page_size(void);

/**
 * @brief Change memory protection.
 * @param addr Address.
 * @param len  Length.
 * @param prot Protection flags.
 * @return 0 on success, -1 on error.
 */
int __public__os_mprotect(void *addr, int64_t len, int prot);

/**
 * @brief Give advice about memory usage.
 * @param addr   Address.
 * @param len    Length.
 * @param advice Advice flags.
 * @return 0 on success, -1 on error.
 */
int __public__os_madvise(void *addr, int64_t len, int advice);

/**
 * @brief Wait on a futex.
 * @param uaddr   Address of futex.
 * @param val     Expected value.
 * @param timeout Timeout (NULL for infinite).
 * @return 0 on success, -1 on error.
 */
int __public__os_futex_wait(int *uaddr, int val, const void *timeout);

/**
 * @brief Wake one waiter on a futex.
 * @param uaddr Address of futex.
 * @return 0 on success, -1 on error.
 */
int __public__os_futex_wake_one(int *uaddr);

/**
 * @brief Wake all waiters on a futex.
 * @param uaddr Address of futex.
 * @return 0 on success, -1 on error.
 */
int __public__os_futex_wake_all(int *uaddr);

/**
 * @brief Install a signal handler.
 * @param sig     Signal number.
 * @param handler Handler function.
 * @return 0 on success, -1 on error.
 */
int __public__os_signal_install(int sig, void (*handler)(int));

/**
 * @brief Read directory entries.
 * @param fd   Directory file descriptor.
 * @param buf  Buffer for entries.
 * @param size Buffer size.
 * @return Bytes read or -1 on error.
 */
int __public__os_getdents64(int fd, void *buf, int64_t size);

/**
 * @brief Rename with flags (macOS renameatx_np wrapper).
 * @param oldpath Old path.
 * @param newpath New path.
 * @param flags   Rename flags.
 * @return 0 on success, -1 on error.
 */
int __public__os_renameat2(__public__string_t *oldpath, __public__string_t *newpath, int flags);

/**
 * @brief Set process group ID.
 * @param pid  Process ID.
 * @param pgid Process group ID.
 * @return 0 on success, -1 on error.
 */
int __public__os_setpgid(int pid, int pgid);

/**
 * @brief Get process group ID.
 * @param pid Process ID.
 * @return Process group ID or -1 on error.
 */
int __public__os_getpgid(int pid);

/**
 * @brief Get process group ID of calling process.
 * @return Process group ID.
 */
int __public__os_getpgrp(void);

/**
 * @brief Create a new session.
 * @return Session ID or -1 on error.
 */
int __public__os_setsid(void);

/**
 * @brief Get resource limits.
 * @param resource Resource type.
 * @return Resource limit structure with result.
 */
__public__os_getrlimit_rt_t __public__os_getrlimit(int resource);

/**
 * @brief Set resource limits.
 * @param resource Resource type.
 * @param cur      Soft limit.
 * @param max      Hard limit.
 * @return 0 on success, -1 on error.
 */
int __public__os_setrlimit(int resource, int64_t cur, int64_t max);

/**
 * @brief Create a directory.
 * @param path Directory path.
 * @param mode Permissions.
 * @return 0 on success, -1 on error.
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
 * @brief Remove a directory.
 * @param path Directory path.
 * @return 0 on success, -1 on error.
 */
int __public__os_rmdir(__public__string_t *path);

/**
 * @brief Delete a file.
 * @param path File path.
 * @return 0 on success, -1 on error.
 */
int __public__os_unlink(__public__string_t *path);

/**
 * @brief Rename a file or directory.
 * @param oldpath Old path.
 * @param newpath New path.
 * @return 0 on success, -1 on error.
 */
int __public__os_rename(__public__string_t *oldpath, __public__string_t *newpath);

/**
 * @brief Create a hard link.
 * @param oldpath Existing path.
 * @param newpath Link path.
 * @return 0 on success, -1 on error.
 */
int __public__os_link(__public__string_t *oldpath, __public__string_t *newpath) ;

/**
 * @brief Create a symbolic link.
 * @param target   Target path.
 * @param linkpath Link path.
 * @return 0 on success, -1 on error.
 */
int __public__os_symlink(__public__string_t *target, __public__string_t *linkpath);

/**
 * @brief Read symbolic link contents.
 * @param path Symbolic link path.
 * @param buf  Output buffer.
 * @param size Buffer size.
 * @return Bytes read or -1 on error.
 */
int64_t __public__os_readlink(__public__string_t *path, char *buf, int64_t size);

/**
 * @brief Get file status.
 * @param path File path.
 * @return Stat structure with result.
 */
__public__os_stat_rt_t __public__os_stat(__public__string_t *path);

/**
 * @brief Get file status (don't follow symlinks).
 * @param path File path.
 * @return Stat structure with result.
 */
__public__os_stat_rt_t __public__os_lstat(__public__string_t *path);

/**
 * @brief Check file accessibility.
 * @param path File path.
 * @param mode Access mode.
 * @return 0 on success, -1 on error.
 */
int __public__os_access(__public__string_t *path, int mode);

#endif