/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#define _DEFAULT_SOURCE
#define _GNU_SOURCE

#include "gitmind/io_ops.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

/* Default file operations - use real system calls */
static const gm_file_ops_t default_file_ops = {.fopen = fopen,
                                               .fclose = fclose,
                                               .fread = fread,
                                               .fwrite = fwrite,
                                               .fprintf = fprintf,
                                               .fflush = fflush,
                                               .remove = remove,
                                               .rename = rename};

/* Default directory operations - use real system calls */
static const gm_dir_ops_t default_dir_ops = {
    .mkdir = mkdir, .rmdir = rmdir, .chdir = chdir, .getcwd = getcwd};

/* Default file system operations - use real system calls */
static const gm_fs_ops_t default_fs_ops = {.stat = stat,
                                           .lstat = lstat,
                                           .access = access,
                                           .chmod = chmod,
                                           .unlink = unlink,
                                           .readlink = readlink,
                                           .symlink = symlink};

/* Default process operations - use real system calls */
static const gm_process_ops_t default_process_ops = {
    .system = system, .fork = fork, .execvp = execvp, .waitpid = waitpid};

/* Combined default I/O operations */
static const gm_io_ops_t default_io_ops = {.file = &default_file_ops,
                                           .dir = &default_dir_ops,
                                           .fs = &default_fs_ops,
                                           .process = &default_process_ops};

const gm_io_ops_t *gm_io_ops_default(void) {
    return &default_io_ops;
}