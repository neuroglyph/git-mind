/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

/*
 * QUALITY REGRESSION TESTS
 * 
 * These tests ensure edge.c NEVER degrades in quality again.
 * They test structure, not just behavior.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Test that no function exceeds size limits */
void test_function_size_limits(void) {
    FILE *fp = fopen("../src/edge/edge.c", "r");
    assert(fp != NULL);
    
    char line[1024];
    int in_function = 0;
    int line_count = 0;
    char func_name[256] = {0};
    
    while (fgets(line, sizeof(line), fp)) {
        /* Detect function start */
        if (!in_function && strstr(line, "(") && strstr(line, ")") && 
            strstr(line, "{")) {
            in_function = 1;
            line_count = 1;
            /* Extract function name for error reporting */
            sscanf(line, "%*[^a-zA-Z_]%255[a-zA-Z0-9_]", func_name);
            continue;
        }
        
        if (in_function) {
            line_count++;
            
            /* Detect function end */
            if (line[0] == '}' && strlen(line) < 3) {
                /* Verify function is within limits */
                if (line_count > 25) {
                    fprintf(stderr, "FAIL: Function '%s' has %d lines (max 25)\n",
                            func_name, line_count);
                    assert(0);
                }
                in_function = 0;
            }
        }
    }
    
    fclose(fp);
}

/* Test that no magic numbers exist */
void test_no_magic_numbers(void) {
    FILE *fp = fopen("../src/edge/edge.c", "r");
    assert(fp != NULL);
    
    char line[1024];
    int line_num = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        
        /* Skip comments and strings */
        if (strstr(line, "//") || strstr(line, "/*") || strstr(line, "\"")) {
            continue;
        }
        
        /* Check for numeric literals (except 0, 1, -1) */
        char *ptr = line;
        while (*ptr) {
            if (isdigit(*ptr)) {
                int num = atoi(ptr);
                if (num != 0 && num != 1 && num != -1) {
                    /* Should be a #define */
                    fprintf(stderr, "FAIL: Magic number %d at line %d\n", 
                            num, line_num);
                    assert(0);
                }
                /* Skip past this number */
                while (isdigit(*ptr)) ptr++;
            } else {
                ptr++;
            }
        }
    }
    
    fclose(fp);
}

/* Test that all functions have single responsibility */
void test_single_responsibility(void) {
    /* Each function should:
     * 1. Have a clear, single-purpose name
     * 2. Not contain "and" in its name
     * 3. Not call more than 5 other functions
     */
    FILE *fp = fopen("../src/edge/edge.c", "r");
    assert(fp != NULL);
    
    char line[1024];
    int in_function = 0;
    int call_count = 0;
    char func_name[256] = {0};
    
    while (fgets(line, sizeof(line), fp)) {
        /* Function detection logic... */
        if (!in_function && strstr(line, "(") && !strstr(line, ";")) {
            in_function = 1;
            call_count = 0;
            sscanf(line, "%*[^a-zA-Z_]%255[a-zA-Z0-9_]", func_name);
            
            /* Check name doesn't have "and" */
            if (strstr(func_name, "_and_")) {
                fprintf(stderr, "FAIL: Function '%s' does multiple things\n",
                        func_name);
                assert(0);
            }
        }
        
        if (in_function) {
            /* Count function calls */
            char *call = strstr(line, "(");
            if (call && !strstr(line, "if") && !strstr(line, "for") &&
                !strstr(line, "while")) {
                call_count++;
            }
            
            if (line[0] == '}') {
                if (call_count > 5) {
                    fprintf(stderr, "FAIL: Function '%s' makes %d calls (max 5)\n",
                            func_name, call_count);
                    assert(0);
                }
                in_function = 0;
            }
        }
    }
    
    fclose(fp);
}

/* Test that no insecure functions are used */
void test_no_insecure_functions(void) {
    const char *banned[] = {
        "strcpy", "strcat", "sprintf", "gets", "memcpy", "memmove"
    };
    
    FILE *fp = fopen("../src/edge/edge.c", "r");
    assert(fp != NULL);
    
    char line[1024];
    int line_num = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        
        for (int i = 0; i < sizeof(banned)/sizeof(banned[0]); i++) {
            if (strstr(line, banned[i]) && strstr(line, "(")) {
                /* Check it's not our safe wrapper */
                if (!strstr(line, "safe_")) {
                    fprintf(stderr, "FAIL: Insecure function '%s' at line %d\n",
                            banned[i], line_num);
                    assert(0);
                }
            }
        }
    }
    
    fclose(fp);
}

/* Test variable naming conventions */
void test_variable_naming(void) {
    FILE *fp = fopen("../src/edge/edge.c", "r");
    assert(fp != NULL);
    
    char line[1024];
    int line_num = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        
        /* Look for variable declarations */
        if ((strstr(line, "int ") || strstr(line, "size_t ") || 
             strstr(line, "char ")) && strstr(line, ";")) {
            
            char var_name[256];
            /* Extract variable name */
            if (sscanf(line, "%*[^a-zA-Z_]%*[a-zA-Z_]*%*[ *]%255[a-zA-Z0-9_]",
                       var_name) == 1) {
                
                /* Check minimum length (except i,j,k in for loops) */
                if (strlen(var_name) < 3 && !strstr(line, "for")) {
                    fprintf(stderr, "FAIL: Variable '%s' too short at line %d\n",
                            var_name, line_num);
                    assert(0);
                }
                
                /* Check snake_case */
                for (char *p = var_name; *p; p++) {
                    if (isupper(*p)) {
                        fprintf(stderr, "FAIL: Variable '%s' not snake_case\n",
                                var_name);
                        assert(0);
                    }
                }
            }
        }
    }
    
    fclose(fp);
}

/* Test that all parameters are validated */
void test_parameter_validation(void) {
    /* Every public function should validate its parameters */
    /* This is a bit complex, but ensures defensive programming */
    
    /* For now, just check that gm_edge_* functions check for NULL */
    FILE *fp = fopen("../src/edge/edge.c", "r");
    assert(fp != NULL);
    
    char line[1024];
    int in_public_func = 0;
    int found_validation = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "int gm_edge_") && strstr(line, "(")) {
            in_public_func = 1;
            found_validation = 0;
        }
        
        if (in_public_func) {
            if (strstr(line, "if") && strstr(line, "!")) {
                found_validation = 1;
            }
            
            if (line[0] == '}') {
                if (!found_validation) {
                    fprintf(stderr, "FAIL: Public function missing validation\n");
                    assert(0);
                }
                in_public_func = 0;
            }
        }
    }
    
    fclose(fp);
}

/* Master test runner */
int main(void) {
    printf("🛡️  Running Quality Guardian Tests...\n");
    
    test_function_size_limits();
    printf("✅ Function size limits enforced\n");
    
    test_no_magic_numbers();
    printf("✅ No magic numbers found\n");
    
    test_single_responsibility();
    printf("✅ Single responsibility maintained\n");
    
    test_no_insecure_functions();
    printf("✅ No insecure functions used\n");
    
    test_variable_naming();
    printf("✅ Variable naming conventions followed\n");
    
    test_parameter_validation();
    printf("✅ Parameter validation present\n");
    
    printf("\n🏆 QUALITY FORTRESS HOLDS! Edge.c is protected!\n");
    return 0;
}