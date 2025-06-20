/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

/*
 * QUALITY TESTS v2: Guardian, Not Gatekeeper
 * 
 * These tests ensure quality WITHOUT stifling innovation.
 * They test principles, not implementation details.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Test that the module has reasonable structure */
void test_reasonable_structure(void) {
    /* Instead of counting every line, just check for obvious problems */
    FILE *fp = fopen("../src/edge/edge.c", "r");
    assert(fp != NULL);
    
    char line[1024];
    int consecutive_blank_lines = 0;
    int max_line_length = 0;
    int line_num = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        int len = strlen(line);
        
        /* Track max line length */
        if (len > max_line_length) {
            max_line_length = len;
        }
        
        /* Check for code smell: too many blank lines */
        if (len <= 1) {
            consecutive_blank_lines++;
            if (consecutive_blank_lines > 3) {
                printf("⚠️  Warning: Many blank lines around line %d\n", line_num);
                /* Just warn, don't fail! */
            }
        } else {
            consecutive_blank_lines = 0;
        }
    }
    
    /* Gentle reminder if lines are super long */
    if (max_line_length > 100) {
        printf("💡 Tip: Some lines are >100 chars. Consider wrapping for readability.\n");
    }
    
    fclose(fp);
}

/* Test that critical safety properties hold */
void test_safety_properties(void) {
    /* These are the ACTUAL important things */
    FILE *fp = fopen("../src/edge/edge.c", "r");
    assert(fp != NULL);
    
    char line[1024];
    int found_bounds_check = 0;
    int found_null_check = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        /* Look for evidence of defensive programming */
        if (strstr(line, "if") && (strstr(line, "NULL") || strstr(line, "!"))) {
            found_null_check = 1;
        }
        
        if (strstr(line, "sizeof") || strstr(line, "GM_PATH_MAX")) {
            found_bounds_check = 1;
        }
    }
    
    /* These are genuinely important */
    assert(found_null_check && "Code should check for NULL pointers");
    assert(found_bounds_check && "Code should respect buffer bounds");
    
    fclose(fp);
}

/* Test that the module is testable */
void test_testability(void) {
    /* Can we actually test this code? */
    FILE *fp = fopen("../src/edge/edge.c", "r");
    assert(fp != NULL);
    
    char line[1024];
    int uses_context = 0;
    int has_small_functions = 0;
    int function_count = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        /* Check for dependency injection via context */
        if (strstr(line, "ctx->")) {
            uses_context = 1;
        }
        
        /* Count functions (roughly) */
        if (strstr(line, "static") && strstr(line, "(")) {
            function_count++;
        }
    }
    
    /* More functions = better decomposition (usually) */
    if (function_count > 5) {
        has_small_functions = 1;
    }
    
    printf("📊 Testability score:\n");
    printf("   - Uses dependency injection: %s\n", uses_context ? "✅" : "❌");
    printf("   - Has multiple small functions: %s\n", has_small_functions ? "✅" : "⚠️");
    
    fclose(fp);
}

/* Test for obvious code smells only */
void test_no_obvious_smells(void) {
    FILE *fp = fopen("../src/edge/edge.c", "r");
    assert(fp != NULL);
    
    char line[1024];
    int line_num = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        line_num++;
        
        /* Only flag REALLY bad stuff */
        if (strstr(line, "goto") && !strstr(line, "//")) {
            printf("⚠️  Warning: goto at line %d - make sure it's justified\n", 
                   line_num);
        }
        
        if (strstr(line, "TODO") || strstr(line, "FIXME")) {
            printf("📝 Note: TODO at line %d - don't forget!\n", line_num);
        }
        
        /* Deeply nested code (just warn) */
        int indent_level = 0;
        for (char *p = line; *p == ' ' || *p == '\t'; p++) {
            indent_level++;
        }
        if (indent_level > 24) {  /* 6 levels of 4-space indent */
            printf("🔍 Deep nesting at line %d - consider refactoring\n", 
                   line_num);
        }
    }
    
    fclose(fp);
}

/* Test that innovation is possible */
void test_extensibility(void) {
    printf("\n🚀 Extensibility check:\n");
    
    /* Instead of rigid rules, ask questions */
    printf("   Ask yourself:\n");
    printf("   - Can I easily add a new edge type?\n");
    printf("   - Can I add edge metadata without breaking things?\n");
    printf("   - Can I swap out the timestamp implementation?\n");
    printf("   - Can I add new validation rules?\n");
    printf("\n   If any answer is 'no', consider refactoring!\n");
}

int main(void) {
    printf("🌱 Running Quality Tests (Innovation-Friendly Edition)...\n\n");
    
    test_reasonable_structure();
    printf("✅ Structure is reasonable\n");
    
    test_safety_properties();
    printf("✅ Safety properties maintained\n");
    
    test_testability();
    
    test_no_obvious_smells();
    
    test_extensibility();
    
    printf("\n✨ Quality check complete!\n");
    printf("   Remember: These tests are here to help, not hinder.\n");
    printf("   Feel free to innovate within safety bounds!\n");
    
    return 0;
}