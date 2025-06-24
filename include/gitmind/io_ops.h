/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_IO_OPS_H
#define GITMIND_IO_OPS_H

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

/**
 * File operations interface for dependency injection.
 * This allows for test doubles and better testability.
 */
typedef struct gm_file_ops {
    FILE *(*fopen)(const char *path, const char *mode);
    int (*fclose)(FILE *stream);
    size_t (*fread)(void *ptr, size_t size, size_t count, FILE *stream);
    size_t (*fwrite)(const void *ptr, size_t size, size_t count, FILE *stream);
    int (*fprintf)(FILE *stream, const char *format, ...);
    int (*fflush)(FILE *stream);
    int (*remove)(const char *path);
    int (*rename)(const char *oldpath, const char *newpath);
} gm_file_ops_t;

/**
 * Directory operations interface for dependency injection.
 */
typedef struct gm_dir_ops {
    int (*mkdir)(const char *path, mode_t mode);
    int (*rmdir)(const char *path);
    int (*chdir)(const char *path);
    char *(*getcwd)(char *buf, size_t size);
} gm_dir_ops_t;

/**
 * File system operations interface for dependency injection.
 */
typedef struct gm_fs_ops {
    int (*stat)(const char *path, struct stat *buf);
    int (*lstat)(const char *path, struct stat *buf);
    int (*access)(const char *path, int mode);
    int (*chmod)(const char *path, mode_t mode);
    int (*unlink)(const char *path);
    ssize_t (*readlink)(const char *path, char *buf, size_t bufsiz);
    int (*symlink)(const char *oldpath, const char *newpath);
} gm_fs_ops_t;

/**
 * Process operations interface for dependency injection.
 */
typedef struct gm_process_ops {
    int (*system)(const char *command);
    pid_t (*fork)(void);
    int (*execvp)(const char *file, char *const argv[]);
    pid_t (*waitpid)(pid_t pid, int *status, int options);
} gm_process_ops_t;

/**
 * Combined I/O operations structure.
 */
typedef struct gm_io_ops {
    const gm_file_ops_t *file;
    const gm_dir_ops_t *dir;
    const gm_fs_ops_t *fs;
    const gm_process_ops_t *process;
} gm_io_ops_t;

/**
 * Get default I/O operations (uses real system calls).
 */
const gm_io_ops_t *gm_io_ops_default(void);

#endif /* GITMIND_IO_OPS_H */