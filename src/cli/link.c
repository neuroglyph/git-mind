/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#define _GNU_SOURCE

#include "gitmind.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Parse relationship type from string using constants */
static gm_rel_type_t parse_rel_type(const char *str) {
    if (strcasecmp(str, GM_STR_IMPLEMENTS) == 0) {
        return GM_REL_IMPLEMENTS;
    } else if (strcasecmp(str, GM_STR_REFERENCES) == 0) {
        return GM_REL_REFERENCES;
    } else if (strcasecmp(str, GM_STR_DEPENDS_ON) == 0 || strcasecmp(str, GM_STR_DEPENDS_DASH) == 0) {
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
        fprintf(stderr, GM_ERR_USAGE_LINK "\n");
        fprintf(stderr, GM_ERR_TYPES_HELP "\n");
        return GM_INVALID_ARG;
    }
    
    return GM_OK;
}

/* Validate and process link inputs */
static int validate_link_inputs(const char *type_str, const char *confidence_str,
                               gm_rel_type_t *rel_type, uint16_t *confidence,
                               gm_attribution_t *attribution) {
    int result;
    
    /* Parse relationship type */
    *rel_type = parse_rel_type(type_str);
    
    /* Parse confidence if provided */
    if (confidence_str) {
        result = gm_confidence_parse(confidence_str, confidence);
        if (result != GM_OK) {
            fprintf(stderr, GM_ERR_INVALID_CONF "\n");
            return GM_INVALID_ARG;
        }
    }
    
    /* Get attribution from environment */
    result = gm_attribution_from_env(attribution);
    if (result != GM_OK) {
        fprintf(stderr, GM_ERR_PARSE_ARGS "\n");
        return result;
    }
    
    /* For AI sources, use provided confidence or default AI confidence */
    if (attribution->source_type != GM_SOURCE_HUMAN && !confidence_str) {
        *confidence = GM_AI_DEFAULT_CONFIDENCE;
    }
    
    return GM_OK;
}

/* Create edge from arguments */
static int create_edge_from_args(gm_context_t *ctx, const char *src_path,
                               const char *tgt_path, gm_rel_type_t rel_type,
                               uint16_t confidence, const gm_attribution_t *attribution,
                               gm_edge_attributed_t *edge) {
    int result = gm_edge_attributed_create(ctx, src_path, tgt_path, rel_type, 
                                         confidence, attribution, GM_LANE_DEFAULT, edge);
    if (result != GM_OK) {
        fprintf(stderr, "Error: %s\n", gm_error_string(result));
    }
    return result;
}

/* Save edge to journal */
static int save_edge_to_journal(gm_context_t *ctx, const gm_edge_attributed_t *edge) {
    int result = gm_journal_append_attributed(ctx, edge, 1);
    if (result != GM_OK) {
        fprintf(stderr, GM_ERR_WRITE_FAILED "\n");
    }
    return result;
}

/* Print success message */
static void print_link_success(const gm_edge_attributed_t *edge,
                             const gm_attribution_t *attribution) {
    char formatted[GM_FORMAT_BUFFER_SIZE];
    
    if (attribution->source_type == GM_SOURCE_HUMAN) {
        gm_edge_attributed_format(edge, formatted, sizeof(formatted));
    } else {
        gm_edge_attributed_format_with_attribution(edge, formatted, sizeof(formatted));
    }
    printf(GM_SUCCESS_CREATED "\n", formatted);
}

/* Link command implementation with attribution support */
int gm_cmd_link(gm_context_t *ctx, int argc, char **argv) {
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
    result = parse_link_arguments(argc, argv, &src_path, &tgt_path, 
                                 &type_str, &confidence_str);
    if (result != GM_OK) {
        return result;
    }
    
    /* Validate inputs */
    result = validate_link_inputs(type_str, confidence_str, &rel_type, 
                                 &confidence, &attribution);
    if (result != GM_OK) {
        return result;
    }
    
    /* Create edge */
    result = create_edge_from_args(ctx, src_path, tgt_path, rel_type, 
                                  confidence, &attribution, &edge);
    if (result != GM_OK) {
        return result;
    }
    
    /* Save to journal */
    result = save_edge_to_journal(ctx, &edge);
    if (result != GM_OK) {
        return result;
    }
    
    /* Print success */
    print_link_success(&edge, &attribution);
    
    return GM_OK;
}