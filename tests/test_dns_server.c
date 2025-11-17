/**
 * @file test_dns_server.c
 * @author Marcel Feiler (xfeile00)
 * @date 10.11.2025
 * @brief Comprehensive DNS Server Tests
 */

#include "dns.h"
#include "dns_parser.h"
#include "dns_builder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define TEST_PORT 15353
#define TEST_SERVER "127.0.0.1"
#define TIMEOUT_SEC 2

// Test counters
static int tests_passed = 0;
static int tests_failed = 0;
static int test_num = 0;

// Color output
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_BLUE "\033[0;34m"
#define COLOR_RESET "\033[0m"

/**
 * Tvorba test DNS query paketu
 */
int create_test_query(const char *domain, uint16_t qtype, 
                      uint8_t *buffer, size_t buf_len, size_t *query_len) {
    if (domain == NULL || buffer == NULL || query_len == NULL) {
        return -1;
    }
    
    size_t pos = 0;
    
    // Header
    if (pos + DNS_HEADER_SIZE > buf_len) return -1;
    
    // Transaction ID
    *(uint16_t *)(buffer + pos) = htons(0x1234);
    pos += 2;
    
    // Flags: Standard query, RD=1
    *(uint16_t *)(buffer + pos) = htons(0x0100);
    pos += 2;
    
    // QDCOUNT = 1
    *(uint16_t *)(buffer + pos) = htons(1);
    pos += 2;
    
    // ANCOUNT = 0
    *(uint16_t *)(buffer + pos) = htons(0);
    pos += 2;
    
    // NSCOUNT = 0
    *(uint16_t *)(buffer + pos) = htons(0);
    pos += 2;
    
    // ARCOUNT = 0
    *(uint16_t *)(buffer + pos) = htons(0);
    pos += 2;
    
    // QNAME - encode domain
    const char *label = domain;
    while (*label) {
        const char *dot = strchr(label, '.');
        size_t len = dot ? (size_t)(dot - label) : strlen(label);
        
        if (len > 63 || pos + 1 + len > buf_len) return -1;
        
        buffer[pos++] = (uint8_t)len;
        memcpy(buffer + pos, label, len);
        pos += len;
        
        if (dot) {
            label = dot + 1;
        } else {
            break;
        }
    }
    
    // Terminating zero
    if (pos + 1 > buf_len) return -1;
    buffer[pos++] = 0;
    
    // QTYPE
    if (pos + 2 > buf_len) return -1;
    *(uint16_t *)(buffer + pos) = htons(qtype);
    pos += 2;
    
    // QCLASS = IN
    if (pos + 2 > buf_len) return -1;
    *(uint16_t *)(buffer + pos) = htons(DNS_CLASS_IN);
    pos += 2;
    
    *query_len = pos;
    return 0;
}

/**
 * Send DNS query and receive response
 */
