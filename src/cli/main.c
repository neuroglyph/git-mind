// SPDX-License-Identifier: Apache-2.0
// © 2025 J. Kirby Ross / Neuroglyph Collective

#include "gitmind_lib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

// Global flags
static int verbose = 0;
static int porcelain = 0;

// Message constants
#define MSG_INIT_SUCCESS "GitMind initialized successfully\n"
#define MSG_LINK_CREATED "Link created: %s -> %s [%s]\n"
#define MSG_NO_LINKS "No links found\n"
#define MSG_LINK_FORMAT "%s: %s -> %s (ts:%ld)\n"
#define MSG_LINK_REMOVED "Link removed: %s -> %s\n"
#define MSG_ALL_LINKS_VALID "All links are valid\n"
#define MSG_BROKEN_LINKS_FOUND "Found %d broken link%s\n"
#define MSG_RUN_CHECK_FIX "Run 'gitmind check --fix' to remove them\n"
#define MSG_BROKEN_LINKS_REMOVED "Removed %d broken link%s\n"
#define MSG_VERSION_FORMAT "git-mind version %s\n"

// Porcelain message constants
#define PORCELAIN_INIT_OK "OK\n"
#define PORCELAIN_LINK_CREATED "CREATED %s %s %s\n"
#define PORCELAIN_LINK_FORMAT "%s %s %s %ld\n"
#define PORCELAIN_LINK_REMOVED "REMOVED %s %s\n"

// Error message constants
#define ERR_MSG_LINK_REQUIRES_ARGS "Error: link command requires source and target arguments\n"
#define ERR_MSG_UNLINK_REQUIRES_ARGS "Error: unlink command requires source and target arguments\n"
#define ERR_MSG_MISSING_FILE_ARG "Error: missing file argument\n"
#define ERR_MSG_DEPTH_OUT_OF_RANGE "Error: depth must be between 1 and %d\n"
#define ERR_MSG_UNKNOWN_COMMAND "Error: unknown command '%s'\n"

// Default constants
#define GM_DEFAULT_DEPTH 1
#define GM_MAX_DEPTH 10

// Format types
typedef enum {
    GM_FORMAT_TREE,
    GM_FORMAT_LIST
} gm_format_t;

static void print_usage(const char* prog) {
    fprintf(stderr, "Usage: %s [--verbose] [--porcelain] <command> [options]\n", prog);
    fprintf(stderr, "\nGlobal options:\n");
    fprintf(stderr, "  --verbose, -v           Enable verbose output\n");
    fprintf(stderr, "  --porcelain             Machine-readable output\n");
    fprintf(stderr, "\nCommands:\n");
    fprintf(stderr, "  init                    Initialize git-mind in current repository\n");
    fprintf(stderr, "  link <source> <target>  Create a link between files\n");
    fprintf(stderr, "    --type TYPE           Link type (default: REFERENCES)\n");
    fprintf(stderr, "  list                    List all links\n");
    fprintf(stderr, "    --source FILE         Filter by source file\n");
    fprintf(stderr, "    --target FILE         Filter by target file\n");
    fprintf(stderr, "  traverse <file>         Traverse graph from file\n");
    fprintf(stderr, "    --depth N             Traversal depth (default: 1, max: 10)\n");
    fprintf(stderr, "    --format FORMAT       Output format: tree, list (default: tree)\n");
    fprintf(stderr, "  unlink <source> <target> Remove link between files\n");
    fprintf(stderr, "  check                   Check link integrity\n");
    fprintf(stderr, "    --fix                 Remove broken links\n");
    fprintf(stderr, "  status                  Show repository status\n");
    fprintf(stderr, "  version                 Show version\n");
}

static int cmd_init(gm_context_t* ctx, int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    int ret = gm_init(ctx, ".");
    if (ret != GM_OK) {
        fprintf(stderr, "Error: %s\n", gm_last_error(ctx));
        return 1;
    }
    
    if (porcelain) {
        printf(PORCELAIN_INIT_OK);
    } else if (verbose) {
        printf(MSG_INIT_SUCCESS);
    }
    return 0;
}

static int cmd_link(gm_context_t* ctx, int argc, char** argv) {
    const char* type = "REFERENCES";
    
    // Save global optind 
    int cmd_index = optind;  // This points to "link"
    
    // We need to find source and target before any options
    // They should be right after the command
    if (argc < cmd_index + 3) {  // Need at least: link source target
        fprintf(stderr, ERR_MSG_LINK_REQUIRES_ARGS);
        print_usage(argv[0]);
        return 1;
    }
    
    const char* source = argv[cmd_index + 1];
    const char* target = argv[cmd_index + 2];
    
    // Now check for --type option after source and target
    for (int i = cmd_index + 3; i < argc; i++) {
        if (strcmp(argv[i], "--type") == 0 || strcmp(argv[i], "-t") == 0) {
            if (i + 1 < argc) {
                type = argv[i + 1];
                i++; // Skip the argument
            } else {
                fprintf(stderr, "Error: --type requires an argument\n");
                return 1;
            }
        }
    }
    
    int ret = gm_link_create(ctx, source, target, type);
    if (ret != GM_OK) {
        fprintf(stderr, "Error: %s\n", gm_last_error(ctx));
        return 1;
    }
    
    if (porcelain) {
        printf(PORCELAIN_LINK_CREATED, source, target, type);
    } else if (verbose) {
        printf(MSG_LINK_CREATED, source, target, type);
    }
    return 0;
}

