/**
 * @file resolver.h
 * @author Marcel Feiler (xfeile00)
 * @date 10.11.2025
 * @brief Filtering DNS Resolver
 */

 #ifndef RESOLVER_H
 #define RESOLVER_H
 
 #include "dns.h"
 
 /* Timeout pre upstream dotazy (sekundy) */
 #define UPSTREAM_TIMEOUT_SEC    5
 
 /* Počet pokusov pri timeoutoch */
 #define UPSTREAM_RETRY_COUNT    3
 
 /**
  * @brief Prepošle DNS dotaz na upstream server
  * @param query DNS dotaz na preposlanie
  * @param upstream IP adresa alebo hostname upstream servera
  * @param response Výstupný buffer pre odpoveď (alokuje sa)
  * @param resp_len Dĺžka odpovede
  * @return 0 pri úspechu, -1 pri chybe
  * 
  * Edge cases:
  * - Neplatná upstream adresa
  * - Timeout pri čakaní na odpoveď
  * - Neúplná odpoveď (truncated)
  * - Upstream server nedostupný
  * - Hostname upstream - potreba DNS resolution (getaddrinfo)
  */
 int forward_query(const dns_message_t *query, const char *upstream, 
                   uint8_t **response, size_t *resp_len);
 
 /**
  * @brief Zistí IP adresu z hostname
  * @param hostname Hostname alebo IP adresa
  * @param ip_addr Výstupný buffer pre IP adresu (min. INET_ADDRSTRLEN)
  * @return 0 pri úspechu, -1 pri chybe
  * 
  * Ak je hostname už IP adresa, skopíruje ju.
  * Inak použije getaddrinfo() na resolution.
  */
 int resolve_upstream_address(const char *hostname, char *ip_addr);
 
 /**
  * @brief Vytvorí UDP socket pre upstream komunikáciu
  * @return Socket descriptor, -1 pri chybe
  */
 int create_upstream_socket(void);
 
 #endif /* RESOLVER_H */