int send_dns_query(const char *server, uint16_t port,
                   const uint8_t *query, size_t query_len,
                   uint8_t *response, size_t resp_buf_len, size_t *resp_len) {
    int sockfd;
    struct sockaddr_in server_addr;
    struct timeval timeout;
    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        return -1;
    }
    
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, server, &server_addr.sin_addr);
    
    ssize_t sent = sendto(sockfd, query, query_len, 0,
                         (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (sent < 0 || (size_t)sent != query_len) {
        close(sockfd);
        return -1;
    }
    
    socklen_t addr_len = sizeof(server_addr);
    ssize_t received = recvfrom(sockfd, response, resp_buf_len, 0,
                               (struct sockaddr *)&server_addr, &addr_len);
    
    close(sockfd);
    
    if (received < 0) {
        return -1;
    }
    
    *resp_len = (size_t)received;
    return 0;
}

/**
 * Get RCODE from DNS response
 */
int get_rcode(const uint8_t *response, size_t resp_len) {
    if (resp_len < DNS_HEADER_SIZE) {
        return -1;
    }
    
    uint16_t flags = ntohs(*(uint16_t *)(response + 2));
    return flags & 0x0F;
}

/**
 * Test helper - expects RCODE
 */
void test_domain_expects_rcode(const char *domain, uint16_t qtype, 
                               int expected_rcode, const char *test_name) {
    test_num++;
    printf("\n" COLOR_BLUE "[TEST %d]" COLOR_RESET " %s: %s (type %d)\n", 
           test_num, test_name, domain, qtype);
    
    uint8_t query[512], response[512];
    size_t query_len, resp_len;
    
    if (create_test_query(domain, qtype, query, sizeof(query), &query_len) != 0) {
        printf("  " COLOR_RED " FAIL" COLOR_RESET ": Failed to create query\n");
        tests_failed++;
        return;
    }
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    if (send_dns_query(TEST_SERVER, TEST_PORT, query, query_len,
                      response, sizeof(response), &resp_len) != 0) {
        printf("  " COLOR_RED " FAIL" COLOR_RESET ": No response (timeout?)\n");
        tests_failed++;
        return;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) * 1000.0 + 
                     (end.tv_nsec - start.tv_nsec) / 1000000.0;
    
    int rcode = get_rcode(response, resp_len);
    
    if (rcode == expected_rcode) {
        printf("  " COLOR_GREEN " PASS" COLOR_RESET 
               ": Got expected RCODE=%d (%.1f ms)\n", rcode, elapsed);
        tests_passed++;
    } else {
        printf("  " COLOR_RED " FAIL" COLOR_RESET 
               ": Expected RCODE=%d, got %d (%.1f ms)\n", 
               expected_rcode, rcode, elapsed);
        tests_failed++;
    }
}

/**
 * Test helper - ocakava rychlu odpoved (blocked locally)
 */
void test_blocked_domain_fast(const char *domain) {
    test_num++;
    printf("\n" COLOR_BLUE "[TEST %d]" COLOR_RESET " Fast blocking: %s\n", 
           test_num, domain);
    
    uint8_t query[512], response[512];
    size_t query_len, resp_len;
    
    if (create_test_query(domain, DNS_TYPE_A, query, sizeof(query), &query_len) != 0) {
        printf("  " COLOR_RED " FAIL" COLOR_RESET ": Failed to create query\n");
        tests_failed++;
        return;
    }
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    if (send_dns_query(TEST_SERVER, TEST_PORT, query, query_len,
                      response, sizeof(response), &resp_len) != 0) {
        printf("  " COLOR_RED " FAIL" COLOR_RESET ": No response\n");
        tests_failed++;
        return;
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) * 1000.0 + 
                     (end.tv_nsec - start.tv_nsec) / 1000000.0;
    
    int rcode = get_rcode(response, resp_len);
    
    if (rcode == DNS_RCODE_NXDOMAIN && elapsed < 5.0) {
        printf("  " COLOR_GREEN " PASS" COLOR_RESET 
               ": Blocked locally (%.1f ms < 5ms)\n", elapsed);
        tests_passed++;
    } else {
        printf("  " COLOR_RED " FAIL" COLOR_RESET 
               ": RCODE=%d, time=%.1f ms (expected fast NXDOMAIN)\n", 
               rcode, elapsed);
        tests_failed++;
    }
}

/**
 * Main test suite
 */
int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;
    
    printf("══════════════════════════════════════════════════════════════\n");
    printf("         DNS Server Comprehensive Test Suite\n");
    printf("══════════════════════════════════════════════════════════════\n");
    
    printf("\n" COLOR_YELLOW "[INFO]" COLOR_RESET " Server should be running on %s:%d\n", 
           TEST_SERVER, TEST_PORT);
    printf(COLOR_YELLOW "[INFO]" COLOR_RESET " Start server manually:\n");
    printf("  ./dns -s 8.8.8.8 -p %d -f filter_file2.txt -v\n", TEST_PORT);
    printf("\nPress Enter when server is ready...");
    getchar();
    
    printf("\n" COLOR_BLUE "════ BASIC FUNCTIONALITY TESTS ════" COLOR_RESET "\n");
    
    // Test 1: Simple blocked domain
    test_domain_expects_rcode("ads.google.com", DNS_TYPE_A, 
                             DNS_RCODE_NXDOMAIN, "Blocked domain");
    
    // Test 2: Another blocked domain
    test_domain_expects_rcode("doubleclick.net", DNS_TYPE_A,
                             DNS_RCODE_NXDOMAIN, "Blocked domain #2");
    
    // Test 3: Allowed domain (with upstream)
    test_domain_expects_rcode("google.com", DNS_TYPE_A,
                             DNS_RCODE_NOERROR, "Allowed domain (forwarded)");
    
    // Test 4: Another allowed domain
    test_domain_expects_rcode("github.com", DNS_TYPE_A,
                             DNS_RCODE_NOERROR, "Allowed domain #2");
    
    printf("\n" COLOR_BLUE "════ SUBDOMAIN BLOCKING TESTS ════" COLOR_RESET "\n");
    
    // Test 5: Subdomain of blocked domain
    test_domain_expects_rcode("tracker.ads.google.com", DNS_TYPE_A,
                             DNS_RCODE_NXDOMAIN, "Subdomain blocking");
    
    // Test 6: Deep subdomain
    test_domain_expects_rcode("x.y.z.ads.google.com", DNS_TYPE_A,
                             DNS_RCODE_NXDOMAIN, "Deep subdomain blocking");
    
    // Test 7: Parent of blocked domain (should be allowed)
    test_domain_expects_rcode("google.com", DNS_TYPE_A,
                             DNS_RCODE_NOERROR, "Parent domain allowed");
    
    // Test 8: Sibling of blocked domain (should be allowed)
    test_domain_expects_rcode("mail.google.com", DNS_TYPE_A,
                             DNS_RCODE_NOERROR, "Sibling domain allowed");
    
    printf("\n" COLOR_BLUE "════ CASE SENSITIVITY TESTS ════" COLOR_RESET "\n");
    
    // Test 9: Uppercase domain (DNS is case-insensitive)
    test_domain_expects_rcode("ADS.GOOGLE.COM", DNS_TYPE_A,
                             DNS_RCODE_NXDOMAIN, "Uppercase blocked domain");
    
    // Test 10: Mixed case
    test_domain_expects_rcode("DoUbLeClIcK.NeT", DNS_TYPE_A,
                             DNS_RCODE_NXDOMAIN, "Mixed case blocked domain");
    
    printf("\n" COLOR_BLUE "════ UNSUPPORTED TYPE TESTS ════" COLOR_RESET "\n");
    
    // Test 11: AAAA query (not implemented)
    test_domain_expects_rcode("google.com", DNS_TYPE_AAAA,
                             DNS_RCODE_NOTIMPL, "AAAA type (unsupported)");
    
    // Test 12: MX query (not implemented)
    test_domain_expects_rcode("google.com", DNS_TYPE_MX,
                             DNS_RCODE_NOTIMPL, "MX type (unsupported)");
    
    // Test 13: CNAME query (not implemented)
    test_domain_expects_rcode("google.com", DNS_TYPE_CNAME,
                             DNS_RCODE_NOTIMPL, "CNAME type (unsupported)");
    
    printf("\n" COLOR_BLUE "════ EDGE CASE TESTS ════" COLOR_RESET "\n");
    
    // Test 14: Single label domain
    test_domain_expects_rcode("single", DNS_TYPE_A,
                             DNS_RCODE_NXDOMAIN, "Single label (blocked)");
    
    // Test 15: Very long domain
    test_domain_expects_rcode("very.long.subdomain.with.many.labels.example.com", 
                             DNS_TYPE_A, DNS_RCODE_NXDOMAIN, "Long domain");
    
    // Test 16: Domain with hyphen
    test_domain_expects_rcode("test-hyphen.com", DNS_TYPE_A,
                             DNS_RCODE_NXDOMAIN, "Domain with hyphen");
    
    // Test 17: Domain with numbers
    test_domain_expects_rcode("test123.org", DNS_TYPE_A,
                             DNS_RCODE_NXDOMAIN, "Domain with numbers");
    
    printf("\n" COLOR_BLUE "════ PERFORMANCE TESTS ════" COLOR_RESET "\n");
    
    // Test 18-20: Fast blocking (should be < 5ms)
    test_blocked_domain_fast("ads.google.com");
    test_blocked_domain_fast("doubleclick.net");
    test_blocked_domain_fast("tracker.ads.google.com");
    
    printf("\n" COLOR_BLUE "════ MULTIPLE QUERY TESTS ════" COLOR_RESET "\n");
    
    // Test 21-25: Rapid fire queries
    for (int i = 0; i < 5; i++) {
        const char *domains[] = {
            "google.com",
            "ads.google.com",
            "github.com",
            "doubleclick.net",
            "example.com"
        };
        int expected[] = {
            DNS_RCODE_NOERROR,
            DNS_RCODE_NXDOMAIN,
            DNS_RCODE_NOERROR,
            DNS_RCODE_NXDOMAIN,
            DNS_RCODE_NOERROR
        };
        
        test_domain_expects_rcode(domains[i], DNS_TYPE_A, 
                                 expected[i], "Rapid fire query");
    }
    
    // Summary
    printf("\n══════════════════════════════════════════════════════════════\n");
    printf("                       TEST SUMMARY\n");
    printf("══════════════════════════════════════════════════════════════\n");
    printf("  Tests Run:    %d\n", tests_passed + tests_failed);
    printf("  " COLOR_GREEN "Tests Passed: %d" COLOR_RESET "\n", tests_passed);
    printf("  " COLOR_RED "Tests Failed: %d" COLOR_RESET "\n", tests_failed);
    printf("  Success Rate: %.1f%%\n", 
           (tests_passed * 100.0) / (tests_passed + tests_failed));
    printf("══════════════════════════════════════════════════════════════\n");
    
    if (tests_failed == 0) {
        printf("\n" COLOR_GREEN " All tests PASSED!" COLOR_RESET "\n\n");
        return 0;
    } else {
        printf("\n" COLOR_RED " Some tests FAILED!" COLOR_RESET "\n\n");
        return 1;
    }
}