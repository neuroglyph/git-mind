/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#define _GNU_SOURCE

#include "gitmind/output.h"
#include "gitmind/context.h"
#include "gitmind/edge_attributed.h"
#include "gitmind/journal.h"
#include "gitmind/error.h"
#include "../../include/gitmind/constants.h"
#include "cli_runtime.h"

#include "gitmind/constants_internal.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward declaration for command symbol to satisfy -Wmissing-prototypes */
int gm_cmd_link(gm_context_t *ctx, gm_cli_ctx_t *cli, int argc, char **argv);

/* Parse relationship type from string using constants */
static gm_rel_type_t parse_rel_type(const char *str) {
    if (strcasecmp(str, GM_STR_IMPLEMENTS) == 0) {
        return GM_REL_IMPLEMENTS;
    } else if (strcasecmp(str, GM_STR_REFERENCES) == 0) {
        return GM_REL_REFERENCES;
    } else if (strcasecmp(str, GM_STR_DEPENDS_ON) == 0 ||
               strcasecmp(str, GM_STR_DEPENDS_DASH) == 0) {
        return GM_REL_DEPENDS_ON;
    } else if (strcasecmp(str, GM_STR_AUGMENTS) == 0) {
        return GM_REL_AUGMENTS;
    }
    return GM_REL_CUSTOM;
}

/* Parse link command arguments */
static int parse_link_arguments(int argc, char **argv, const char **src_path,
                                const char **tgt_path, const char **type_str,
                                const char **confidence_str) {
    *type_str = GM_DEFAULT_REL_TYPE;

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], GM_FLAG_TYPE) == 0 && i + 1 < argc) {
            *type_str = argv[++i];
        } else if (strcmp(argv[i], GM_FLAG_CONFIDENCE) == 0 && i + 1 < argc) {
            *confidence_str = argv[++i];
        } else if (!*src_path) {
            *src_path = argv[i];
        } else if (!*tgt_path) {
            *tgt_path = argv[i];
        }
    }

    /* Validate required arguments */
    if (!*src_path || !*tgt_path) {
        return GM_ERR_INVALID_ARGUMENT;
    }

    return GM_OK;
}

/* Validate and process link inputs */
static int validate_link_inputs(gm_context_t *ctx, gm_cli_ctx_t *cli,
                                const char *type_str,
                                const char *confidence_str,
                                gm_rel_type_t *rel_type, uint16_t *confidence,
                                gm_attribution_t *attribution) {
    (void)ctx;
    int result;

    /* Parse relationship type */
    *rel_type = parse_rel_type(type_str);

    /* Parse confidence if provided */
    if (confidence_str) {
        gm_result_uint16_t c = gm_confidence_parse(confidence_str);
        if (!c.ok) {
            gm_output_error(cli->out, GM_ERR_INVALID_CONF "\n");
            return GM_ERR_INVALID_ARGUMENT;
        } else {
            *confidence = c.u.val;
        }
    }

    /* Get attribution from environment */
    result = gm_attribution_from_env(attribution);
    if (result != GM_OK) {
        gm_output_error(cli->out, GM_ERR_PARSE_ARGS "\n");
        return result;
    }

    /* For AI sources, use provided confidence or default AI confidence */
    if (attribution->source_type != GM_SOURCE_HUMAN && !confidence_str) {
        *confidence = GM_AI_DEFAULT_CONFIDENCE;
    }

    return GM_OK;
}

/* Create edge from arguments */
static int create_edge_from_args(gm_context_t *ctx, gm_cli_ctx_t *cli, const char *src_path,
                                 const char *tgt_path, gm_rel_type_t rel_type,
                                 uint16_t confidence,
                                 const gm_attribution_t *attribution,
                                 gm_edge_attributed_t *edge) {
    gm_result_edge_attributed_t r =
        gm_edge_attributed_create(ctx, src_path, tgt_path, rel_type, confidence,
                                  attribution, GM_LANE_PRIMARY);
    if (!r.ok) {
        char *msg = gm_error_format(r.u.err);
        gm_output_error(cli->out, GM_ERR_FORMAT "\n", msg ? msg : "unknown error");
        free(msg);
        gm_error_free(r.u.err);
        return GM_ERR_INVALID_ARGUMENT;
    }
    *edge = r.u.val;
    return GM_OK;
}

