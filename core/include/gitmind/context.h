/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_CONTEXT_H
#define GITMIND_CONTEXT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
/* Keep time ops local here to preserve stable test interfaces */

#include "gitmind/ports/fs_temp_port.h"
#include "gitmind/ports/git_repository_port.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file context.h
 * @brief Context structure for dependency injection
 */

/* Time operations for testing */
typedef struct gm_time_ops {
    time_t (*time)(time_t *tloc);
    int (*clock_gettime)(int clk_id, struct timespec *timespec);
} gm_time_ops_t;

/* Context for dependency injection */
typedef struct gm_context {
    /* Git operations */
    struct {
        int (*resolve_blob)(void *repo, const char *path, uint8_t *sha);
        int (*create_commit)(void *repo, const char *ref, const void *data,
                             size_t len);
        int (*read_commits)(void *repo, const char *ref, void *callback,
                            void *userdata);
    } git_ops;
    
    /* Time operations for dependency injection */
    const gm_time_ops_t *time_ops;

    /* User data */
    void *git_repo;
    void *user_data;

    /* Ports */
    gm_fs_temp_port_t fs_temp_port;
    void (*fs_temp_port_dispose)(gm_fs_temp_port_t *port);

    gm_git_repository_port_t git_repo_port;
    void (*git_repo_port_dispose)(gm_git_repository_port_t *port);
} gm_context_t;

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_CONTEXT_H */
