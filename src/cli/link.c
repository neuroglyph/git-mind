/* SPDX-License-Identifier: Apache-2.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#define _GNU_SOURCE

#include "gitmind.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Parse relationship type from string */
static gm_rel_type_t parse_rel_type(const char *str) {
    if (strcasecmp(str, "implements") == 0) {
        return GM_REL_IMPLEMENTS;
    } else if (strcasecmp(str, "references") == 0) {
        return GM_REL_REFERENCES;
    } else if (strcasecmp(str, "depends_on") == 0 || strcasecmp(str, "depends-on") == 0) {
        return GM_REL_DEPENDS_ON;
    } else if (strcasecmp(str, "augments") == 0) {
        return GM_REL_AUGMENTS;
    }
    return GM_REL_CUSTOM;
}

/* Link command implementation */
int gm_cmd_link(gm_context_t *ctx, int argc, char **argv) {
    const char *src_path = NULL;
    const char *tgt_path = NULL;
    const char *type_str = "references";
    gm_rel_type_t rel_type;
    gm_edge_t edge;
    int result;
    
    /* Parse arguments */
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "--type") == 0 && i + 1 < argc) {
            type_str = argv[++i];
        } else if (!src_path) {
            src_path = argv[i];
        } else if (!tgt_path) {
            tgt_path = argv[i];
        }
    }
    
    /* Validate arguments */
    if (!src_path || !tgt_path) {
        fprintf(stderr, "Usage: git-mind link <source> <target> [--type <type>]\n");
        fprintf(stderr, "Types: implements, references, depends_on, augments\n");
        return GM_INVALID_ARG;
    }
    
    /* Parse relationship type */
    rel_type = parse_rel_type(type_str);
    
    /* Create edge */
    result = gm_edge_create(ctx, src_path, tgt_path, rel_type, &edge);
    if (result != GM_OK) {
        fprintf(stderr, "Error: %s\n", gm_error_string(result));
        return result;
    }
    
    /* Append to journal */
    result = gm_journal_append(ctx, &edge, 1);
    if (result != GM_OK) {
        fprintf(stderr, "Error: Failed to write link\n");
        return result;
    }
    
    /* Print success */
    char formatted[512];
    gm_edge_format(&edge, formatted, sizeof(formatted));
    printf("Created link: %s\n", formatted);
    
    return GM_OK;
}