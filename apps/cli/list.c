/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/output.h"
#include "gitmind/context.h"
#include "gitmind/edge.h"
#include "gitmind/edge_attributed.h"
#include "gitmind/journal.h"
#include "gitmind/error.h"
#include "gitmind/constants.h"
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
    gm_output_t *output;
    int filter_ai_only;    /* 1 = AI only */
    float min_conf;        /* < 0 disables */
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
    if (gm_output_is_porcelain(lctx->output)) {
        gm_output_porcelain(lctx->output, PORCELAIN_KEY_SOURCE, "%s", edge->src_path);
        gm_output_porcelain(lctx->output, PORCELAIN_KEY_TARGET, "%s", edge->tgt_path);
        gm_output_porcelain(lctx->output, PORCELAIN_KEY_TYPE, "%d", edge->rel_type);
        gm_output_porcelain(lctx->output, PORCELAIN_KEY_CONFIDENCE, GM_FMT_CONFIDENCE,
                            (double)gm_confidence_from_half_float(edge->confidence));
        gm_output_porcelain(lctx->output, PORCELAIN_KEY_ULID, "%s", edge->ulid);
    } else {
        char formatted[GM_FORMAT_BUFFER_SIZE];
        gm_edge_format(edge, formatted, sizeof(formatted));
        gm_output_print(lctx->output, "%s\n", formatted);
    }

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

    /* Apply attribution filters */
    if (lctx->filter_ai_only && edge->attribution.source_type == GM_SOURCE_HUMAN) {
        return 0;
    }
    if (lctx->min_conf >= 0.0f) {
        if (gm_confidence_from_half_float(edge->confidence) < lctx->min_conf) {
            return 0;
        }
    }

    /* Format and print edge */
    if (gm_output_is_porcelain(lctx->output)) {
        gm_output_porcelain(lctx->output, PORCELAIN_KEY_SOURCE, "%s", edge->src_path);
        gm_output_porcelain(lctx->output, PORCELAIN_KEY_TARGET, "%s", edge->tgt_path);
        gm_output_porcelain(lctx->output, PORCELAIN_KEY_TYPE, "%d", edge->rel_type);
        gm_output_porcelain(lctx->output, PORCELAIN_KEY_CONFIDENCE, GM_FMT_CONFIDENCE,
                            (double)gm_confidence_from_half_float(edge->confidence));
        gm_output_porcelain(lctx->output, PORCELAIN_KEY_ULID, "%s", edge->ulid);
    } else {
        char formatted[GM_FORMAT_BUFFER_SIZE];
        if (lctx->show_attribution ||
            edge->attribution.source_type != GM_SOURCE_HUMAN) {
            gm_edge_attributed_format_with_attribution(edge, formatted,
                                                       sizeof(formatted));
        } else {
            gm_edge_attributed_format(edge, formatted, sizeof(formatted));
        }
        gm_output_print(lctx->output, "%s\n", formatted);
    }

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
static void setup_list_filter(list_ctx_t *lctx, const char *source_filter, const char *min_conf_str) {
    lctx->filter_ai_only = 0;
    lctx->min_conf = -1.0f;
    if (source_filter && strcmp(source_filter, GM_FILTER_VAL_AI) == 0) {
        lctx->filter_ai_only = 1;
    }
    if (min_conf_str && *min_conf_str) {
        char *endp = NULL;
        float v = strtof(min_conf_str, &endp);
        if (endp && *endp == '\0' && v >= 0.0f && v <= 1.0f) {
            lctx->min_conf = v;
        } else {
            gm_output_verbose(lctx->output, "Ignoring invalid --min-confidence value: %s\n", min_conf_str);
        }
    }
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
    if (gm_output_is_porcelain(lctx->output)) {
        gm_output_porcelain(lctx->output, PORCELAIN_KEY_STATUS, PORCELAIN_STATUS_SUCCESS);
        gm_output_porcelain(lctx->output, PORCELAIN_KEY_COUNT, "%d", lctx->count);
        return;
    }
    if (lctx->count == 0) {
        if (lctx->filter_path) {
            gm_output_print(lctx->output, GM_MSG_NO_LINKS_PATH "\n", lctx->filter_path);
        } else if (use_filter) {
            gm_output_print(lctx->output, GM_MSG_NO_LINKS_FILTER "\n");
        } else {
            gm_output_print(lctx->output, GM_MSG_NO_LINKS "\n");
        }
    } else {
        const char *filter_desc = "";
        if (source_filter) {
            filter_desc = source_filter;
        } else if (min_conf_str) {
            filter_desc = GM_FILTER_DESC_CONF;
        }

        if (use_filter && strlen(filter_desc) > 0) {
            gm_output_print(lctx->output, GM_SUCCESS_FILTERED "\n", (size_t)lctx->count, filter_desc);
        } else {
            gm_output_print(lctx->output, GM_SUCCESS_TOTAL "\n", (size_t)lctx->count);
        }
    }
}

/* List command implementation with attribution support */
/* Forward declaration to satisfy -Wmissing-prototypes */
int gm_cmd_list(gm_context_t *ctx, gm_cli_ctx_t *cli, int argc, char **argv);

int gm_cmd_list(gm_context_t *ctx, gm_cli_ctx_t *cli, int argc, char **argv) {
    list_ctx_t lctx = {0};
    lctx.output = cli->out;
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
        setup_list_filter(&lctx, source_filter, min_conf_str);
    }

    /* Execute the query */
    result = execute_list_query(ctx, branch, &lctx, use_filter);

    if (result == GM_ERR_NOT_FOUND) {
        /* Don't print here, let the summary handle it */
    } else if (result != GM_OK) {
        gm_output_error(cli->out, GM_ERR_READ_LINKS "\n");
        return result;
    }

    /* Print summary */
    format_list_output(&lctx, source_filter, min_conf_str, use_filter);

    return GM_OK;
}
