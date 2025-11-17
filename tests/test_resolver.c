/**
 * @file test_resolver.c
 * @author Marcel Feiler (xfeile00)
 * @date 10.11.2025
 * @brief Filtering DNS Resolver
 */

 #include "dns.h"
 #include "dns_parser.h"
 #include "dns_builder.h"
 #include "resolver.h"
 
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <arpa/inet.h>
 #include <sys/socket.h>
 #include <netinet/in.h>
 
 #define COLOR_GREEN "\033[32m"
 #define COLOR_RED "\033[31m"
 #define COLOR_YELLOW "\033[33m"
 #define COLOR_RESET "\033[0m"
 
 int tests_passed = 0;
 int tests_failed = 0;
 
 #define TEST_PASS(msg) do { \
     printf("  " COLOR_GREEN "Y" COLOR_RESET " %s\n", msg); \
     tests_passed++; \
 } while(0)
 
 #define TEST_FAIL(msg) do { \
     printf("  " COLOR_RED "N" COLOR_RESET " %s\n", msg); \
     tests_failed++; \
 } while(0)
 
 #define TEST_SKIP(msg) do { \
     printf("  " COLOR_YELLOW "W" COLOR_RESET " %s (skipped)\n", msg); \
 } while(0)
 
 /**
  * @brief Test resolve_upstream_address
  */
 void test_resolve_upstream_address() {
     printf("\n[TEST] resolve_upstream_address()\n");
     
     char ip_addr[16];
     
     /* Test 1: IPv4 adresa */
     if (resolve_upstream_address("8.8.8.8", ip_addr) == 0) {
         if (strcmp(ip_addr, "8.8.8.8") == 0) {
             TEST_PASS("IPv4 address passthrough");
         } else {
             TEST_FAIL("IPv4 address mismatch");
             printf("    Expected: 8.8.8.8, Got: %s\n", ip_addr);
         }
     } else {
         TEST_FAIL("Failed to process IPv4 address");
     }
     
     /* Test 2: Hostname resolution */
     if (resolve_upstream_address("dns.google", ip_addr) == 0) {
         /* Overíme že je to platná IPv4 adresa */
         struct in_addr addr;
         if (inet_pton(AF_INET, ip_addr, &addr) == 1) {
             TEST_PASS("Hostname resolution (dns.google)");
             printf("    Resolved to: %s\n", ip_addr);
         } else {
             TEST_FAIL("Invalid IP address from hostname resolution");
         }
     } else {
         TEST_SKIP("Hostname resolution (network required)");
     }
     
     /* Test 3: NULL hostname */
     if (resolve_upstream_address(NULL, ip_addr) == -1) {
         TEST_PASS("NULL hostname handling");
     } else {
         TEST_FAIL("Should reject NULL hostname");
     }
     
     /* Test 4: NULL ip_addr */
     if (resolve_upstream_address("8.8.8.8", NULL) == -1) {
         TEST_PASS("NULL ip_addr buffer handling");
     } else {
         TEST_FAIL("Should reject NULL ip_addr");
     }
     
     /* Test 5: Neplatný hostname */
     if (resolve_upstream_address("this.domain.does.not.exist.invalid", ip_addr) == -1) {
         TEST_PASS("Invalid hostname rejection");
     } else {
         TEST_SKIP("Invalid hostname (network dependent)");
     }
 }
 
 /**
  * @brief Vytvorí test DNS query
  */
 int create_test_query_packet(const char *domain, uint8_t **buffer, size_t *len) {
     size_t buf_size = 512;
     uint8_t *buf = (uint8_t *)malloc(buf_size);
     if (!buf) return -1;
     
     size_t pos = 0;
     
     /* Header */
     *(uint16_t *)(buf + pos) = htons(0xABCD); pos += 2;
     *(uint16_t *)(buf + pos) = htons(0x0100); pos += 2;
     *(uint16_t *)(buf + pos) = htons(1); pos += 2;
     *(uint16_t *)(buf + pos) = htons(0); pos += 2;
     *(uint16_t *)(buf + pos) = htons(0); pos += 2;
     *(uint16_t *)(buf + pos) = htons(0); pos += 2;
     
     /* QNAME */
     const char *label = domain;
     while (*label) {
         const char *dot = strchr(label, '.');
         size_t label_len = dot ? (size_t)(dot - label) : strlen(label);
         
         buf[pos++] = (uint8_t)label_len;
         memcpy(buf + pos, label, label_len);
         pos += label_len;
         
         if (dot) label = dot + 1;
         else break;
     }
     buf[pos++] = 0;
     
     /* QTYPE, QCLASS */
     *(uint16_t *)(buf + pos) = htons(DNS_TYPE_A); pos += 2;
     *(uint16_t *)(buf + pos) = htons(DNS_CLASS_IN); pos += 2;
     
     *buffer = buf;
     *len = pos;
     return 0;
 }
 
 /**
  * @brief Test forward_query s reálnym upstream serverom
  */
 void test_forward_query() {
     printf("\n[TEST] forward_query()\n");
     
     /* Vytvorenie test query */
     uint8_t *query_buf = NULL;
     size_t query_len = 0;
     
     if (create_test_query_packet("google.com", &query_buf, &query_len) != 0) {
         TEST_FAIL("Failed to create test query");
         return;
     }
     
     /* Parse query */
     dns_message_t query;
     if (parse_dns_message(query_buf, query_len, &query) != 0) {
         TEST_FAIL("Failed to parse test query");
         free(query_buf);
         return;
     }
     
     /* Test 1: Forward na Google DNS (8.8.8.8) */
     printf("  Testing upstream forwarding to 8.8.8.8...\n");
     
     uint8_t *response = NULL;
     size_t resp_len = 0;
     
     if (forward_query(&query, "8.8.8.8", &response, &resp_len) == 0) {
         TEST_PASS("Forward to 8.8.8.8 successful");
         
         /* Validácia odpovede */
         if (response != NULL && resp_len >= DNS_HEADER_SIZE) {
             dns_header_t resp_header;
             if (parse_dns_header(response, resp_len, &resp_header) == 0) {
                 printf("    Response: %zu bytes, RCODE=%u, ANCOUNT=%u\n",
                        resp_len, 
                        resp_header.flags & 0x0F,
                        resp_header.ancount);
                 
                 /* Check Transaction ID */
                 if (resp_header.id == 0xABCD) {
                     TEST_PASS("Transaction ID preserved");
                 } else {
                     TEST_FAIL("Transaction ID mismatch");
                 }
                 
                 /* Check QR flag */
                 if (resp_header.flags & DNS_FLAG_QR) {
                     TEST_PASS("QR flag set (response)");
                 } else {
                     TEST_FAIL("QR flag not set");
                 }
                 
                 /* Check RCODE */
                 uint16_t rcode = resp_header.flags & 0x0F;
                 if (rcode == DNS_RCODE_NOERROR) {
                     TEST_PASS("RCODE is NOERROR");
                 } else {
                     printf("    Note: RCODE=%u (not NOERROR)\n", rcode);
                 }
             } else {
                 TEST_FAIL("Failed to parse response header");
             }
             
             free(response);
         } else {
             TEST_FAIL("Invalid response from upstream");
         }
     } else {
         TEST_SKIP("Forward to 8.8.8.8 (network required)");
     }
     
     /* Test 2: NULL query */
     response = NULL;
     resp_len = 0;
     
     if (forward_query(NULL, "8.8.8.8", &response, &resp_len) == -1) {
         TEST_PASS("NULL query handling");
     } else {
         TEST_FAIL("Should reject NULL query");
         if (response) free(response);
     }
     
     /* Test 3: NULL upstream */
     response = NULL;
     resp_len = 0;
     
     if (forward_query(&query, NULL, &response, &resp_len) == -1) {
         TEST_PASS("NULL upstream handling");
     } else {
         TEST_FAIL("Should reject NULL upstream");
         if (response) free(response);
     }
     
     /* Cleanup */
     free(query_buf);
     free_dns_message(&query);
 }
 
 /**
  * @brief Test create_upstream_socket
  */
 void test_create_upstream_socket() {
     printf("\n[TEST] create_upstream_socket()\n");
     
     int sockfd = create_upstream_socket();
     
     if (sockfd >= 0) {
         TEST_PASS("Socket created successfully");
         close(sockfd);
     } else {
         TEST_FAIL("Failed to create socket");
     }
 }
 
 /**
  * @brief Main test runner
  */
 int main() {
     printf("==============================================\n");
     printf("Upstream Resolver Module Unit Tests\n");
     printf("==============================================\n");
     printf("\n" COLOR_YELLOW "Note: Some tests require network connectivity" COLOR_RESET "\n");
     
     test_resolve_upstream_address();
     test_create_upstream_socket();
     test_forward_query();
     
     printf("\n==============================================\n");
     printf("TEST RESULTS:\n");
     printf("  " COLOR_GREEN "Passed: %d" COLOR_RESET "\n", tests_passed);
     if (tests_failed > 0) {
         printf("  " COLOR_RED "Failed: %d" COLOR_RESET "\n", tests_failed);
     } else {
         printf("  Failed: 0\n");
     }
     printf("  Total:  %d\n", tests_passed + tests_failed);
     printf("==============================================\n");
     
     if (tests_failed == 0) {
         printf(COLOR_GREEN " All tests passed!" COLOR_RESET "\n");
         return 0;
     } else {
         printf(COLOR_RED " Some tests failed!" COLOR_RESET "\n");
         return 1;
     }
 }