/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_IO_IO_H
#define GITMIND_IO_IO_H

#include <gitmind/result.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>

/* Result types for I/O operations */
GM_RESULT_DEF(gm_result_file_ptr, FILE *);
/* gm_result_size_t already defined in result.h */
/* gm_result_int_t already defined in result.h */
GM_RESULT_DEF(gm_result_string, char *);
GM_RESULT_DEF(gm_result_pid, pid_t);
GM_RESULT_DEF(gm_result_ssize, ssize_t);

/**
 * File operations interface with Result types.
 * All operations return Result types for proper error handling.
 */
typedef struct gm_file_ops {
    gm_result_file_ptr_t (*fopen)(const char *path, const char *mode);
    gm_result_void_t (*fclose)(FILE *stream);
    gm_result_size_t (*fread)(void *ptr, size_t size, size_t count, FILE *stream);
    gm_result_size_t (*fwrite)(const void *ptr, size_t size, size_t count, FILE *stream);
    gm_result_int_t (*fprintf)(FILE *stream, const char *format, ...);
    gm_result_void_t (*fflush)(FILE *stream);
    gm_result_void_t (*remove)(const char *path);
    gm_result_void_t (*rename)(const char *oldpath, const char *newpath);
} gm_file_ops_t;

/**
 * Directory operations interface with Result types.
 */
typedef struct gm_dir_ops {
    gm_result_void_t (*mkdir)(const char *path, mode_t mode);
    gm_result_void_t (*rmdir)(const char *path);
    gm_result_void_t (*chdir)(const char *path);
    gm_result_string_t (*getcwd)(char *buf, size_t size);
} gm_dir_ops_t;

/**
 * File system operations interface with Result types.
 */
typedef struct gm_fs_ops {
    gm_result_void_t (*stat)(const char *path, struct stat *buf);
    gm_result_void_t (*lstat)(const char *path, struct stat *buf);
    gm_result_void_t (*access)(const char *path, int mode);
    gm_result_void_t (*chmod)(const char *path, mode_t mode);
    gm_result_void_t (*unlink)(const char *path);
    gm_result_ssize_t (*readlink)(const char *path, char *buf, size_t bufsiz);
    gm_result_void_t (*symlink)(const char *oldpath, const char *newpath);
} gm_fs_ops_t;

/**
 * Process operations interface with Result types.
 */
typedef struct gm_process_ops {
    gm_result_int_t (*system)(const char *command);
    gm_result_pid_t (*fork)(void);
    gm_result_void_t (*execvp)(const char *file, char *const argv[]);
    gm_result_pid_t (*waitpid)(pid_t pid, int *status, int options);
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
 * @return Pointer to default I/O operations structure
 */
const gm_io_ops_t *gm_io_ops_default(void);

#endif /* GITMIND_IO_IO_H */
