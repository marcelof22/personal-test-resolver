/**
 * @file dns_builder.c
 * @author Marcel Feiler (xfeile00)
 * @date 10.11.2025
 * @brief Filtering DNS Resolver
 */

 #ifndef DNS_BUILDER_H
 #define DNS_BUILDER_H
 
 #include "dns.h"
 
 /**
  * @brief Vytvorí chybovú DNS odpoveď
  * @param query Pôvodný DNS dotaz
  * @param rcode Response code (FORMERR, NXDOMAIN, NOTIMPL, atď.)
  * @param response Výstupný buffer pre odpoveď (alokuje sa)
  * @param resp_len Dĺžka odpovede
  * @return 0 pri úspechu, -1 pri chybe
  * 
  * Vytvorí odpoveď s:
  * - QR=1 (response)
  * - RCODE=zadaný kód
  * - Skopíruje question section z dotazu
  * - Žiadne answer/authority/additional sections
  */
 int build_error_response(const dns_message_t *query, uint8_t rcode, 
                          uint8_t **response, size_t *resp_len);
 
 /**
  * @brief Zakóduje doménové meno do DNS formátu
  * @param domain Doménové meno (napr. "www.google.com")
  * @param buffer Výstupný buffer
  * @param buf_len Veľkosť bufferu
  * @return Počet zapísaných bajtov, -1 pri chybe
  * 
  * Formát podľa RFC 1035 Section 3.1:
  * - Každý label prefixovaný svojou dĺžkou
  * - Ukončené nulou
  * - Príklad: "www.google.com" -> 3www6google3com0
  * 
  * Edge cases:
  * - Prázdna doména
  * - Label dlhší ako 63 znakov
  * - Celková dĺžka > 255 znakov
  */
 int encode_dns_name(const char *domain, uint8_t *buffer, size_t buf_len);
 
 /**
  * @brief Vytvorí DNS header do bufferu
  * @param buffer Buffer pre header (min 12 bajtov)
  * @param header DNS header štruktúra
  * @return 0 pri úspechu, -1 pri chybe
  */
 int build_dns_header(uint8_t *buffer, const dns_header_t *header);
 
 #endif /* DNS_BUILDER_H */