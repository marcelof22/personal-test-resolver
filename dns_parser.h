/**
 * @file dns_parser.h
 * @author Marcel Feiler (xfeile00)
 * @date 10.11.2025
 * @brief Filtering DNS Resolver
 */

#ifndef DNS_PARSER_H
#define DNS_PARSER_H

#include "dns.h"

/**
 * @brief Parsuje DNS hlavičku zo surových dát
 * @param buffer Surové DNS data
 * @param len Dĺžka bufferu
 * @param header Výstupná štruktúra pre hlavičku
 * @return 0 pri úspechu, -1 pri chybe
 * 
 * Edge cases:
 * - Buffer príliš krátky (< 12 bajtov)
 * - Neplatné hodnoty v hlavičke
 */
int parse_dns_header(const uint8_t *buffer, size_t len, dns_header_t *header);

/**
 * @brief Parsuje DNS question section
 * @param buffer Surové DNS data
 * @param len Dĺžka bufferu
 * @param offset Aktuálna pozícia v bufferi (bude aktualizovaná)
 * @param question Výstupná štruktúra pre otázku
 * @return 0 pri úspechu, -1 pri chybe
 * 
 * Edge cases:
 * - DNS compression pointers (0xC0)
 * - Neplatné label dĺžky
 * - Meno dlhšie ako DNS_MAX_NAME_LEN
 * - Neukončené meno (chýbajúci 0x00)
 * - Circular compression pointers
 */
int parse_dns_question(const uint8_t *buffer, size_t len, size_t *offset, 
                       dns_question_t *question);

/**
 * @brief Parsuje celú DNS správu
 * @param buffer Surové DNS data
 * @param len Dĺžka bufferu
 * @param message Výstupná DNS správa
 * @return 0 pri úspechu, -1 pri chybe
 */
int parse_dns_message(const uint8_t *buffer, size_t len, dns_message_t *message);

/**
 * @brief Uvoľní pamäť alokovanu v DNS správe
 * @param message DNS správa na uvoľnenie
 */
void free_dns_message(dns_message_t *message);

/**
 * @brief Parsuje doménové meno z DNS správy (s podporou compression)
 * @param buffer Surové DNS data
 * @param len Dĺžka bufferu
 * @param offset Pozícia kde začína meno (bude aktualizovaná)
 * @param name Výstupný buffer pre meno
 * @param name_len Maximálna dĺžka výstupného bufferu
 * @return 0 pri úspechu, -1 pri chybe
 * 
 * Edge cases podľa RFC 1035 Section 4.1.4:
 * - Label dĺžka 0-63 (00-3F)
 * - Pointer (C0-FF) - compression
 * - Circular pointers (nekonečná slučka)
 * - Pointer na neplatnú pozíciu
 */
int parse_dns_name(const uint8_t *buffer, size_t len, size_t *offset, 
                   char *name, size_t name_len);

#endif /* DNS_PARSER_H */