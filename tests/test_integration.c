/**
 * @file test_integration.c
 * @author Marcel (xlogin00)
 * @date November 2025
 * @brief Integration tests for DNS resolver
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "dns_server.h"
#include "dns_parser.h"
#include "dns_builder.h"
#include "filter.h"
#include "resolver.h"

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
        printf("N FAILED: %s\n", msg); \
    } while(0)

// ============================================================================
// INTEGRATION TEST 1: Full Pipeline - Blocked Domain
// ============================================================================

void test_integration_blocked_domain() {
    TEST("Full pipeline - blocked domain");
    
    // Setup filter
    filter_t *filter = filter_init();
    assert(filter != NULL);
    filter_insert(filter, "ads.google.com");
    
    // Create DNS query for blocked domain
    uint8_t query[512];
    uint16_t query_len = 0;
    
    // Build simple A query for ads.google.com
    // Transaction ID
    query[0] = 0x12;
    query[1] = 0x34;
    
    // Flags: standard query, recursion desired
    query[2] = 0x01;
    query[3] = 0x00;
    
    // Questions: 1
    query[4] = 0x00;
    query[5] = 0x01;
    
    // Answer RRs: 0
    query[6] = 0x00;
    query[7] = 0x00;
    
    // Authority RRs: 0
    query[8] = 0x00;
    query[9] = 0x00;
    
    // Additional RRs: 0
    query[10] = 0x00;
    query[11] = 0x00;
    
    // QNAME: ads.google.com
    query_len = 12;
    query[query_len++] = 3;
    memcpy(&query[query_len], "ads", 3); query_len += 3;
    query[query_len++] = 6;
    memcpy(&query[query_len], "google", 6); query_len += 6;
    query[query_len++] = 3;
    memcpy(&query[query_len], "com", 3); query_len += 3;
    query[query_len++] = 0;
    
    // QTYPE: A (1)
    query[query_len++] = 0x00;
    query[query_len++] = 0x01;
    
    // QCLASS: IN (1)
    query[query_len++] = 0x00;
    query[query_len++] = 0x01;
    
    // Parse query
    dns_query_t parsed_query;
    int parse_result = dns_parse_query(query, query_len, &parsed_query);
    assert(parse_result == 0);
    assert(strcmp(parsed_query.qname, "ads.google.com") == 0);
    
    // Check filter
    bool blocked = filter_lookup(filter, parsed_query.qname);
    assert(blocked == true);
    
    // Build NXDOMAIN response
    uint8_t response[512];
    uint16_t response_len = 0;
    
    response_len = dns_build_error_response(
        query,
        query_len,
        response,
        sizeof(response),
        DNS_RCODE_NXDOMAIN
    );
    
    assert(response_len > 0);
    
    // Verify response
    assert(response[0] == 0x12);  // Transaction ID preserved
    assert(response[1] == 0x34);
    assert((response[3] & 0x0F) == DNS_RCODE_NXDOMAIN);  // RCODE = 3
    
    filter_free(filter);
    PASS();
}

// ============================================================================
// INTEGRATION TEST 2: Full Pipeline - Allowed Domain
// ============================================================================

void test_integration_allowed_domain() {
    TEST("Full pipeline - allowed domain");
    
    // Setup filter
    filter_t *filter = filter_init();
    assert(filter != NULL);
    filter_insert(filter, "ads.google.com");
    
    // Create DNS query for allowed domain
    uint8_t query[512];
    uint16_t query_len = 0;
    
    // Build A query for example.com
    query[0] = 0x56;
    query[1] = 0x78;
    query[2] = 0x01;
    query[3] = 0x00;
    query[4] = 0x00;
    query[5] = 0x01;
    query[6] = 0x00;
    query[7] = 0x00;
    query[8] = 0x00;
    query[9] = 0x00;
    query[10] = 0x00;
    query[11] = 0x00;
    
    query_len = 12;
    query[query_len++] = 7;
    memcpy(&query[query_len], "example", 7); query_len += 7;
    query[query_len++] = 3;
    memcpy(&query[query_len], "com", 3); query_len += 3;
    query[query_len++] = 0;
    
    query[query_len++] = 0x00;
    query[query_len++] = 0x01;
    query[query_len++] = 0x00;
    query[query_len++] = 0x01;
    
    // Parse query
    dns_query_t parsed_query;
    int parse_result = dns_parse_query(query, query_len, &parsed_query);
    assert(parse_result == 0);
    assert(strcmp(parsed_query.qname, "example.com") == 0);
    
    // Check filter
    bool blocked = filter_lookup(filter, parsed_query.qname);
    assert(blocked == false);
    
    // In real scenario, would forward to upstream
    // Here we just verify it's not blocked
    
    filter_free(filter);
    PASS();
}

// ============================================================================
// INTEGRATION TEST 3: Subdomain Blocking
// ============================================================================

void test_integration_subdomain_blocking() {
    TEST("Subdomain blocking integration");
    
    // Setup filter with parent domain
    filter_t *filter = filter_init();
    assert(filter != NULL);
    filter_insert(filter, "ads.google.com");
    
    // Create query for subdomain
    uint8_t query[512];
    uint16_t query_len = 0;
    
    // Build A query for tracker.ads.google.com
    query[0] = 0xAB;
    query[1] = 0xCD;
    query[2] = 0x01;
    query[3] = 0x00;
    query[4] = 0x00;
    query[5] = 0x01;
    query[6] = 0x00;
    query[7] = 0x00;
    query[8] = 0x00;
    query[9] = 0x00;
    query[10] = 0x00;
    query[11] = 0x00;
    
    query_len = 12;
    query[query_len++] = 7;
    memcpy(&query[query_len], "tracker", 7); query_len += 7;
    query[query_len++] = 3;
    memcpy(&query[query_len], "ads", 3); query_len += 3;
    query[query_len++] = 6;
    memcpy(&query[query_len], "google", 6); query_len += 6;
    query[query_len++] = 3;
    memcpy(&query[query_len], "com", 3); query_len += 3;
    query[query_len++] = 0;
    
    query[query_len++] = 0x00;
    query[query_len++] = 0x01;
    query[query_len++] = 0x00;
    query[query_len++] = 0x01;
    
    // Parse
    dns_query_t parsed_query;
    int parse_result = dns_parse_query(query, query_len, &parsed_query);
    assert(parse_result == 0);
    assert(strcmp(parsed_query.qname, "tracker.ads.google.com") == 0);
    
    // Verify subdomain is blocked
    bool blocked = filter_lookup(filter, parsed_query.qname);
    assert(blocked == true);
    
    // Build NXDOMAIN response
    uint8_t response[512];
    uint16_t response_len = dns_build_error_response(
        query,
        query_len,
        response,
        sizeof(response),
        DNS_RCODE_NXDOMAIN
    );
    
    assert(response_len > 0);
    assert((response[3] & 0x0F) == DNS_RCODE_NXDOMAIN);
    
    filter_free(filter);
    PASS();
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main(void) {
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║           DNS Resolver - Integration Tests                ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n\n");
    
    printf("Running integration tests...\n\n");
    
    test_integration_blocked_domain();
    test_integration_allowed_domain();
    test_integration_subdomain_blocking();
    
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
        printf("\nY All integration tests PASSED!\n\n");
        return 0;
    } else {
        printf("\nN Some integration tests FAILED!\n\n");
        return 1;
    }
}