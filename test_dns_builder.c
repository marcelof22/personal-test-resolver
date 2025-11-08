/**
 * @file test_dns_builder.c
 * @brief Unit testy pre DNS builder modul
 */

 #include "dns.h"
 #include "dns_parser.h"
 #include "dns_builder.h"
 #include "utils.h"
 
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <assert.h>
 #include <arpa/inet.h>
 
 /* ANSI farby */
 #define COLOR_GREEN "\033[32m"
 #define COLOR_RED "\033[31m"
 #define COLOR_RESET "\033[0m"
 
 int tests_passed = 0;
 int tests_failed = 0;
 
 #define TEST_PASS(msg) do { \
     printf("  " COLOR_GREEN "✓" COLOR_RESET " %s\n", msg); \
     tests_passed++; \
 } while(0)
 
 #define TEST_FAIL(msg) do { \
     printf("  " COLOR_RED "✗" COLOR_RESET " %s\n", msg); \
     tests_failed++; \
 } while(0)
 
 /**
  * @brief Test encode_dns_name
  */
 void test_encode_dns_name() {
     printf("\n[TEST] encode_dns_name()\n");
     
     uint8_t buffer[256];
     int len;
     
     /* Test 1: Jednoduchá doména "google.com" */
     len = encode_dns_name("google.com", buffer, sizeof(buffer));
     if (len == 12) {  /* 6google3com0 = 1+6+1+3+1 = 12 */
         if (buffer[0] == 6 && 
             memcmp(&buffer[1], "google", 6) == 0 &&
             buffer[7] == 3 &&
             memcmp(&buffer[8], "com", 3) == 0 &&
             buffer[11] == 0) {
             TEST_PASS("Encoding 'google.com'");
         } else {
             TEST_FAIL("Nesprávny formát 'google.com'");
         }
     } else {
         TEST_FAIL("Nesprávna dĺžka pre 'google.com'");
         printf("    Očakávané: 12, dostali sme: %d\n", len);
     }
     
     /* Test 2: Subdoména "www.example.org" */
     len = encode_dns_name("www.example.org", buffer, sizeof(buffer));
     if (len == 17) {  /* 3www7example3org0 */
         if (buffer[0] == 3 &&
             memcmp(&buffer[1], "www", 3) == 0 &&
             buffer[4] == 7 &&
             memcmp(&buffer[5], "example", 7) == 0 &&
             buffer[12] == 3 &&
             memcmp(&buffer[13], "org", 3) == 0 &&
             buffer[16] == 0) {
             TEST_PASS("Encoding 'www.example.org'");
         } else {
             TEST_FAIL("Nesprávny formát 'www.example.org'");
         }
     } else {
         TEST_FAIL("Nesprávna dĺžka pre 'www.example.org'");
     }
     
     /* Test 3: Root doména (prázdny string) */
     len = encode_dns_name("", buffer, sizeof(buffer));
     if (len == 1 && buffer[0] == 0) {
         TEST_PASS("Encoding root domain");
     } else {
         TEST_FAIL("Nesprávne kódovanie root domain");
     }
     
     /* Test 4: Label príliš dlhý (> 63 znakov) */
     char long_label[100];
     memset(long_label, 'a', 64);
     long_label[64] = '\0';
     strcat(long_label, ".com");
     
     len = encode_dns_name(long_label, buffer, sizeof(buffer));
     if (len == -1) {
         TEST_PASS("Odmietnutie príliš dlhého labelu");
     } else {
         TEST_FAIL("Mal odmietnuť príliš dlhý label");
     }
     
     /* Test 5: Buffer príliš malý */
     len = encode_dns_name("example.com", buffer, 5);
     if (len == -1) {
         TEST_PASS("Buffer overflow protection");
     } else {
         TEST_FAIL("Mal detekovať buffer overflow");
     }
     
     /* Test 6: NULL domain */
     len = encode_dns_name(NULL, buffer, sizeof(buffer));
     if (len == -1) {
         TEST_PASS("NULL domain handling");
     } else {
         TEST_FAIL("Mal odmietnuť NULL domain");
     }
     
     /* Test 7: NULL buffer */
     len = encode_dns_name("example.com", NULL, 100);
     if (len == -1) {
         TEST_PASS("NULL buffer handling");
     } else {
         TEST_FAIL("Mal odmietnuť NULL buffer");
     }
     
     /* Test 8: Trailing dot */
     len = encode_dns_name("example.com.", buffer, sizeof(buffer));
     if (len > 0 && buffer[len-1] == 0) {
         TEST_PASS("Handling trailing dot");
     } else {
         TEST_FAIL("Nesprávne spracovanie trailing dot");
     }
 }
 
 /**
  * @brief Test build_error_response
  */
 void test_build_error_response() {
     printf("\n[TEST] build_error_response()\n");
     
     /* Vytvorenie test query */
     uint8_t query_buffer[] = {
         /* Header */
         0xAB, 0xCD,        /* ID */
         0x01, 0x00,        /* Flags: RD=1 */
         0x00, 0x01,        /* QDCOUNT = 1 */
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         
         /* Question: example.com, type A */
         7, 'e', 'x', 'a', 'm', 'p', 'l', 'e',
         3, 'c', 'o', 'm',
         0,
         0x00, 0x01,        /* Type A */
         0x00, 0x01         /* Class IN */
     };
     
     dns_message_t query;
     if (parse_dns_message(query_buffer, sizeof(query_buffer), &query) != 0) {
         TEST_FAIL("Parsing test query failed");
         return;
     }
     
     /* Test 1: NXDOMAIN response (blokovaná doména) */
     uint8_t *response = NULL;
     size_t resp_len = 0;
     
     if (build_error_response(&query, DNS_RCODE_NXDOMAIN, &response, &resp_len) == 0) {
         if (response != NULL && resp_len >= DNS_HEADER_SIZE) {
             /* Parse response header */
             dns_header_t resp_header;
             parse_dns_header(response, resp_len, &resp_header);
             
             /* Check flags */
             uint16_t qr = (resp_header.flags & DNS_FLAG_QR) ? 1 : 0;
             uint16_t rd = (resp_header.flags & DNS_FLAG_RD) ? 1 : 0;
             uint16_t rcode = resp_header.flags & 0x0F;
             
             if (resp_header.id == 0xABCD &&
                 qr == 1 &&
                 rd == 1 &&
                 rcode == DNS_RCODE_NXDOMAIN &&
                 resp_header.qdcount == 1 &&
                 resp_header.ancount == 0) {
                 TEST_PASS("NXDOMAIN response");
             } else {
                 TEST_FAIL("Nesprávne hodnoty v NXDOMAIN response");
                 printf("    ID: 0x%04X (očakávané 0xABCD)\n", resp_header.id);
                 printf("    QR: %u (očakávané 1)\n", qr);
                 printf("    RD: %u (očakávané 1)\n", rd);
                 printf("    RCODE: %u (očakávané %u)\n", rcode, DNS_RCODE_NXDOMAIN);
             }
             
             free(response);
         } else {
             TEST_FAIL("Neplatná NXDOMAIN response");
         }
     } else {
         TEST_FAIL("Build NXDOMAIN response failed");
     }
     
     /* Test 2: NOTIMPL response (neimplementovaný typ) */
     response = NULL;
     resp_len = 0;
     
     if (build_error_response(&query, DNS_RCODE_NOTIMPL, &response, &resp_len) == 0) {
         dns_header_t resp_header;
         parse_dns_header(response, resp_len, &resp_header);
         
         uint16_t rcode = resp_header.flags & 0x0F;
         
         if (rcode == DNS_RCODE_NOTIMPL) {
             TEST_PASS("NOTIMPL response");
         } else {
             TEST_FAIL("Nesprávny RCODE v NOTIMPL response");
         }
         
         free(response);
     } else {
         TEST_FAIL("Build NOTIMPL response failed");
     }
     
     /* Test 3: FORMERR response (chyba formátu) */
     response = NULL;
     resp_len = 0;
     
     if (build_error_response(&query, DNS_RCODE_FORMERR, &response, &resp_len) == 0) {
         dns_header_t resp_header;
         parse_dns_header(response, resp_len, &resp_header);
         
         uint16_t rcode = resp_header.flags & 0x0F;
         
         if (rcode == DNS_RCODE_FORMERR) {
             TEST_PASS("FORMERR response");
         } else {
             TEST_FAIL("Nesprávny RCODE v FORMERR response");
         }
         
         free(response);
     } else {
         TEST_FAIL("Build FORMERR response failed");
     }
     
     /* Test 4: NULL query */
     response = NULL;
     resp_len = 0;
     
     if (build_error_response(NULL, DNS_RCODE_NXDOMAIN, &response, &resp_len) == -1) {
         TEST_PASS("NULL query handling");
     } else {
         TEST_FAIL("Mal odmietnuť NULL query");
         if (response) free(response);
     }
     
     /* Test 5: Neplatný RCODE */
     response = NULL;
     resp_len = 0;
     
     if (build_error_response(&query, 99, &response, &resp_len) == -1) {
         TEST_PASS("Invalid RCODE handling");
     } else {
         TEST_FAIL("Mal odmietnuť neplatný RCODE");
         if (response) free(response);
     }
     
     /* Cleanup */
     free_dns_message(&query);
 }
 
 /**
  * @brief Test round-trip: encode -> decode
  */
 void test_roundtrip() {
     printf("\n[TEST] Round-trip encoding/decoding\n");
     
     /* Test domény */
     const char *test_domains[] = {
         "google.com",
         "www.example.org",
         "mail.server.example.com",
         "a.b.c.d.e.f.g"
     };
     
     for (size_t i = 0; i < sizeof(test_domains) / sizeof(test_domains[0]); i++) {
         uint8_t buffer[256];
         char decoded[256];
         
         /* Encode */
         int enc_len = encode_dns_name(test_domains[i], buffer, sizeof(buffer));
         if (enc_len < 0) {
             TEST_FAIL("Round-trip encode failed");
             printf("    Domain: %s\n", test_domains[i]);
             continue;
         }
         
         /* Decode */
         size_t offset = 0;
         if (parse_dns_name(buffer, (size_t)enc_len, &offset, decoded, sizeof(decoded)) != 0) {
             TEST_FAIL("Round-trip decode failed");
             printf("    Domain: %s\n", test_domains[i]);
             continue;
         }
         
         /* Compare */
         if (strcmp(test_domains[i], decoded) == 0) {
             TEST_PASS("Round-trip OK");
             printf("    Domain: %s\n", test_domains[i]);
         } else {
             TEST_FAIL("Round-trip mismatch");
             printf("    Original: %s\n", test_domains[i]);
             printf("    Decoded:  %s\n", decoded);
         }
     }
 }
 
 /**
  * @brief Test build_dns_header
  */
 void test_build_dns_header() {
     printf("\n[TEST] build_dns_header()\n");
     
     dns_header_t header = {
         .id = 0x1234,
         .flags = 0x8180,  /* QR=1, RD=1, RA=1 */
         .qdcount = 1,
         .ancount = 0,
         .nscount = 0,
         .arcount = 0
     };
     
     uint8_t buffer[DNS_HEADER_SIZE];
     
     /* Test 1: Platný header */
     if (build_dns_header(buffer, &header) == 0) {
         /* Parse späť */
         dns_header_t parsed;
         if (parse_dns_header(buffer, sizeof(buffer), &parsed) == 0) {
             if (parsed.id == 0x1234 &&
                 parsed.flags == 0x8180 &&
                 parsed.qdcount == 1) {
                 TEST_PASS("build_dns_header");
             } else {
                 TEST_FAIL("Nesprávne hodnoty v build_dns_header");
             }
         } else {
             TEST_FAIL("Parse built header failed");
         }
     } else {
         TEST_FAIL("build_dns_header failed");
     }
     
     /* Test 2: NULL buffer */
     if (build_dns_header(NULL, &header) == -1) {
         TEST_PASS("NULL buffer handling");
     } else {
         TEST_FAIL("Mal odmietnuť NULL buffer");
     }
     
     /* Test 3: NULL header */
     if (build_dns_header(buffer, NULL) == -1) {
         TEST_PASS("NULL header handling");
     } else {
         TEST_FAIL("Mal odmietnuť NULL header");
     }
 }
 
 /**
  * @brief Main test runner
  */
 int main() {
     printf("==============================================\n");
     printf("DNS Builder Module Unit Tests\n");
     printf("==============================================\n");
     
     test_encode_dns_name();
     test_build_error_response();
     test_roundtrip();
     test_build_dns_header();
     
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
         printf(COLOR_GREEN "✓ All tests passed!" COLOR_RESET "\n");
         return 0;
     } else {
         printf(COLOR_RED "✗ Some tests failed!" COLOR_RESET "\n");
         return 1;
     }
 }