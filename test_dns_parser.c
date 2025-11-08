/**
 * @file test_dns_parser.c
 * @brief Unit testy pre DNS parser modul
 */

 #include "dns.h"
 #include "dns_parser.h"
 #include "utils.h"
 
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <assert.h>
 #include <arpa/inet.h>
 
 /* ANSI farby pre výstup */
 #define COLOR_GREEN "\033[32m"
 #define COLOR_RED "\033[31m"
 #define COLOR_YELLOW "\033[33m"
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
  * @brief Test parsing DNS header
  */
 void test_parse_dns_header() {
     printf("\n[TEST] parse_dns_header()\n");
     
     /* Vytvorenie test DNS hlavičky */
     uint8_t buffer[DNS_HEADER_SIZE] = {
         0x12, 0x34,  /* ID = 0x1234 */
         0x01, 0x00,  /* Flags: QR=0, RD=1 */
         0x00, 0x01,  /* QDCOUNT = 1 */
         0x00, 0x00,  /* ANCOUNT = 0 */
         0x00, 0x00,  /* NSCOUNT = 0 */
         0x00, 0x00   /* ARCOUNT = 0 */
     };
     
     dns_header_t header;
     
     /* Test 1: Platná hlavička */
     if (parse_dns_header(buffer, sizeof(buffer), &header) == 0) {
         if (header.id == 0x1234 && 
             header.flags == 0x0100 &&
             header.qdcount == 1 &&
             header.ancount == 0) {
             TEST_PASS("Platná DNS hlavička");
         } else {
             TEST_FAIL("Nesprávne hodnoty v hlavičke");
         }
     } else {
         TEST_FAIL("Parsing platnej hlavičky");
     }
     
     /* Test 2: Buffer príliš krátky */
     if (parse_dns_header(buffer, 10, &header) == -1) {
         TEST_PASS("Buffer príliš krátky");
     } else {
         TEST_FAIL("Mal odmietnuť krátky buffer");
     }
     
     /* Test 3: NULL buffer */
     if (parse_dns_header(NULL, sizeof(buffer), &header) == -1) {
         TEST_PASS("NULL buffer");
     } else {
         TEST_FAIL("Mal odmietnuť NULL buffer");
     }
     
     /* Test 4: NULL header */
     if (parse_dns_header(buffer, sizeof(buffer), NULL) == -1) {
         TEST_PASS("NULL header pointer");
     } else {
         TEST_FAIL("Mal odmietnuť NULL header");
     }
 }
 
 /**
  * @brief Test parsing DNS name (bez compression)
  */
 void test_parse_dns_name_simple() {
     printf("\n[TEST] parse_dns_name() - simple\n");
     
     /* Test: "www.google.com" */
     /* Format: 3www6google3com0 */
     uint8_t buffer[] = {
         3, 'w', 'w', 'w',
         6, 'g', 'o', 'o', 'g', 'l', 'e',
         3, 'c', 'o', 'm',
         0
     };
     
     char name[DNS_MAX_NAME_LEN + 1];
     size_t offset = 0;
     
     /* Test 1: Platné meno */
     if (parse_dns_name(buffer, sizeof(buffer), &offset, name, sizeof(name)) == 0) {
         if (strcmp(name, "www.google.com") == 0 && offset == sizeof(buffer)) {
             TEST_PASS("Parsing 'www.google.com'");
         } else {
             TEST_FAIL("Nesprávny výsledok");
             printf("    Očakávané: 'www.google.com', offset=%zu\n", sizeof(buffer));
             printf("    Dostali sme: '%s', offset=%zu\n", name, offset);
         }
     } else {
         TEST_FAIL("Parsing 'www.google.com' zlyhal");
     }
     
     /* Test 2: Jednoduchá doména "com" */
     uint8_t buffer2[] = { 3, 'c', 'o', 'm', 0 };
     offset = 0;
     
     if (parse_dns_name(buffer2, sizeof(buffer2), &offset, name, sizeof(name)) == 0) {
         if (strcmp(name, "com") == 0) {
             TEST_PASS("Parsing 'com'");
         } else {
             TEST_FAIL("Nesprávny výsledok pre 'com'");
         }
     } else {
         TEST_FAIL("Parsing 'com' zlyhal");
     }
     
     /* Test 3: Label príliš dlhý (> 63) */
     uint8_t buffer3[100];
     buffer3[0] = 64;  /* Neplatná dĺžka */
     for (int i = 1; i <= 64; i++) {
         buffer3[i] = 'a';
     }
     buffer3[65] = 0;
     offset = 0;
     
     if (parse_dns_name(buffer3, sizeof(buffer3), &offset, name, sizeof(name)) == -1) {
         TEST_PASS("Odmietnutie príliš dlhého labelu");
     } else {
         TEST_FAIL("Mal odmietnuť príliš dlhý label");
     }
 }
 
 /**
  * @brief Test DNS name compression
  */
 void test_parse_dns_name_compression() {
     printf("\n[TEST] parse_dns_name() - compression\n");
     
     /* 
      * Simulácia DNS packetu s compression:
      * Offset 0: "www.google.com" (3www6google3com0)
      * Offset 16: "mail" + pointer na ".google.com" (offset 4)
      */
     uint8_t buffer[32] = {
         /* Offset 0-15: "www.google.com" */
         3, 'w', 'w', 'w',           /* Offset 0-3 */
         6, 'g', 'o', 'o', 'g', 'l', 'e',  /* Offset 4-10 */
         3, 'c', 'o', 'm',           /* Offset 11-14 */
         0,                          /* Offset 15 */
         
         /* Offset 16-20: "mail" + pointer */
         4, 'm', 'a', 'i', 'l',      /* Offset 16-20 */
         0xC0, 0x04                  /* Pointer na offset 4 (.google.com) */
     };
     
     char name[DNS_MAX_NAME_LEN + 1];
     size_t offset;
     
     /* Test 1: Parsing s compression */
     offset = 16;
     if (parse_dns_name(buffer, sizeof(buffer), &offset, name, sizeof(name)) == 0) {
         if (strcmp(name, "mail.google.com") == 0) {
             TEST_PASS("DNS compression parsing");
         } else {
             TEST_FAIL("Nesprávny výsledok s compression");
             printf("    Očakávané: 'mail.google.com'\n");
             printf("    Dostali sme: '%s'\n", name);
         }
     } else {
         TEST_FAIL("Compression parsing zlyhal");
     }
     
     /* Test 2: Circular pointer detection */
     uint8_t buffer_circular[10] = {
         0xC0, 0x00  /* Pointer na seba samého */
     };
     offset = 0;
     
     if (parse_dns_name(buffer_circular, sizeof(buffer_circular), &offset, 
                        name, sizeof(name)) == -1) {
         TEST_PASS("Detekcia circular pointer");
     } else {
         TEST_FAIL("Mal detekovať circular pointer");
     }
     
     /* Test 3: Pointer na neplatnú pozíciu */
     uint8_t buffer_invalid[5] = {
         0xC0, 0xFF  /* Pointer mimo bufferu */
     };
     offset = 0;
     
     if (parse_dns_name(buffer_invalid, sizeof(buffer_invalid), &offset, 
                        name, sizeof(name)) == -1) {
         TEST_PASS("Odmietnutie neplatného pointeru");
     } else {
         TEST_FAIL("Mal odmietnuť neplatný pointer");
     }
 }
 
 /**
  * @brief Test parsing DNS question section
  */
 void test_parse_dns_question() {
     printf("\n[TEST] parse_dns_question()\n");
     
     /* 
      * DNS Question pre "example.com", type A, class IN
      * Format: 7example3com0 0001 0001
      */
     uint8_t buffer[32] = {
         /* Header (12 bajtov) - nie je potrebný pre tento test ale pridáme */
         0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
         
         /* Question section (začína na offset 12) */
         7, 'e', 'x', 'a', 'm', 'p', 'l', 'e',
         3, 'c', 'o', 'm',
         0,
         0x00, 0x01,  /* QTYPE = A (1) */
         0x00, 0x01   /* QCLASS = IN (1) */
     };
     
     dns_question_t question;
     size_t offset = 12;  /* Skip header */
     
     /* Test 1: Platná otázka */
     if (parse_dns_question(buffer, sizeof(buffer), &offset, &question) == 0) {
         if (strcmp(question.qname, "example.com") == 0 &&
             question.qtype == DNS_TYPE_A &&
             question.qclass == DNS_CLASS_IN) {
             TEST_PASS("Parsing DNS question");
             free(question.qname);
         } else {
             TEST_FAIL("Nesprávne hodnoty v question");
             if (question.qname) free(question.qname);
         }
     } else {
         TEST_FAIL("Parsing DNS question zlyhal");
     }
     
     /* Test 2: Buffer príliš krátky */
     offset = 12;
     if (parse_dns_question(buffer, 20, &offset, &question) == -1) {
         TEST_PASS("Buffer príliš krátky pre question");
     } else {
         TEST_FAIL("Mal odmietnuť krátky buffer");
         if (question.qname) free(question.qname);
     }
 }
 
 /**
  * @brief Test parsing celej DNS správy
  */
 void test_parse_dns_message() {
     printf("\n[TEST] parse_dns_message()\n");
     
     /* 
      * Kompletná DNS Query pre "google.com" type A
      */
     uint8_t buffer[] = {
         /* Header */
         0xAB, 0xCD,        /* ID */
         0x01, 0x00,        /* Flags: standard query, RD=1 */
         0x00, 0x01,        /* QDCOUNT = 1 */
         0x00, 0x00,        /* ANCOUNT = 0 */
         0x00, 0x00,        /* NSCOUNT = 0 */
         0x00, 0x00,        /* ARCOUNT = 0 */
         
         /* Question */
         6, 'g', 'o', 'o', 'g', 'l', 'e',
         3, 'c', 'o', 'm',
         0,
         0x00, 0x01,        /* QTYPE = A */
         0x00, 0x01         /* QCLASS = IN */
     };
     
     dns_message_t message;
     
     /* Test 1: Kompletná správa */
     if (parse_dns_message(buffer, sizeof(buffer), &message) == 0) {
         if (message.header.id == 0xABCD &&
             message.header.qdcount == 1 &&
             message.questions != NULL &&
             strcmp(message.questions[0].qname, "google.com") == 0 &&
             message.questions[0].qtype == DNS_TYPE_A) {
             TEST_PASS("Parsing kompletnej DNS správy");
         } else {
             TEST_FAIL("Nesprávne hodnoty v správe");
         }
         free_dns_message(&message);
     } else {
         TEST_FAIL("Parsing DNS správy zlyhal");
     }
     
     /* Test 2: Buffer príliš krátky */
     if (parse_dns_message(buffer, 10, &message) == -1) {
         TEST_PASS("Odmietnutie príliš krátkeho bufferu");
     } else {
         TEST_FAIL("Mal odmietnuť príliš krátky buffer");
         free_dns_message(&message);
     }
     
     /* Test 3: NULL buffer */
     if (parse_dns_message(NULL, sizeof(buffer), &message) == -1) {
         TEST_PASS("NULL buffer");
     } else {
         TEST_FAIL("Mal odmietnuť NULL buffer");
         free_dns_message(&message);
     }
 }
 
 /**
  * @brief Test free_dns_message
  */
 void test_free_dns_message() {
     printf("\n[TEST] free_dns_message()\n");
     
     uint8_t buffer[] = {
         /* Header */
         0x00, 0x01, 0x01, 0x00, 0x00, 0x01,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         /* Question */
         4, 't', 'e', 's', 't',
         3, 'c', 'o', 'm',
         0,
         0x00, 0x01, 0x00, 0x01
     };
     
     dns_message_t message;
     
     if (parse_dns_message(buffer, sizeof(buffer), &message) == 0) {
         free_dns_message(&message);
         TEST_PASS("Free DNS message");
     } else {
         TEST_FAIL("Parsing pre free test");
     }
     
     /* Test: Free NULL message */
     free_dns_message(NULL);
     TEST_PASS("Free NULL message (no crash)");
 }
 
 /**
  * @brief Main test runner
  */
 int main() {
     printf("==============================================\n");
     printf("DNS Parser Module Unit Tests\n");
     printf("==============================================\n");
     
     test_parse_dns_header();
     test_parse_dns_name_simple();
     test_parse_dns_name_compression();
     test_parse_dns_question();
     test_parse_dns_message();
     test_free_dns_message();
     
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