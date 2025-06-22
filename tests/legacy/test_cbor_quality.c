/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

/*
 * QUALITY REGRESSION TESTS FOR CBOR MODULE
 * 
 * These tests ensure CBOR modules NEVER degrade in quality again.
 * We achieved ZERO warnings during migration - let's keep it that way!
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#define MAX_LINE_LENGTH 1024
#define MAX_FUNCTION_LINES 25

/* Test that functions follow SRP and size limits */
void test_cbor_function_sizes(void) {
    const char *files[] = {
        "../src/attribution/cbor.c",
        "../src/attribution/cbor_common.c", 
        "../src/attribution/cbor_decode_ex.c",
        NULL
    };
    
    for (int file_idx = 0; files[file_idx]; file_idx++) {
        FILE *fp = fopen(files[file_idx], "r");
        assert(fp != NULL);
        
        char line[MAX_LINE_LENGTH];
        int in_function = 0;
        int line_count = 0;
        char func_name[256] = {0};
        
        while (fgets(line, sizeof(line), fp)) {
            /* Detect function start */
            if (!in_function && strstr(line, "(") && strstr(line, ")") && 
                strstr(line, "{")) {
                in_function = 1;
                line_count = 1;
                sscanf(line, "%*[^a-zA-Z_]%255[a-zA-Z0-9_]", func_name);
                continue;
            }
            
            if (in_function) {
                line_count++;
                
                /* Detect function end */
                if (line[0] == '}' && strlen(line) < 3) {
                    if (line_count > MAX_FUNCTION_LINES) {
                        fprintf(stderr, "FAIL: %s: Function '%s' has %d lines (max %d)\n",
                                files[file_idx], func_name, line_count, MAX_FUNCTION_LINES);
                        assert(0);
                    }
                    in_function = 0;
                }
            }
        }
        
        fclose(fp);
    }
}

/* Test that all memory operations use gm_mem.h */
void test_cbor_uses_safe_memory(void) {
    const char *files[] = {
        "../src/attribution/cbor.c",
        "../src/attribution/cbor_common.c", 
        "../src/attribution/cbor_decode_ex.c",
        NULL
    };
    
    const char *banned[] = {
        " memcpy(",
        " memset(",
        " memmove(",
        " strcpy(",
        " strncpy(",
        NULL
    };
    
    for (int file_idx = 0; files[file_idx]; file_idx++) {
        FILE *fp = fopen(files[file_idx], "r");
        assert(fp != NULL);
        
        char line[MAX_LINE_LENGTH];
        int line_num = 0;
        
        while (fgets(line, sizeof(line), fp)) {
            line_num++;
            
            /* Skip comments */
            if (strstr(line, "//") || strstr(line, "/*")) {
                continue;
            }
            
            /* Check for banned functions */
            for (int ban_idx = 0; banned[ban_idx]; ban_idx++) {
                if (strstr(line, banned[ban_idx])) {
                    fprintf(stderr, "FAIL: %s:%d uses banned function %s\n",
                            files[file_idx], line_num, banned[ban_idx]);
                    fprintf(stderr, "      Use gm_mem.h functions instead!\n");
                    assert(0);
                }
            }
        }
        
        fclose(fp);
    }
}

/* Test that DI pattern is followed */
void test_cbor_dependency_injection(void) {
    /* cbor.c should have _ex functions for test doubles */
    FILE *fp = fopen("../src/attribution/cbor.c", "r");
    assert(fp != NULL);
    
    int found_encode_ex = 0;
    int found_decode_ex = 0;
    char line[MAX_LINE_LENGTH];
    
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "gm_edge_encode_cbor_ex")) {
            found_encode_ex = 1;
        }
        if (strstr(line, "gm_edge_decode_cbor_ex")) {
            found_decode_ex = 1;
        }
    }
    
    fclose(fp);
    
    assert(found_encode_ex && "cbor.c should have encode_ex for test doubles");
    assert(found_decode_ex && "cbor.c should have decode_ex for test doubles");
}

/* Test that loop variables follow convention */
void test_cbor_loop_conventions(void) {
    const char *files[] = {
        "../src/attribution/cbor.c",
        "../src/attribution/cbor_common.c", 
        "../src/attribution/cbor_decode_ex.c",
        NULL
    };
    
    for (int file_idx = 0; files[file_idx]; file_idx++) {
        FILE *fp = fopen(files[file_idx], "r");
        assert(fp != NULL);
        
        char line[MAX_LINE_LENGTH];
        int line_num = 0;
        
        while (fgets(line, sizeof(line), fp)) {
            line_num++;
            
            /* Check for banned loop variable names */
            if (strstr(line, "for") && strstr(line, "int")) {
                /* Should NOT use _i_ (reserved) or i (too simple during migration) */
                if (strstr(line, "int _i_") || strstr(line, "int i ") || 
                    strstr(line, "int i;") || strstr(line, "int i=")) {
                    fprintf(stderr, "FAIL: %s:%d uses improper loop variable\n",
                            files[file_idx], line_num);
                    fprintf(stderr, "      Use descriptive names like 'idx' or 'index'\n");
                    assert(0);
                }
            }
        }
        
        fclose(fp);
    }
}

int main(void) {
    printf("🔍 Running CBOR quality regression tests...\n\n");
    
    printf("📏 Testing function sizes (SRP)...\n");
    test_cbor_function_sizes();
    printf("✅ All functions follow size limits!\n\n");
    
    printf("🛡️ Testing memory safety...\n");
    test_cbor_uses_safe_memory();
    printf("✅ All memory operations use gm_mem.h!\n\n");
    
    printf("💉 Testing dependency injection...\n");
    test_cbor_dependency_injection();
    printf("✅ DI pattern properly implemented!\n\n");
    
    printf("🔄 Testing loop conventions...\n");
    test_cbor_loop_conventions();
    printf("✅ Loop variables follow conventions!\n\n");
    
    printf("🎉 ALL TESTS PASSED! CBOR module quality is protected!\n");
    printf("\nRemember: These pedantic tests are for MIGRATION PHASE only.\n");
    printf("Post-migration will use more pragmatic tests.\n");
    
    return 0;
}