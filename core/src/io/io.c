/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

/* Feature test macros - exempt from naming conventions */
/* NOLINTBEGIN(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,readability-identifier-naming) */
#define _DEFAULT_SOURCE
#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
/* NOLINTEND(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,readability-identifier-naming) */

#include <gitmind/io/io.h>
#include <gitmind/error.h>
#include <gitmind/result.h> /* for gm_result_void_t, gm_result_size_t, gm_result_int_t */

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h> /* for pid_t */
#include <sys/wait.h>
#include <unistd.h>

/* Error code constants */
static const int GM_ERROR_FILE_OPERATION = 1001;
static const int GM_ERROR_DIR_OPERATION = 2001;
static const int GM_ERROR_FS_OPERATION = 3001;
static const int GM_ERROR_PROCESS_OPERATION = 4001;

/* Wrapper functions for file operations with Result types */

static gm_result_file_ptr_t wrap_fopen(const char *path, const char *mode) {
    FILE *file = fopen(path, mode);
    if (!file) {
        return (gm_result_file_ptr_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_FILE_OPERATION, "Failed to open file: %s", strerror(errno))
        };
    }
    return (gm_result_file_ptr_t){.ok = true, .u.val = file};
}

static gm_result_void_t wrap_fclose(FILE *stream) {
    if (fclose(stream) != 0) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_FILE_OPERATION, "Failed to close file: %s", strerror(errno))
        };
    }
    return (gm_result_void_t){.ok = true};
}

static gm_result_size_t wrap_fread(void *ptr, size_t size, size_t count, FILE *stream) {
    size_t result = fread(ptr, size, count, stream);
    if (result < count && ferror(stream)) {
        return (gm_result_size_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_FILE_OPERATION, "Failed to read file: %s", strerror(errno))
        };
    }
    return (gm_result_size_t){.ok = true, .u.val = result};
}

static gm_result_size_t wrap_fwrite(const void *ptr, size_t size, size_t count, FILE *stream) {
    size_t result = fwrite(ptr, size, count, stream);
    if (result < count) {
        return (gm_result_size_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_FILE_OPERATION, "Failed to write file: %s", strerror(errno))
        };
    }
    return (gm_result_size_t){.ok = true, .u.val = result};
}

static gm_result_int_t wrap_fprintf(FILE *stream, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = vfprintf(stream, format, args);
    va_end(args);
    
    if (result < 0) {
        return (gm_result_int_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_FILE_OPERATION, "Failed to write formatted output: %s", strerror(errno))
        };
    }
    return (gm_result_int_t){.ok = true, .u.val = result};
}

static gm_result_void_t wrap_fflush(FILE *stream) {
    if (fflush(stream) != 0) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_FILE_OPERATION, "Failed to flush stream: %s", strerror(errno))
        };
    }
    return (gm_result_void_t){.ok = true};
}

static gm_result_void_t wrap_remove(const char *path) {
    if (remove(path) != 0) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_FILE_OPERATION, "Failed to remove file: %s", strerror(errno))
        };
    }
    return (gm_result_void_t){.ok = true};
}

static gm_result_void_t wrap_rename(const char *oldpath, const char *newpath) {
    if (rename(oldpath, newpath) != 0) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_FILE_OPERATION, "Failed to rename file: %s", strerror(errno))
        };
    }
    return (gm_result_void_t){.ok = true};
}

/* Wrapper functions for directory operations with Result types */

static gm_result_void_t wrap_mkdir(const char *path, mode_t mode) {
    if (mkdir(path, mode) != 0) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_DIR_OPERATION, "Failed to create directory: %s", strerror(errno))
        };
    }
    return (gm_result_void_t){.ok = true};
}

static gm_result_void_t wrap_rmdir(const char *path) {
    if (rmdir(path) != 0) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_DIR_OPERATION, "Failed to remove directory: %s", strerror(errno))
        };
    }
    return (gm_result_void_t){.ok = true};
}

static gm_result_void_t wrap_chdir(const char *path) {
    if (chdir(path) != 0) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_DIR_OPERATION, "Failed to change directory: %s", strerror(errno))
        };
    }
    return (gm_result_void_t){.ok = true};
}

static gm_result_string_t wrap_getcwd(char *buf, size_t size) {
    char *result = getcwd(buf, size);
    if (!result) {
        return (gm_result_string_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_DIR_OPERATION, "Failed to get current directory: %s", strerror(errno))
        };
    }
    return (gm_result_string_t){.ok = true, .u.val = result};
}

/* Wrapper functions for file system operations with Result types */

static gm_result_void_t wrap_stat(const char *path, struct stat *buf) {
    if (stat(path, buf) != 0) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_FS_OPERATION, "Failed to stat file: %s", strerror(errno))
        };
    }
    return (gm_result_void_t){.ok = true};
}

