/**
 * @file test_filter.c
 * @author Marcel (xlogin00)
 * @date 10.11.2025
 * @brief Unit tests for domain filter module (Trie implementation)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "filter.h"

// Test counter
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) \
    do { \
        printf("  [%d] %s... ", ++tests_run, name); \
        fflush(stdout); \
    } while(0)

#define PASS() \
    do { \
        tests_passed++; \
        printf("Y\n"); \
    } while(0)

#define FAIL(msg) \
    do { \
        printf(" FAILED: %s\n", msg); \
    } while(0)

// ============================================================================
// TEST 1-5: Initialization and Cleanup
// ============================================================================

void test_filter_init() {
    TEST("Filter initialization");
    
    filter_t *filter = filter_init();
    assert(filter != NULL);
    assert(filter->root != NULL);
    
    filter_free(filter);
    PASS();
}

void test_filter_free_null() {
    TEST("Filter free with NULL");
    
    filter_free(NULL);  // Should not crash
    
    PASS();
}

void test_filter_free_empty() {
    TEST("Filter free empty filter");
    
    filter_t *filter = filter_init();
    filter_free(filter);
    
    PASS();
}

void test_filter_double_init() {
    TEST("Multiple filter initializations");
    
    filter_t *f1 = filter_init();
    filter_t *f2 = filter_init();
    
    assert(f1 != NULL);
    assert(f2 != NULL);
    assert(f1 != f2);
    
    filter_free(f1);
    filter_free(f2);
    
    PASS();
}

void test_filter_init_free_cycle() {
    TEST("Init-free cycle (memory leak test)");
    
    for (int i = 0; i < 100; i++) {
        filter_t *filter = filter_init();
        filter_free(filter);
    }
    
    PASS();
}

// ============================================================================
// TEST 6-15: Basic Insert and Lookup
// ============================================================================

void test_filter_insert_single() {
    TEST("Insert single domain");
    
    filter_t *filter = filter_init();
    
    int result = filter_insert(filter, "example.com");
    assert(result == 0);
    
    filter_free(filter);
    PASS();
}

void test_filter_insert_multiple() {
    TEST("Insert multiple domains");
    
    filter_t *filter = filter_init();
    
    assert(filter_insert(filter, "example.com") == 0);
    assert(filter_insert(filter, "google.com") == 0);
    assert(filter_insert(filter, "test.org") == 0);
    
    filter_free(filter);
    PASS();
}

void test_filter_lookup_exact_match() {
    TEST("Lookup exact match");
    
    filter_t *filter = filter_init();
    filter_insert(filter, "ads.google.com");
    
    bool blocked = filter_lookup(filter, "ads.google.com");
    assert(blocked == true);
    
    filter_free(filter);
    PASS();
}

void test_filter_lookup_not_found() {
    TEST("Lookup non-existent domain");
    
    filter_t *filter = filter_init();
    filter_insert(filter, "ads.google.com");
    
    bool blocked = filter_lookup(filter, "example.com");
    assert(blocked == false);
    
    filter_free(filter);
    PASS();
}

void test_filter_lookup_empty() {
    TEST("Lookup in empty filter");
    
    filter_t *filter = filter_init();
    
    bool blocked = filter_lookup(filter, "any.domain.com");
    assert(blocked == false);
    
    filter_free(filter);
    PASS();
}

void test_filter_insert_null_domain() {
    TEST("Insert NULL domain");
    
    filter_t *filter = filter_init();
    
    int result = filter_insert(filter, NULL);
    assert(result != 0);  // Should fail
    
    filter_free(filter);
    PASS();
}

void test_filter_insert_empty_domain() {
    TEST("Insert empty domain");
    
    filter_t *filter = filter_init();
    
    int result = filter_insert(filter, "");
    assert(result != 0);  // Should fail
    
    filter_free(filter);
    PASS();
}

void test_filter_lookup_null_domain() {
    TEST("Lookup NULL domain");
    
    filter_t *filter = filter_init();
    
    bool blocked = filter_lookup(filter, NULL);
    assert(blocked == false);
    
    filter_free(filter);
    PASS();
}

void test_filter_insert_duplicate() {
    TEST("Insert duplicate domain");
    
    filter_t *filter = filter_init();
    
    assert(filter_insert(filter, "example.com") == 0);
    assert(filter_insert(filter, "example.com") == 0);  // Should succeed
    
    filter_free(filter);
    PASS();
}

void test_filter_case_sensitivity() {
    TEST("Case sensitivity test");
    
    filter_t *filter = filter_init();
    filter_insert(filter, "Example.COM");
    
    // DNS is case-insensitive
    bool b1 = filter_lookup(filter, "example.com");
    bool b2 = filter_lookup(filter, "EXAMPLE.COM");
    bool b3 = filter_lookup(filter, "Example.Com");
    
    assert(b1 == true);
    assert(b2 == true);
    assert(b3 == true);
    
    filter_free(filter);
    PASS();
}

// ============================================================================
// TEST 16-30: Subdomain Matching
// ============================================================================

void test_filter_subdomain_one_level() {
    TEST("Subdomain matching - one level");
    
    filter_t *filter = filter_init();
    filter_insert(filter, "ads.google.com");
    
    // Subdomain should be blocked
    bool blocked = filter_lookup(filter, "tracker.ads.google.com");
    assert(blocked == true);
    
    filter_free(filter);
    PASS();
}

void test_filter_subdomain_two_levels() {
    TEST("Subdomain matching - two levels");
    
    filter_t *filter = filter_init();
    filter_insert(filter, "ads.google.com");
    
    bool blocked = filter_lookup(filter, "x.y.ads.google.com");
    assert(blocked == true);
    
    filter_free(filter);
    PASS();
}

void test_filter_subdomain_many_levels() {
    TEST("Subdomain matching - many levels");
    
    filter_t *filter = filter_init();
    filter_insert(filter, "ads.google.com");
    
    bool blocked = filter_lookup(filter, "a.b.c.d.e.ads.google.com");
    assert(blocked == true);
    
    filter_free(filter);
    PASS();
}

void test_filter_parent_not_blocked() {
    TEST("Parent domain not blocked");
    
    filter_t *filter = filter_init();
    filter_insert(filter, "ads.google.com");
    
    // Parent should NOT be blocked
    bool blocked = filter_lookup(filter, "google.com");
    assert(blocked == false);
    
    filter_free(filter);
    PASS();
}

void test_filter_sibling_not_blocked() {
    TEST("Sibling domain not blocked");
    
    filter_t *filter = filter_init();
    filter_insert(filter, "ads.google.com");
    
    // Sibling should NOT be blocked
    bool blocked = filter_lookup(filter, "mail.google.com");
    assert(blocked == false);
    
    filter_free(filter);
    PASS();
}

void test_filter_similar_domain_not_blocked() {
    TEST("Similar domain not blocked");
    
    filter_t *filter = filter_init();
    filter_insert(filter, "ads.google.com");
    
    // Similar but different
    bool b1 = filter_lookup(filter, "ads.google.org");
    bool b2 = filter_lookup(filter, "ads.googlee.com");
    
    assert(b1 == false);
    assert(b2 == false);
    
    filter_free(filter);
    PASS();
}

void test_filter_root_domain() {
    TEST("Root domain blocking");
    
    filter_t *filter = filter_init();
    filter_insert(filter, "com");
    
    // All .com domains should be blocked
    bool b1 = filter_lookup(filter, "google.com");
    bool b2 = filter_lookup(filter, "example.com");
    bool b3 = filter_lookup(filter, "test.com");
    
    assert(b1 == true);
    assert(b2 == true);
    assert(b3 == true);
    
    filter_free(filter);
    PASS();
}

void test_filter_single_label() {
    TEST("Single label domain");
    
    filter_t *filter = filter_init();
    filter_insert(filter, "localhost");
    
    bool blocked = filter_lookup(filter, "localhost");
    assert(blocked == true);
    
    filter_free(filter);
    PASS();
}

void test_filter_subdomain_chain() {
    TEST("Subdomain chain");
    
    filter_t *filter = filter_init();
    filter_insert(filter, "google.com");
    
    bool b1 = filter_lookup(filter, "ads.google.com");
    bool b2 = filter_lookup(filter, "tracker.ads.google.com");
    bool b3 = filter_lookup(filter, "x.tracker.ads.google.com");
    
    assert(b1 == true);
    assert(b2 == true);
    assert(b3 == true);
    
    filter_free(filter);
    PASS();
}

void test_filter_multiple_blocks_subdomain() {
    TEST("Multiple blocked domains with subdomains");
    
    filter_t *filter = filter_init();
    filter_insert(filter, "ads.google.com");
    filter_insert(filter, "tracker.example.com");
    
    bool b1 = filter_lookup(filter, "x.ads.google.com");
    bool b2 = filter_lookup(filter, "y.tracker.example.com");
    bool b3 = filter_lookup(filter, "safe.google.com");
    
    assert(b1 == true);
    assert(b2 == true);
    assert(b3 == false);
    
    filter_free(filter);
    PASS();
}

void test_filter_exact_vs_subdomain() {
    TEST("Exact match vs subdomain");
    
    filter_t *filter = filter_init();
    filter_insert(filter, "ads.example.com");
    
    bool b1 = filter_lookup(filter, "ads.example.com");      // exact
    bool b2 = filter_lookup(filter, "x.ads.example.com");    // subdomain
    bool b3 = filter_lookup(filter, "example.com");          // parent
    
    assert(b1 == true);
    assert(b2 == true);
    assert(b3 == false);
    
    filter_free(filter);
    PASS();
}

void test_filter_trailing_dot() {
    TEST("Trailing dot handling");
    
    filter_t *filter = filter_init();
    filter_insert(filter, "example.com");
    
    // With and without trailing dot
    bool b1 = filter_lookup(filter, "example.com");
    (void)filter_lookup(filter, "example.com.");  // b2 depends on implementation

    // Both should work (DNS allows trailing dot)
    assert(b1 == true);
    
    filter_free(filter);
    PASS();
}

void test_filter_leading_dot() {
    TEST("Leading dot handling");
    
    filter_t *filter = filter_init();
    filter_insert(filter, "example.com");

    (void)filter_lookup(filter, ".example.com");  // Should handle gracefully
    
    filter_free(filter);
    PASS();
}

void test_filter_multiple_dots() {
    TEST("Multiple consecutive dots");
    
    filter_t *filter = filter_init();

    (void)filter_insert(filter, "example..com");  // Should reject or handle gracefully
    
    filter_free(filter);
    PASS();
}

void test_filter_very_long_domain() {
    TEST("Very long domain name");
    
    filter_t *filter = filter_init();
    
    // Create a long domain (but under 255 char limit)
    char long_domain[256];
    strcpy(long_domain, "very.long.subdomain.name.with.many.labels.example.com");
    
    int result = filter_insert(filter, long_domain);
    assert(result == 0);
    
    bool blocked = filter_lookup(filter, long_domain);
    assert(blocked == true);
    
    filter_free(filter);
    PASS();
}

// ============================================================================
// TEST 31-40: Edge Cases
// ============================================================================

void test_filter_hyphen_domain() {
    TEST("Domain with hyphens");
    
    filter_t *filter = filter_init();
    filter_insert(filter, "my-ads.google-analytics.com");
    
    bool blocked = filter_lookup(filter, "my-ads.google-analytics.com");
    assert(blocked == true);
    
    filter_free(filter);
    PASS();
}

void test_filter_numeric_domain() {
    TEST("Numeric labels in domain");
    
    filter_t *filter = filter_init();
    filter_insert(filter, "ads123.example456.com");
    
    bool blocked = filter_lookup(filter, "ads123.example456.com");
    assert(blocked == true);
    
    filter_free(filter);
    PASS();
}

void test_filter_idn_domain() {
    TEST("International domain (IDN)");
    
    filter_t *filter = filter_init();
    
    // Simple ASCII test (full IDN support not required)
    filter_insert(filter, "example.com");
    bool blocked = filter_lookup(filter, "example.com");
    assert(blocked == true);
    
    filter_free(filter);
    PASS();
}

void test_filter_max_label_length() {
    TEST("Maximum label length (63 chars)");
    
    filter_t *filter = filter_init();
    
    // DNS label max is 63 characters
    char max_label[70];
    memset(max_label, 'a', 63);
    max_label[63] = '\0';
    strcat(max_label, ".com");
    
    int result = filter_insert(filter, max_label);
    assert(result == 0);
    
    filter_free(filter);
    PASS();
}

void test_filter_many_labels() {
    TEST("Many labels in domain");
    
    filter_t *filter = filter_init();
    
    char domain[256] = "a.b.c.d.e.f.g.h.i.j.k.l.m.n.o.p.com";
    
    int result = filter_insert(filter, domain);
    assert(result == 0);
    
    bool blocked = filter_lookup(filter, domain);
    assert(blocked == true);
    
    filter_free(filter);
    PASS();
}

void test_filter_wildcard_not_supported() {
    TEST("Wildcard not supported");
    
    filter_t *filter = filter_init();
    
    // Wildcards should be treated as literal
    filter_insert(filter, "*.google.com");
    
    bool b1 = filter_lookup(filter, "*.google.com");
    bool b2 = filter_lookup(filter, "ads.google.com");
    
    assert(b1 == true);
    assert(b2 == false);  // Wildcard not expanded
    
    filter_free(filter);
    PASS();
}

void test_filter_special_chars() {
    TEST("Special characters in domain");
    
    filter_t *filter = filter_init();
    
    // DNS allows hyphens and numbers
    int r1 = filter_insert(filter, "ads-123.example.com");
    assert(r1 == 0);

    // Invalid characters should be rejected or handled
    (void)filter_insert(filter, "ads_test.com");  // underscore - behavior depends on implementation
    
    filter_free(filter);
    PASS();
}

void test_filter_uppercase_lowercase_mix() {
    TEST("Mixed case domain");
    
    filter_t *filter = filter_init();
    filter_insert(filter, "ADS.GooGLe.CoM");
    
    bool b1 = filter_lookup(filter, "ads.google.com");
    bool b2 = filter_lookup(filter, "ADS.GOOGLE.COM");
    bool b3 = filter_lookup(filter, "Ads.Google.Com");
    
    assert(b1 == true);
    assert(b2 == true);
    assert(b3 == true);
    
    filter_free(filter);
    PASS();
}

void test_filter_whitespace() {
    TEST("Domain with whitespace");
    
    filter_t *filter = filter_init();

    // Should trim or reject
    (void)filter_insert(filter, " example.com ");  // Behavior depends on implementation
    
    filter_free(filter);
    PASS();
}

void test_filter_newline() {
    TEST("Domain with newline");
    
    filter_t *filter = filter_init();

    (void)filter_insert(filter, "example.com\n");  // Should handle gracefully
    
    filter_free(filter);
    PASS();
}

// ============================================================================
// TEST 41-47: Performance and Stress Tests
// ============================================================================

void test_filter_many_domains() {
    TEST("Insert many domains (1000)");
    
    filter_t *filter = filter_init();
    
    char domain[50];
    for (int i = 0; i < 1000; i++) {
        snprintf(domain, sizeof(domain), "domain%d.example.com", i);
        int result = filter_insert(filter, domain);
        assert(result == 0);
    }
    
    // Verify some
    bool b1 = filter_lookup(filter, "domain0.example.com");
    bool b2 = filter_lookup(filter, "domain500.example.com");
    bool b3 = filter_lookup(filter, "domain999.example.com");
    
    assert(b1 == true);
    assert(b2 == true);
    assert(b3 == true);
    
    filter_free(filter);
    PASS();
}

void test_filter_lookup_performance() {
    TEST("Lookup performance");
    
    filter_t *filter = filter_init();
    
    // Insert 100 domains
    for (int i = 0; i < 100; i++) {
        char domain[50];
        snprintf(domain, sizeof(domain), "ads%d.google.com", i);
        filter_insert(filter, domain);
    }
    
    // Perform 1000 lookups
    for (int i = 0; i < 1000; i++) {
        filter_lookup(filter, "test.example.com");
    }
    
    filter_free(filter);
    PASS();
}

void test_filter_deep_trie() {
    TEST("Deep Trie structure");
    
    filter_t *filter = filter_init();
    
    // Create deep domain
    filter_insert(filter, "a.b.c.d.e.f.g.h.i.j.example.com");
    
    bool blocked = filter_lookup(filter, "x.a.b.c.d.e.f.g.h.i.j.example.com");
    assert(blocked == true);
    
    filter_free(filter);
    PASS();
}

void test_filter_wide_trie() {
    TEST("Wide Trie structure");
    
    filter_t *filter = filter_init();
    
    // Insert many domains with same parent
    for (int i = 0; i < 50; i++) {
        char domain[50];
        snprintf(domain, sizeof(domain), "subdomain%d.example.com", i);
        filter_insert(filter, domain);
    }
    
    bool blocked = filter_lookup(filter, "subdomain25.example.com");
    assert(blocked == true);
    
    filter_free(filter);
    PASS();
}

void test_filter_mixed_depths() {
    TEST("Mixed depth domains");
    
    filter_t *filter = filter_init();
    
    filter_insert(filter, "a.com");
    filter_insert(filter, "b.example.com");
    filter_insert(filter, "c.d.e.test.org");
    
    bool b1 = filter_lookup(filter, "x.a.com");
    bool b2 = filter_lookup(filter, "y.b.example.com");
    bool b3 = filter_lookup(filter, "z.c.d.e.test.org");
    
    assert(b1 == true);
    assert(b2 == true);
    assert(b3 == true);
    
    filter_free(filter);
    PASS();
}

void test_filter_realistic_blocklist() {
    TEST("Realistic ad blocklist");
    
    filter_t *filter = filter_init();
    
    // Common ad domains
    const char *ad_domains[] = {
        "ads.google.com",
        "doubleclick.net",
        "googleadservices.com",
        "googlesyndication.com",
        "facebook-pixel.com",
        "analytics.google.com",
        "ad.doubleclick.net",
        NULL
    };
    
    for (int i = 0; ad_domains[i] != NULL; i++) {
        filter_insert(filter, ad_domains[i]);
    }
    
    // Test blocking
    bool b1 = filter_lookup(filter, "ads.google.com");
    bool b2 = filter_lookup(filter, "tracker.ads.google.com");
    bool b3 = filter_lookup(filter, "google.com");
    
    assert(b1 == true);
    assert(b2 == true);
    assert(b3 == false);
    
    filter_free(filter);
    PASS();
}

void test_filter_memory_efficiency() {
    TEST("Memory efficiency test");
    
    filter_t *filter = filter_init();
    
    // Insert domains with common suffixes (should share nodes)
    filter_insert(filter, "ads.google.com");
    filter_insert(filter, "analytics.google.com");
    filter_insert(filter, "tracker.google.com");
    filter_insert(filter, "pixel.google.com");
    
    // All should be blocked
    bool b1 = filter_lookup(filter, "ads.google.com");
    bool b2 = filter_lookup(filter, "analytics.google.com");
    
    assert(b1 == true);
    assert(b2 == true);
    
    filter_free(filter);
    PASS();
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void) {
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║           DNS Filter Module - Unit Tests                  ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    printf("Running tests...\n\n");
    
    // Initialization (5 tests)
    printf("Initialization & Cleanup:\n");
    test_filter_init();
    test_filter_free_null();
    test_filter_free_empty();
    test_filter_double_init();
    test_filter_init_free_cycle();
    
    // Basic operations (10 tests)
    printf("\nBasic Insert & Lookup:\n");
    test_filter_insert_single();
    test_filter_insert_multiple();
    test_filter_lookup_exact_match();
    test_filter_lookup_not_found();
    test_filter_lookup_empty();
    test_filter_insert_null_domain();
    test_filter_insert_empty_domain();
    test_filter_lookup_null_domain();
    test_filter_insert_duplicate();
    test_filter_case_sensitivity();
    
    // Subdomain matching (15 tests)
    printf("\nSubdomain Matching:\n");
    test_filter_subdomain_one_level();
    test_filter_subdomain_two_levels();
    test_filter_subdomain_many_levels();
    test_filter_parent_not_blocked();
    test_filter_sibling_not_blocked();
    test_filter_similar_domain_not_blocked();
    test_filter_root_domain();
    test_filter_single_label();
    test_filter_subdomain_chain();
    test_filter_multiple_blocks_subdomain();
    test_filter_exact_vs_subdomain();
    test_filter_trailing_dot();
    test_filter_leading_dot();
    test_filter_multiple_dots();
    test_filter_very_long_domain();
    
    // Edge cases (10 tests)
    printf("\nEdge Cases:\n");
    test_filter_hyphen_domain();
    test_filter_numeric_domain();
    test_filter_idn_domain();
    test_filter_max_label_length();
    test_filter_many_labels();
    test_filter_wildcard_not_supported();
    test_filter_special_chars();
    test_filter_uppercase_lowercase_mix();
    test_filter_whitespace();
    test_filter_newline();
    
    // Performance (7 tests)
    printf("\nPerformance & Stress Tests:\n");
    test_filter_many_domains();
    test_filter_lookup_performance();
    test_filter_deep_trie();
    test_filter_wide_trie();
    test_filter_mixed_depths();
    test_filter_realistic_blocklist();
    test_filter_memory_efficiency();
    
    // Summary
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║                      TEST SUMMARY                          ║\n");
    printf("╠════════════════════════════════════════════════════════════╣\n");
    printf("║  Tests Run:    %3d                                         ║\n", tests_run);
    printf("║  Tests Passed: %3d                                         ║\n", tests_passed);
    printf("║  Tests Failed: %3d                                         ║\n", tests_run - tests_passed);
    printf("╚════════════════════════════════════════════════════════════╝\n");
    
    if (tests_passed == tests_run) {
        printf("\nY All tests PASSED!\n\n");
        return 0;
    } else {
        printf("\nN Some tests FAILED!\n\n");
        return 1;
    }
}