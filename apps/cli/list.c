/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/output.h"
#include "gitmind/context.h"
#include "gitmind/edge.h"
#include "gitmind/edge_attributed.h"
#include "gitmind/journal.h"
#include "gitmind/error.h"
#include "../../include/gitmind/constants.h"
#include "cli_runtime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* List context for callback */
typedef struct {
    const char *filter_path;
    int count;
    int show_all;
    int show_augments;
    int show_attribution;
} list_ctx_t;

/* Legacy edge callback for listing */
static int list_edge_callback(const gm_edge_t *edge, void *userdata) {
    list_ctx_t *lctx = (list_ctx_t *)userdata;

    /* Apply filter if specified */
    if (lctx->filter_path) {
        if (strcmp(edge->src_path, lctx->filter_path) != 0 &&
            strcmp(edge->tgt_path, lctx->filter_path) != 0) {
            return 0; /* Continue */
        }
    }

    /* Skip AUGMENTS edges unless --show-augments */
    if (!lctx->show_augments && edge->rel_type == GM_REL_AUGMENTS) {
        return 0; /* Skip augments edges by default */
    }

    /* Format and print edge */
    char formatted[GM_FORMAT_BUFFER_SIZE];
    gm_edge_format(edge, formatted, sizeof(formatted));
    printf("%s\n", formatted);

    lctx->count++;
    return 0; /* Continue */
}

/* Attributed edge callback for listing */
static int list_attributed_edge_callback(const gm_edge_attributed_t *edge,
                                         void *userdata) {
    list_ctx_t *lctx = (list_ctx_t *)userdata;

    /* Apply path filter if specified */
    if (lctx->filter_path) {
        if (strcmp(edge->src_path, lctx->filter_path) != 0 &&
            strcmp(edge->tgt_path, lctx->filter_path) != 0) {
            return 0; /* Continue */
        }
    }

    /* Skip AUGMENTS edges unless --show-augments */
    if (!lctx->show_augments && edge->rel_type == GM_REL_AUGMENTS) {
        return 0; /* Skip augments edges by default */
    }

    /* Format and print edge */
    char formatted[GM_FORMAT_BUFFER_SIZE];
    if (lctx->show_attribution ||
        edge->attribution.source_type != GM_SOURCE_HUMAN) {
        gm_edge_attributed_format_with_attribution(edge, formatted,
                                                   sizeof(formatted));
    } else {
        gm_edge_attributed_format(edge, formatted, sizeof(formatted));
    }
    printf("%s\n", formatted);

    lctx->count++;
    return 0; /* Continue */
}

/* Parse list command arguments */
static void parse_list_arguments(int argc, char **argv, list_ctx_t *lctx,
                                 const char **branch,
                                 const char **source_filter,
                                 const char **min_conf_str, int *use_filter) {
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], GM_FLAG_VERBOSE) == 0) {
            lctx->show_all = 1;
        } else if (strcmp(argv[i], GM_FLAG_SHOW_AUG) == 0) {
            lctx->show_augments = 1;
        } else if (strcmp(argv[i], GM_FLAG_BRANCH) == 0 && i + 1 < argc) {
            *branch = argv[++i];
        } else if (strcmp(argv[i], GM_FLAG_SOURCE) == 0 && i + 1 < argc) {
            *source_filter = argv[++i];
            *use_filter = 1;
        } else if (strcmp(argv[i], GM_FLAG_MIN_CONF) == 0 && i + 1 < argc) {
            *min_conf_str = argv[++i];
            *use_filter = 1;
        } else if (strcmp(argv[i], GM_FLAG_SHOW_ATTR) == 0) {
            lctx->show_attribution = 1;
        } else if (strcmp(argv[i], GM_FLAG_FROM) == 0 && i + 1 < argc) {
            lctx->filter_path = argv[++i];
        } else if (!lctx->filter_path && argv[i][0] != GM_OPTION_PREFIX) {
            /* Positional argument for path filter */
            lctx->filter_path = argv[i];
        }
    }
}

/* Set up filter based on arguments */
/* Filters are currently disabled in minimal CLI; placeholders retained */
static void setup_list_filter(const char *source_filter, const char *min_conf_str) {
    (void)source_filter;
    (void)min_conf_str;
}

/* Execute the list query */
static int execute_list_query(gm_context_t *ctx, const char *branch,
                              list_ctx_t *lctx, int use_filter) {
    /* Try to read attributed edges first, fall back to legacy if needed */
    int result = gm_journal_read_attributed(
        ctx, branch, list_attributed_edge_callback, lctx);

    if (result == GM_ERR_NOT_FOUND && !use_filter) {
        /* Fall back to legacy journal reader if no attribution filters */
        result = gm_journal_read(ctx, branch, list_edge_callback, lctx);
    }

    return result;
}

/* Format list output based on results */
static void format_list_output(const list_ctx_t *lctx,
                               const char *source_filter,
                               const char *min_conf_str, int use_filter) {
    if (lctx->count == 0) {
        if (lctx->filter_path) {
            printf(GM_MSG_NO_LINKS_PATH "\n", lctx->filter_path);
        } else if (use_filter) {
            printf(GM_MSG_NO_LINKS_FILTER "\n");
        } else {
            printf(GM_MSG_NO_LINKS "\n");
        }
    } else {
        const char *filter_desc = "";
        if (source_filter) {
            filter_desc = source_filter;
        } else if (min_conf_str) {
            filter_desc = GM_FILTER_DESC_CONF;
        }

        if (use_filter && strlen(filter_desc) > 0) {
            printf(GM_SUCCESS_FILTERED "\n", (size_t)lctx->count, filter_desc);
        } else {
            printf(GM_SUCCESS_TOTAL "\n", (size_t)lctx->count);
        }
    }
}

/* List command implementation with attribution support */
/* Forward declaration to satisfy -Wmissing-prototypes */
int gm_cmd_list(gm_context_t *ctx, gm_cli_ctx_t *cli, int argc, char **argv);

int gm_cmd_list(gm_context_t *ctx, gm_cli_ctx_t *cli, int argc, char **argv) {
    (void)cli; /* Not used; list outputs directly for now */
    list_ctx_t lctx = {0};
    const char *branch = NULL;
    const char *source_filter = NULL;
    const char *min_conf_str = NULL;
    int use_filter = 0;
    int result;

    /* Parse arguments */
    parse_list_arguments(argc, argv, &lctx, &branch, &source_filter,
                         &min_conf_str, &use_filter);

    /* Set up attribution filter if needed */
    if (use_filter) {
        setup_list_filter(source_filter, min_conf_str);
    }

    /* Execute the query */
    result = execute_list_query(ctx, branch, &lctx, use_filter);

    if (result == GM_ERR_NOT_FOUND) {
        /* Don't print here, let the summary handle it */
    } else if (result != GM_OK) {
        fprintf(stderr, GM_ERR_READ_LINKS "\n");
        return result;
    }

    /* Print summary */
    format_list_output(&lctx, source_filter, min_conf_str, use_filter);

    return GM_OK;
}
