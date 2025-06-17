/* SPDX-License-Identifier: Apache-2.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* List context for callback */
typedef struct {
    const char *filter_path;
    int count;
    int show_all;
    int show_augments;
} list_ctx_t;

/* Edge callback for listing */
static int list_edge_callback(const gm_edge_t *edge, void *userdata) {
    list_ctx_t *lctx = (list_ctx_t *)userdata;
    
    /* Apply filter if specified */
    if (lctx->filter_path) {
        if (strcmp(edge->src_path, lctx->filter_path) != 0 &&
            strcmp(edge->tgt_path, lctx->filter_path) != 0) {
            return 0;  /* Skip this edge */
        }
    }
    
    /* Skip AUGMENTS edges unless --show-augments */
    if (!lctx->show_augments && edge->rel_type == GM_REL_AUGMENTS) {
        return 0;  /* Skip augments edges by default */
    }
    
    /* Format and print edge */
    char formatted[512];
    gm_edge_format(edge, formatted, sizeof(formatted));
    printf("%s\n", formatted);
    
    lctx->count++;
    return 0;  /* Continue iteration */
}

/* List command implementation */
int gm_cmd_list(gm_context_t *ctx, int argc, char **argv) {
    list_ctx_t lctx = {0};
    const char *branch = NULL;
    int result;
    
    /* Parse arguments */
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--all") == 0) {
            lctx.show_all = 1;
        } else if (strcmp(argv[i], "--show-augments") == 0) {
            lctx.show_augments = 1;
        } else if (strcmp(argv[i], "--branch") == 0 && i + 1 < argc) {
            branch = argv[++i];
        } else if (!lctx.filter_path) {
            lctx.filter_path = argv[i];
        }
    }
    
    /* Read journal */
    result = gm_journal_read(ctx, branch, list_edge_callback, &lctx);
    if (result == GM_NOT_FOUND) {
        /* Don't print here, let the summary handle it */
    } else if (result != GM_OK) {
        fprintf(stderr, "Error: Failed to read links\n");
        return result;
    }
    
    /* Print summary */
    if (lctx.count == 0) {
        if (lctx.filter_path) {
            printf("No links found for: %s\n", lctx.filter_path);
        } else {
            printf("No links found\n");
        }
    } else {
        printf("\nTotal: %d link%s\n", lctx.count, lctx.count == 1 ? "" : "s");
    }
    
    return GM_OK;
}