static int cmd_list(gm_context_t* ctx, int argc, char** argv) {
    const char* filter_source = NULL;
    const char* filter_target = NULL;
    
    // Parse options
    int opt;
    static struct option long_options[] = {
        {"source", required_argument, 0, 's'},
        {"target", required_argument, 0, 't'},
        {0, 0, 0, 0}
    };
    
    // Save global optind and set it to after the command
    int saved_optind = optind;
    optind = saved_optind + 1;  // Skip past "list" command
    
    while ((opt = getopt_long(argc, argv, "s:t:", long_options, NULL)) != -1) {
        switch (opt) {
            case 's':
                filter_source = optarg;
                break;
            case 't':
                filter_target = optarg;
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    gm_link_set_t* set = NULL;
    int ret = gm_link_list(ctx, &set, filter_source, filter_target);
    if (ret != GM_OK) {
        fprintf(stderr, "Error: %s\n", gm_last_error(ctx));
        return 1;
    }
    
    if (set->count == 0) {
        if (!porcelain) {
            printf(MSG_NO_LINKS);
        }
    } else {
        for (size_t i = 0; i < set->count; i++) {
            gm_link_t* link = &set->links[i];
            if (porcelain) {
                printf(PORCELAIN_LINK_FORMAT, 
                    link->type, link->source, link->target, link->timestamp);
            } else {
                printf(MSG_LINK_FORMAT, 
                    link->type, link->source, link->target, link->timestamp);
            }
        }
    }
    
    gm_link_set_free(set);
    return 0;
}

static int cmd_unlink(gm_context_t* ctx, int argc, char** argv) {
    // Save global optind
    int saved_optind = optind;
    int cmd_start = saved_optind + 1;  // Skip past "unlink" command
    
    if (argc != cmd_start + 2) {
        fprintf(stderr, ERR_MSG_UNLINK_REQUIRES_ARGS);
        print_usage(argv[0]);
        return 1;
    }
    
    const char* source = argv[cmd_start];
    const char* target = argv[cmd_start + 1];
    
    int ret = gm_link_unlink(ctx, source, target);
    if (ret != GM_OK) {
        fprintf(stderr, "Error: %s\n", gm_last_error(ctx));
        return 1;
    }
    
    if (porcelain) {
        printf(PORCELAIN_LINK_REMOVED, source, target);
    } else if (verbose) {
        printf(MSG_LINK_REMOVED, source, target);
    }
    return 0;
}

static int cmd_check(gm_context_t* ctx, int argc, char** argv) {
    int fix = 0;
    
    // Save global optind
    int saved_optind = optind;
    int cmd_start = saved_optind + 1;  // Skip past "check" command
    
    // Check for --fix flag
    for (int i = cmd_start; i < argc; i++) {
        if (strcmp(argv[i], "--fix") == 0) {
            fix = 1;
            break;
        }
    }
    
    int broken_count = 0;
    int ret = gm_check(ctx, fix, &broken_count);
    if (ret != GM_OK) {
        fprintf(stderr, "Error: %s\n", gm_last_error(ctx));
        return 1;
    }
    
    if (broken_count == 0) {
        printf(MSG_ALL_LINKS_VALID);
    } else if (!fix) {
        printf(MSG_BROKEN_LINKS_FOUND, broken_count, broken_count == 1 ? "" : "s");
        printf(MSG_RUN_CHECK_FIX);
    } else {
        printf(MSG_BROKEN_LINKS_REMOVED, broken_count, broken_count == 1 ? "" : "s");
    }
    
    return 0;
}

static int cmd_status(gm_context_t* ctx, int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    int link_count = 0, unique_files = 0;
    int ret = gm_status(ctx, &link_count, &unique_files);
    if (ret != GM_OK) {
        fprintf(stderr, "Error: %s\n", gm_last_error(ctx));
        return 1;
    }
    
    if (porcelain) {
        printf("LINKS %d\n", link_count);
        printf("FILES %d\n", unique_files);
    } else {
        printf("Total links: %d\n", link_count);
        printf("Unique files: %d\n", unique_files);
    }
    
    return 0;
}

static int cmd_version(gm_context_t* ctx, int argc, char** argv) {
    (void)ctx;
    (void)argc;
    (void)argv;
    
    printf(MSG_VERSION_FORMAT, gm_version_string());
    return 0;
}

// Traverse callback for tree format
static void traverse_tree_callback(const gm_link_t* link, int level, void* userdata) {
    (void)userdata;
    for (int i = 0; i < level; i++) {
        printf("  ");
    }
    printf("└─ %s -> %s [%s]\n", link->source, link->target, link->type);
}

// Traverse callback for list format
static void traverse_list_callback(const gm_link_t* link, int level, void* userdata) {
    (void)userdata;
    printf("%d: %s -> %s [%s]\n", level, link->source, link->target, link->type);
}

static int cmd_traverse(gm_context_t* ctx, int argc, char** argv) {
    // Save global optind
    int saved_optind = optind;
    int cmd_start = saved_optind + 1;  // Skip past "traverse" command
    
    if (argc < cmd_start + 1) {
        fprintf(stderr, ERR_MSG_MISSING_FILE_ARG);
        fprintf(stderr, "Usage: gitmind traverse <file> [options]\n");
        return 1;
    }
    
    const char* file = argv[cmd_start];
    int depth = GM_DEFAULT_DEPTH;
    gm_format_t format = GM_FORMAT_TREE;
    
    // Parse options
    int opt;
    static struct option long_options[] = {
        {"depth", required_argument, 0, 'd'},
        {"format", required_argument, 0, 'f'},
        {0, 0, 0, 0}
    };
    
    optind = cmd_start + 1; // Start after "traverse <file>"
    while ((opt = getopt_long(argc, argv, "d:f:", long_options, NULL)) != -1) {
        switch (opt) {
        case 'd':
            depth = atoi(optarg);
            if (depth <= 0 || depth > GM_MAX_DEPTH) {
                fprintf(stderr, ERR_MSG_DEPTH_OUT_OF_RANGE, GM_MAX_DEPTH);
                return 1;
            }
            break;
        case 'f':
            if (strcmp(optarg, "tree") == 0) {
                format = GM_FORMAT_TREE;
            } else if (strcmp(optarg, "list") == 0) {
                format = GM_FORMAT_LIST;
            } else {
                fprintf(stderr, "Error: Unknown format '%s' (use 'tree' or 'list')\n", optarg);
                return 1;
            }
            break;
        default:
            return 1;
        }
    }
    
    // Use appropriate callback based on format
    void (*callback)(const gm_link_t*, int, void*) = 
        (format == GM_FORMAT_TREE) ? traverse_tree_callback : traverse_list_callback;
    
    if (!porcelain) {
        printf("Traversing from: %s (depth: %d)\n", file, depth);
    }
    
    int ret = gm_traverse(ctx, file, depth, callback, NULL);
    if (ret != GM_OK) {
        fprintf(stderr, "Error: %s\n", gm_last_error(ctx));
        return 1;
    }
    
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    // Parse global options
    int opt;
    static struct option long_options[] = {
        {"verbose", no_argument, 0, 'v'},
        {"porcelain", no_argument, 0, 'p'},
        {0, 0, 0, 0}
    };
    
    // Reset optind for global option parsing
    optind = 1;
    
    // Process global options until we hit a non-option
    while ((opt = getopt_long(argc, argv, "+v", long_options, NULL)) != -1) {
        switch (opt) {
            case 'v':
                verbose = 1;
                break;
            case 'p':
                porcelain = 1;
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    // Check if we have a command after options
    if (optind >= argc) {
        print_usage(argv[0]);
        return 1;
    }
    
    const char* cmd = argv[optind];
    
    // Create context with default backend
    gm_context_t* ctx = gm_create_context(NULL);
    if (!ctx) {
        fprintf(stderr, "Error: Failed to create context\n");
        return 1;
    }
    
    // Set output mode based on flags
    gm_output_mode_t output_mode = GM_OUTPUT_SILENT;
    if (verbose) {
        output_mode = GM_OUTPUT_VERBOSE;
    } else if (porcelain) {
        output_mode = GM_OUTPUT_PORCELAIN;
    }
    gm_set_output_mode(ctx, output_mode);
    
    // Don't shift argv - pass original with adjusted indices
    
    int ret = 0;
    if (strcmp(cmd, "init") == 0) {
        ret = cmd_init(ctx, argc, argv);
    } else if (strcmp(cmd, "version") == 0) {
        ret = cmd_version(ctx, argc, argv);
    } else {
        // For all non-init/version commands, initialize the repository context
        ret = gm_init(ctx, ".");
        if (ret != GM_OK) {
            fprintf(stderr, "Error: %s\n", gm_last_error(ctx));
            gm_destroy_context(ctx);
            return 1;
        }
        
        if (strcmp(cmd, "link") == 0) {
            ret = cmd_link(ctx, argc, argv);
        } else if (strcmp(cmd, "list") == 0) {
            ret = cmd_list(ctx, argc, argv);
        } else if (strcmp(cmd, "unlink") == 0) {
            ret = cmd_unlink(ctx, argc, argv);
        } else if (strcmp(cmd, "check") == 0) {
            ret = cmd_check(ctx, argc, argv);
        } else if (strcmp(cmd, "status") == 0) {
            ret = cmd_status(ctx, argc, argv);
        } else if (strcmp(cmd, "traverse") == 0) {
            ret = cmd_traverse(ctx, argc, argv);
        } else {
            fprintf(stderr, ERR_MSG_UNKNOWN_COMMAND, cmd);
            print_usage(argv[0]);
            ret = 1;
        }
    }
    
    // Cleanup context
    gm_destroy_context(ctx);
    return ret;
}
