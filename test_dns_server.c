/**
 * @file test_dns_server.c
 * @brief Test program pre DNS server
 * 
 * Tento test:
 * 1. Vytvorí test filter súbor
 * 2. Spustí DNS server v separátnom procese
 * 3. Pošle testovacie DNS dotazy
 * 4. Overí odpovede
 */

 #include "dns.h"
 #include "dns_parser.h"
 #include "dns_builder.h"
 
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
 
 #define TEST_PORT 15353  /* Neprivilegovaný port pre testovanie */
 #define TEST_SERVER "127.0.0.1"
 
 /**
  * @brief Vytvorí test DNS query packet
  */
 int create_test_query(const char *domain, uint16_t qtype, 
                       uint8_t *buffer, size_t buf_len, size_t *query_len) {
     if (domain == NULL || buffer == NULL || query_len == NULL) {
         return -1;
     }
     
     size_t pos = 0;
     
     /* Header */
     if (pos + DNS_HEADER_SIZE > buf_len) return -1;
     
     /* Transaction ID */
     *(uint16_t *)(buffer + pos) = htons(0x1234);
     pos += 2;
     
     /* Flags: Standard query, RD=1 */
     *(uint16_t *)(buffer + pos) = htons(0x0100);
     pos += 2;
     
     /* QDCOUNT = 1 */
     *(uint16_t *)(buffer + pos) = htons(1);
     pos += 2;
     
     /* ANCOUNT = 0 */
     *(uint16_t *)(buffer + pos) = htons(0);
     pos += 2;
     
     /* NSCOUNT = 0 */
     *(uint16_t *)(buffer + pos) = htons(0);
     pos += 2;
     
     /* ARCOUNT = 0 */
     *(uint16_t *)(buffer + pos) = htons(0);
     pos += 2;
     
     /* QNAME - encode domain */
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
     
     /* Terminating zero */
     if (pos + 1 > buf_len) return -1;
     buffer[pos++] = 0;
     
     /* QTYPE */
     if (pos + 2 > buf_len) return -1;
     *(uint16_t *)(buffer + pos) = htons(qtype);
     pos += 2;
     
     /* QCLASS = IN */
     if (pos + 2 > buf_len) return -1;
     *(uint16_t *)(buffer + pos) = htons(DNS_CLASS_IN);
     pos += 2;
     
     *query_len = pos;
     return 0;
 }
 
 /**
  * @brief Pošle DNS query a prijme odpoveď
  */
 int send_dns_query(const char *server, uint16_t port,
                    const uint8_t *query, size_t query_len,
                    uint8_t *response, size_t resp_buf_len, size_t *resp_len) {
     int sockfd;
     struct sockaddr_in server_addr;
     struct timeval timeout;
     
     /* Vytvorenie socketu */
     sockfd = socket(AF_INET, SOCK_DGRAM, 0);
     if (sockfd < 0) {
         return -1;
     }
     
     /* Timeout 2 sekundy */
     timeout.tv_sec = 2;
     timeout.tv_usec = 0;
     setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
     
     /* Server adresa */
     memset(&server_addr, 0, sizeof(server_addr));
     server_addr.sin_family = AF_INET;
     server_addr.sin_port = htons(port);
     inet_pton(AF_INET, server, &server_addr.sin_addr);
     
     /* Odoslanie dotazu */
     ssize_t sent = sendto(sockfd, query, query_len, 0,
                          (struct sockaddr *)&server_addr, sizeof(server_addr));
     if (sent < 0 || (size_t)sent != query_len) {
         close(sockfd);
         return -1;
     }
     
     /* Prijatie odpovede */
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
  * @brief Testuje DNS server
  */
 int main() {
     printf("==============================================\n");
     printf("DNS Server Functional Test\n");
     printf("==============================================\n");
     
     printf("\n[INFO] Server should be running on %s:%d\n", TEST_SERVER, TEST_PORT);
     printf("[INFO] Start server manually:\n");
     printf("  ./dns -s 8.8.8.8 -p %d -f test_filter2.txt -v\n", TEST_PORT);
     printf("\nPress Enter when server is ready...");
     getchar();
     
     int tests_passed = 0;
     int tests_failed = 0;
     
     /* Test 1: Blokovaná doména (ads.google.com) */
     printf("\n[TEST 1] Blocked domain: ads.google.com\n");
     {
         uint8_t query[256], response[512];
         size_t query_len, resp_len;
         
         if (create_test_query("ads.google.com", DNS_TYPE_A, 
                              query, sizeof(query), &query_len) == 0) {
             printf("  Query created (%zu bytes)\n", query_len);
             
             if (send_dns_query(TEST_SERVER, TEST_PORT, query, query_len,
                               response, sizeof(response), &resp_len) == 0) {
                 printf("  Response received (%zu bytes)\n", resp_len);
                 
                 dns_message_t parsed;
                 if (parse_dns_message(response, resp_len, &parsed) == 0) {
                     uint16_t rcode = parsed.header.flags & 0x0F;
                     
                     if (rcode == DNS_RCODE_NXDOMAIN) {
                         printf("  ✓ PASS: Got NXDOMAIN (blocked)\n");
                         tests_passed++;
                     } else {
                         printf("  ✗ FAIL: Expected NXDOMAIN, got RCODE=%u\n", rcode);
                         tests_failed++;
                     }
                     
                     free_dns_message(&parsed);
                 } else {
                     printf("  ✗ FAIL: Failed to parse response\n");
                     tests_failed++;
                 }
             } else {
                 printf("  ✗ FAIL: No response (timeout?)\n");
                 tests_failed++;
             }
         }
     }
     
     /* Test 2: Neimplementovaný typ (AAAA) */
     printf("\n[TEST 2] Unsupported type: google.com AAAA\n");
     {
         uint8_t query[256], response[512];
         size_t query_len, resp_len;
         
         if (create_test_query("google.com", DNS_TYPE_AAAA,
                              query, sizeof(query), &query_len) == 0) {
             printf("  Query created (%zu bytes)\n", query_len);
             
             if (send_dns_query(TEST_SERVER, TEST_PORT, query, query_len,
                               response, sizeof(response), &resp_len) == 0) {
                 printf("  Response received (%zu bytes)\n", resp_len);
                 
                 dns_header_t header;
                 if (parse_dns_header(response, resp_len, &header) == 0) {
                     uint16_t rcode = header.flags & 0x0F;
                     
                     if (rcode == DNS_RCODE_NOTIMPL) {
                         printf("  ✓ PASS: Got NOTIMPL\n");
                         tests_passed++;
                     } else {
                         printf("  ✗ FAIL: Expected NOTIMPL, got RCODE=%u\n", rcode);
                         tests_failed++;
                     }
                 } else {
                     printf("  ✗ FAIL: Failed to parse response\n");
                     tests_failed++;
                 }
             } else {
                 printf("  ✗ FAIL: No response\n");
                 tests_failed++;
             }
         }
     }
     
     /* Test 3: Povolená doména (zatiaľ SERVFAIL, lebo resolver nie je implementovaný) */
     printf("\n[TEST 3] Allowed domain: google.com (SERVFAIL expected, resolver not implemented)\n");
     {
         uint8_t query[256], response[512];
         size_t query_len, resp_len;
         
         if (create_test_query("google.com", DNS_TYPE_A,
                              query, sizeof(query), &query_len) == 0) {
             printf("  Query created (%zu bytes)\n", query_len);
             
             if (send_dns_query(TEST_SERVER, TEST_PORT, query, query_len,
                               response, sizeof(response), &resp_len) == 0) {
                 printf("  Response received (%zu bytes)\n", resp_len);
                 
                 dns_header_t header;
                 if (parse_dns_header(response, resp_len, &header) == 0) {
                     uint16_t rcode = header.flags & 0x0F;
                     
                     if (rcode == DNS_RCODE_SERVFAIL) {
                         printf("  ✓ PASS: Got SERVFAIL (expected, resolver stub)\n");
                         tests_passed++;
                     } else {
                         printf("  ✗ FAIL: Expected SERVFAIL, got RCODE=%u\n", rcode);
                         tests_failed++;
                     }
                 } else {
                     printf("  ✗ FAIL: Failed to parse response\n");
                     tests_failed++;
                 }
             } else {
                 printf("  ✗ FAIL: No response\n");
                 tests_failed++;
             }
         }
     }
     
     /* Results */
     printf("\n==============================================\n");
     printf("TEST RESULTS:\n");
     printf("  Passed: %d\n", tests_passed);
     printf("  Failed: %d\n", tests_failed);
     printf("  Total:  %d\n", tests_passed + tests_failed);
     printf("==============================================\n");
     
     if (tests_failed == 0) {
         printf("✓ All tests passed!\n");
         return 0;
     } else {
         printf("✗ Some tests failed!\n");
         return 1;
     }
 }