/* Save edge to journal */
static int save_edge_to_journal(gm_context_t *ctx, gm_cli_ctx_t *cli,
                                const gm_edge_attributed_t *edge) {
    int result = gm_journal_append_attributed(ctx, edge, GM_SINGLE_EDGE_COUNT);
    if (result != GM_OK) {
        gm_output_error(cli->out, GM_ERR_WRITE_FAILED "\n");
    }
    return result;
}

/* Print success message */
static void print_link_success(gm_context_t *ctx, gm_cli_ctx_t *cli,
                               const gm_edge_attributed_t *edge,
                               const gm_attribution_t *attribution) {
    (void)ctx;
    char formatted[GM_FORMAT_BUFFER_SIZE];

    if (gm_output_is_porcelain(cli->out)) {
        gm_output_porcelain(cli->out, PORCELAIN_KEY_STATUS,
                            PORCELAIN_STATUS_CREATED);
        gm_output_porcelain(cli->out, PORCELAIN_KEY_SOURCE, "%s",
                            edge->src_path);
        gm_output_porcelain(cli->out, PORCELAIN_KEY_TARGET, "%s",
                            edge->tgt_path);
        gm_output_porcelain(cli->out, PORCELAIN_KEY_TYPE, "%d",
                            edge->rel_type);
        gm_output_porcelain(cli->out, PORCELAIN_KEY_CONFIDENCE,
                            GM_FMT_CONFIDENCE,
                            (double)gm_confidence_from_half_float(edge->confidence));
        gm_output_porcelain(cli->out, PORCELAIN_KEY_ULID, "%s", edge->ulid);
    } else {
        if (attribution->source_type == GM_SOURCE_HUMAN) {
            gm_edge_attributed_format(edge, formatted, sizeof(formatted));
        } else {
            gm_edge_attributed_format_with_attribution(edge, formatted,
                                                       sizeof(formatted));
        }
        gm_output_print(cli->out, GM_SUCCESS_CREATED "\n", formatted);
    }
}

/* Link command implementation with attribution support */
int gm_cmd_link(gm_context_t *ctx, gm_cli_ctx_t *cli, int argc, char **argv) {
    const char *src_path = NULL;
    const char *tgt_path = NULL;
    const char *type_str = NULL;
    const char *confidence_str = NULL;
    gm_rel_type_t rel_type;
    uint16_t confidence = GM_DEFAULT_CONFIDENCE;
    gm_attribution_t attribution;
    gm_edge_attributed_t edge;
    int result;

    /* Parse arguments */
    result = parse_link_arguments(argc, argv, &src_path, &tgt_path, &type_str,
                                  &confidence_str);
    if (result != GM_OK) {
        gm_output_error(cli->out, GM_ERR_USAGE_LINK "\n");
        gm_output_error(cli->out, GM_ERR_TYPES_HELP "\n");
        return result;
    }

    /* Validate inputs */
    result = validate_link_inputs(ctx, cli, type_str, confidence_str, &rel_type,
                                  &confidence, &attribution);
    if (result != GM_OK) {
        return result;
    }

    /* Create edge */
    result = create_edge_from_args(ctx, cli, src_path, tgt_path, rel_type,
                                   confidence, &attribution, &edge);
    if (result != GM_OK) {
        return result;
    }

    /* Save to journal */
    result = save_edge_to_journal(ctx, cli, &edge);
    if (result != GM_OK) {
        return result;
    }

    /* Print success */
    print_link_success(ctx, cli, &edge, &attribution);

    return GM_OK;
}