static gm_result_void_t wrap_lstat(const char *path, struct stat *buf) {
    if (lstat(path, buf) != 0) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_FS_OPERATION, "Failed to lstat file: %s", strerror(errno))
        };
    }
    return (gm_result_void_t){.ok = true};
}

static gm_result_void_t wrap_access(const char *path, int mode) {
    if (access(path, mode) != 0) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_FS_OPERATION, "Access check failed: %s", strerror(errno))
        };
    }
    return (gm_result_void_t){.ok = true};
}

static gm_result_void_t wrap_chmod(const char *path, mode_t mode) {
    if (chmod(path, mode) != 0) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_FS_OPERATION, "Failed to change file mode: %s", strerror(errno))
        };
    }
    return (gm_result_void_t){.ok = true};
}

static gm_result_void_t wrap_unlink(const char *path) {
    if (unlink(path) != 0) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_FS_OPERATION, "Failed to unlink file: %s", strerror(errno))
        };
    }
    return (gm_result_void_t){.ok = true};
}

static gm_result_ssize_t wrap_readlink(const char *path, char *buf, size_t bufsiz) {
    ssize_t result = readlink(path, buf, bufsiz);
    if (result < 0) {
        return (gm_result_ssize_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_FS_OPERATION, "Failed to read link: %s", strerror(errno))
        };
    }
    return (gm_result_ssize_t){.ok = true, .u.val = result};
}

static gm_result_void_t wrap_symlink(const char *oldpath, const char *newpath) {
    if (symlink(oldpath, newpath) != 0) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_FS_OPERATION, "Failed to create symlink: %s", strerror(errno))
        };
    }
    return (gm_result_void_t){.ok = true};
}

/* Wrapper functions for process operations with Result types */

static gm_result_int_t wrap_system(const char *command) {
    /* NOLINTNEXTLINE(cert-env33-c) - system() usage is intentional for API compatibility */
    int result = system(command);
    if (result == -1) {
        return (gm_result_int_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_PROCESS_OPERATION, "Failed to execute command: %s", strerror(errno))
        };
    }
    return (gm_result_int_t){.ok = true, .u.val = result};
}

static gm_result_pid_t wrap_fork(void) {
    pid_t result = fork();
    if (result < 0) {
        return (gm_result_pid_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_PROCESS_OPERATION, "Failed to fork process: %s", strerror(errno))
        };
    }
    return (gm_result_pid_t){.ok = true, .u.val = result};
}

static gm_result_void_t wrap_execvp(const char *file, char *const argv[]) {
    execvp(file, argv);
    /* If we get here, execvp failed */
    return (gm_result_void_t){
        .ok = false,
        .u.err = GM_ERROR(GM_ERROR_PROCESS_OPERATION, "Failed to execute program: %s", strerror(errno))
    };
}

static gm_result_pid_t wrap_waitpid(pid_t pid, int *status, int options) {
    pid_t result = waitpid(pid, status, options);
    if (result < 0) {
        return (gm_result_pid_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_PROCESS_OPERATION, "Failed to wait for process: %s", strerror(errno))
        };
    }
    return (gm_result_pid_t){.ok = true, .u.val = result};
}

/* Static operation structures */

static const gm_file_ops_t GmDefaultFileOps = {
    .fopen = wrap_fopen,
    .fclose = wrap_fclose,
    .fread = wrap_fread,
    .fwrite = wrap_fwrite,
    .fprintf = wrap_fprintf,
    .fflush = wrap_fflush,
    .remove = wrap_remove,
    .rename = wrap_rename
};

static const gm_dir_ops_t GmDefaultDirOps = {
    .mkdir = wrap_mkdir,
    .rmdir = wrap_rmdir,
    .chdir = wrap_chdir,
    .getcwd = wrap_getcwd
};

static const gm_fs_ops_t GmDefaultFsOps = {
    .stat = wrap_stat,
    .lstat = wrap_lstat,
    .access = wrap_access,
    .chmod = wrap_chmod,
    .unlink = wrap_unlink,
    .readlink = wrap_readlink,
    .symlink = wrap_symlink
};

static const gm_process_ops_t GmDefaultProcessOps = {
    .system = wrap_system,
    .fork = wrap_fork,
    .execvp = wrap_execvp,
    .waitpid = wrap_waitpid
};

static const gm_io_ops_t GmDefaultIoOps = {
    .file = &GmDefaultFileOps,
    .dir = &GmDefaultDirOps,
    .fs = &GmDefaultFsOps,
    .process = &GmDefaultProcessOps
};

const gm_io_ops_t *gm_io_ops_default(void) {
    return &GmDefaultIoOps;
}
