/**
 * @file dns_parser.c
 * @author Marcel Feiler (xfeile00)
 * @date 10.11.2025
 * @brief Filtering DNS Resolver
 */

 #include "dns_parser.h"
 #include "utils.h"
 
 #include <string.h>
 #include <stdlib.h>
 #include <arpa/inet.h>
 
 /* Maximálny počet jumps pre detekciu circular pointers */
 #define MAX_COMPRESSION_JUMPS 10
 
 /**
  * @brief Parsuje DNS hlavičku zo surových dát (RFC 1035 Section 4.1.1)
  * 
  * DNS Header format (12 bajtov):
  *                                     1  1  1  1  1  1
  *       0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
  *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  *     |                      ID                       |
  *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  *     |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
  *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  *     |                    QDCOUNT                    |
  *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  *     |                    ANCOUNT                    |
  *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  *     |                    NSCOUNT                    |
  *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  *     |                    ARCOUNT                    |
  *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  * 
  * Edge cases:
  * - Buffer príliš krátky (< 12 bajtov)
  * - NULL pointers
  */
 int parse_dns_header(const uint8_t *buffer, size_t len, dns_header_t *header) {
     if (buffer == NULL || header == NULL) {
         return -1;
     }
     
     /* Edge case: Buffer príliš krátky */
     if (len < DNS_HEADER_SIZE) {
         return -1;
     }
     
     /* Parsing hlavičky - všetky hodnoty sú v network byte order (big-endian) */
     header->id = ntohs(*(uint16_t *)(buffer + 0));
     header->flags = ntohs(*(uint16_t *)(buffer + 2));
     header->qdcount = ntohs(*(uint16_t *)(buffer + 4));
     header->ancount = ntohs(*(uint16_t *)(buffer + 6));
     header->nscount = ntohs(*(uint16_t *)(buffer + 8));
     header->arcount = ntohs(*(uint16_t *)(buffer + 10));
     
     return 0;
 }
 
 /**
  * @brief Parsuje doménové meno z DNS správy (RFC 1035 Section 3.1, 4.1.4)
  * 
  * DNS Name format:
  * - Séria labels, každý prefixovaný svojou dĺžkou (1 bajt)
  * - Label dĺžka 0-63 (0x00-0x3F)
  * - Ukončené 0x00 (nulová dĺžka)
  * - Compression: pointer prefix 0xC0 (11xxxxxx xxxxxxxx)
  * 
  * Príklad: "www.google.com"
  *   3 'w' 'w' 'w' 6 'g' 'o' 'o' 'g' 'l' 'e' 3 'c' 'o' 'm' 0
  * 
  * Edge cases:
  * - Circular compression pointers
  * - Pointer na neplatnú pozíciu
  * - Label dĺžka > 63
  * - Celková dĺžka > 255
  * - Neukončené meno
  * - Buffer overflow
  * 
  * @param buffer Celý DNS packet (potrebný pre compression)
  * @param len Dĺžka celého packetu
  * @param offset Aktuálna pozícia v bufferi (bude updatovaná)
  * @param name Výstupný buffer pre meno
  * @param name_len Veľkosť výstupného bufferu
  * @return 0 pri úspechu, -1 pri chybe
  */
 int parse_dns_name(const uint8_t *buffer, size_t len, size_t *offset, 
                    char *name, size_t name_len) {
     if (buffer == NULL || offset == NULL || name == NULL || name_len == 0) {
         return -1;
     }
     
     size_t pos = *offset;  /* Aktuálna pozícia v bufferi */
     size_t name_pos = 0;   /* Pozícia vo výstupnom mene */
     int jump_count = 0;    /* Počítadlo jumps pre detekciu loops */
     bool jumped = false;   /* Či sme použili compression pointer */
     
     while (pos < len) {
         uint8_t label_len = buffer[pos];
         
         /* Check pre compression pointer (RFC 1035 Section 4.1.4) */
         if ((label_len & 0xC0) == 0xC0) {
             /* Compression pointer: 11xxxxxx xxxxxxxx */
             
             /* Edge case: Potrebujeme 2 bajty pre pointer */
             if (pos + 1 >= len) {
                 return -1;
             }
             
             /* Extract 14-bit offset */
             uint16_t pointer_offset = ((uint16_t)(label_len & 0x3F) << 8) | buffer[pos + 1];
             
             /* Edge case: Pointer musí ukazovať dozadu (inak by to bola slučka) */
             if (pointer_offset >= pos) {
                 return -1;
             }
             
             /* Edge case: Detekcia circular pointers */
             jump_count++;
             if (jump_count > MAX_COMPRESSION_JUMPS) {
                 return -1;  /* Príliš veľa jumps = pravdepodobne slučka */
             }
             
             /* Update offset iba pri prvom jump */
             if (!jumped) {
                 *offset = pos + 2;  /* Skip 2-byte pointer */
                 jumped = true;
             }
             
             /* Jump na pointer pozíciu */
             pos = pointer_offset;
             continue;
         }
         
         /* Edge case: Label dĺžka > 63 (neplatné) */
         if (label_len > DNS_MAX_LABEL_LEN) {
             return -1;
         }
         
         /* Label dĺžka 0 = koniec mena */
         if (label_len == 0) {
             /* Ak sme nepoužili compression, update offset */
             if (!jumped) {
                 *offset = pos + 1;
             }
             
             /* Odstránenie trailing dot */
             if (name_pos > 0 && name[name_pos - 1] == '.') {
                 name_pos--;
             }
             
             name[name_pos] = '\0';
             return 0;
         }
         
         pos++;  /* Skip length byte */
         
         /* Edge case: Label presahuje buffer */
         if (pos + label_len > len) {
             return -1;
         }
         
         /* Edge case: Výsledné meno bude príliš dlhé */
         if (name_pos + label_len + 1 >= name_len) {  /* +1 pre dot */
             return -1;
         }
         
         /* Skopíruj label do výstupu */
         for (size_t i = 0; i < label_len; i++) {
             name[name_pos++] = (char)buffer[pos++];
         }
         
         /* Pridaj dot (okrem posledného labelu) */
         name[name_pos++] = '.';
         
         /* Edge case: Celková dĺžka > DNS_MAX_NAME_LEN */
         if (name_pos > DNS_MAX_NAME_LEN) {
             return -1;
         }
     }
     
     /* Edge case: Dosiahli sme koniec bufferu bez ukončenia mena */
     return -1;
 }
 
 /**
  * @brief Parsuje DNS question section (RFC 1035 Section 4.1.2)
  * 
  * Question format:
  *                                     1  1  1  1  1  1
  *       0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
  *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  *     |                                               |
  *     /                     QNAME                     /
  *     /                                               /
  *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  *     |                     QTYPE                     |
  *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  *     |                     QCLASS                    |
  *     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
  * 
  * Edge cases:
  * - Buffer príliš krátky pre QTYPE/QCLASS
  * - Neplatné QNAME
  * - Memory allocation failure
  */
 int parse_dns_question(const uint8_t *buffer, size_t len, size_t *offset, 
                        dns_question_t *question) {
     if (buffer == NULL || offset == NULL || question == NULL) {
         return -1;
     }
     
     /* Edge case: Offset mimo bufferu */
     if (*offset >= len) {
         return -1;
     }
     
     /* Alokácia bufferu pre QNAME */
     char qname[DNS_MAX_NAME_LEN + 1];
     
     /* Parsing QNAME */
     if (parse_dns_name(buffer, len, offset, qname, sizeof(qname)) != 0) {
         return -1;
     }
     
     /* Edge case: Nedostatok miesta pre QTYPE a QCLASS (4 bajty) */
     if (*offset + 4 > len) {
         return -1;
     }
     
     /* Parsing QTYPE (2 bajty) */
     question->qtype = ntohs(*(uint16_t *)(buffer + *offset));
     *offset += 2;
     
     /* Parsing QCLASS (2 bajty) */
     question->qclass = ntohs(*(uint16_t *)(buffer + *offset));
     *offset += 2;
     
     /* Alokácia a kopírovanie QNAME */
     question->qname = strdup(qname);
     if (question->qname == NULL) {
         return -1;  /* Memory allocation failure */
     }
     
     return 0;
 }
 
 /**
  * @brief Parsuje celú DNS správu
  * 
  * Edge cases:
  * - NULL pointers
  * - Buffer príliš krátky
  * - Neplatná hlavička
  * - Neplatné otázky
  * - Memory allocation failures
  */
 int parse_dns_message(const uint8_t *buffer, size_t len, dns_message_t *message) {
     if (buffer == NULL || message == NULL) {
         return -1;
     }
     
     /* Inicializácia message štruktúry */
     memset(message, 0, sizeof(dns_message_t));
     
     /* Parsing hlavičky */
     if (parse_dns_header(buffer, len, &message->header) != 0) {
         return -1;
     }
     
     /* Edge case: Iba header, žiadne otázky */
     if (message->header.qdcount == 0) {
         message->questions = NULL;
         
         /* Uloženie raw dát pre potenciálne preposlanie */
         message->raw_data = (uint8_t *)malloc(len);
         if (message->raw_data == NULL) {
             return -1;
         }
         memcpy(message->raw_data, buffer, len);
         message->raw_len = len;
         
         return 0;
     }
     
     /* Alokácia poľa pre otázky */
     message->questions = (dns_question_t *)calloc(message->header.qdcount, 
                                                    sizeof(dns_question_t));
     if (message->questions == NULL) {
         return -1;
     }
     
     /* Parsing všetkých otázok */
     size_t offset = DNS_HEADER_SIZE;
     
     for (uint16_t i = 0; i < message->header.qdcount; i++) {
         if (parse_dns_question(buffer, len, &offset, &message->questions[i]) != 0) {
             /* Cleanup pri chybe */
             for (uint16_t j = 0; j < i; j++) {
                 if (message->questions[j].qname != NULL) {
                     free(message->questions[j].qname);
                 }
             }
             free(message->questions);
             message->questions = NULL;
             return -1;
         }
     }
     
     /* Uloženie raw dát pre potenciálne preposlanie */
     message->raw_data = (uint8_t *)malloc(len);
     if (message->raw_data == NULL) {
         /* Cleanup */
         for (uint16_t i = 0; i < message->header.qdcount; i++) {
             if (message->questions[i].qname != NULL) {
                 free(message->questions[i].qname);
             }
         }
         free(message->questions);
         message->questions = NULL;
         return -1;
     }
     memcpy(message->raw_data, buffer, len);
     message->raw_len = len;
     
     return 0;
 }
 
 /**
  * @brief Uvoľní pamäť alokovanu v DNS správe
  */
 void free_dns_message(dns_message_t *message) {
     if (message == NULL) {
         return;
     }
     
     /* Uvoľnenie otázok */
     if (message->questions != NULL) {
         for (uint16_t i = 0; i < message->header.qdcount; i++) {
             if (message->questions[i].qname != NULL) {
                 free(message->questions[i].qname);
             }
         }
         free(message->questions);
         message->questions = NULL;
     }
     
     /* Uvoľnenie raw dát */
     if (message->raw_data != NULL) {
         free(message->raw_data);
         message->raw_data = NULL;
     }
     
     message->raw_len = 0;
 }