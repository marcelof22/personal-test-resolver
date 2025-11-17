/**
 * @file resolver.c
 * @author Marcel Feiler (xfeile00)
 * @date 10.11.2025
 * @brief Filtering DNS Resolver
 */

 #include "resolver.h"
 #include "dns_parser.h"
 #include "utils.h"
 
 #include <sys/socket.h>
 #include <sys/time.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include <netdb.h>
 #include <string.h>
 #include <unistd.h>
 #include <errno.h>
 #include <stdlib.h>
 
 /* Konštanty pre upstream komunikáciu */
 #define UPSTREAM_PORT 53
 #define UPSTREAM_TIMEOUT_SEC 5
 #define UPSTREAM_RETRIES 3
 
 /**
  * @brief Zistí IP adresu z hostname alebo validuje IP adresu
  * 
  * Podporuje:
  * - IPv4 adresy (napr. "8.8.8.8")
  * - Hostnames (napr. "dns.google")
  * 
  * Edge cases:
  * - Neplatný hostname
  * - DNS resolution failure
  * - NULL pointers
  * - Buffer overflow
  * 
  * @param hostname Hostname alebo IP adresa
  * @param ip_addr Buffer pre výslednú IP adresu (min 16 bajtov)
  * @return 0 pri úspechu, -1 pri chybe
  */
 int resolve_upstream_address(const char *hostname, char *ip_addr) {
     if (hostname == NULL || ip_addr == NULL) {
         return -1;
     }
     
     struct addrinfo hints, *result, *rp;
     int ret;
     
     /* Najprv skúsime či to nie je priamo IP adresa */
     struct in_addr addr;
     if (inet_pton(AF_INET, hostname, &addr) == 1) {
         /* Je to už platná IPv4 adresa */
         strncpy(ip_addr, hostname, 16);
         ip_addr[15] = '\0';
         return 0;
     }
     
     /* Nie je to IP adresa, musíme resolvovať hostname */
     memset(&hints, 0, sizeof(hints));
     hints.ai_family = AF_INET;        /* IPv4 */
     hints.ai_socktype = SOCK_DGRAM;   /* UDP */
     hints.ai_protocol = IPPROTO_UDP;
     
     ret = getaddrinfo(hostname, NULL, &hints, &result);
     if (ret != 0) {
         print_error("Failed to resolve hostname '%s': %s", 
                    hostname, gai_strerror(ret));
         return -1;
     }
     
     /* Iterujeme cez výsledky a vezmeme prvú IPv4 adresu */
     for (rp = result; rp != NULL; rp = rp->ai_next) {
         if (rp->ai_family == AF_INET) {
             struct sockaddr_in *addr_in = (struct sockaddr_in *)rp->ai_addr;
             
             if (inet_ntop(AF_INET, &addr_in->sin_addr, ip_addr, 16) != NULL) {
                 freeaddrinfo(result);
                 return 0;
             }
         }
     }
     
     /* Nenašli sme žiadnu IPv4 adresu */
     print_error("No IPv4 address found for hostname '%s'", hostname);
     freeaddrinfo(result);
     return -1;
 }
 
 /**
  * @brief Prepošle DNS dotaz na upstream server
  * 
  * Proces:
  * 1. Resolve upstream adresy (hostname → IP)
  * 2. Vytvorenie UDP socketu
  * 3. Nastavenie timeoutu
  * 4. Odoslanie dotazu na upstream
  * 5. Prijatie odpovede (s retry)
  * 6. Validácia odpovede
  * 
  * Edge cases:
  * - Upstream nedostupný
  * - Timeout
  * - Truncated response (TC flag)
  * - Invalid response
  * - Transaction ID mismatch
  * - Socket errors
  * 
  * @param query Pôvodný DNS dotaz
  * @param upstream Upstream server (IP alebo hostname)
  * @param response Buffer pre odpoveď (alokuje sa)
  * @param resp_len Dĺžka odpovede
  * @return 0 pri úspechu, -1 pri chybe
  */
 int forward_query(const dns_message_t *query, const char *upstream, 
                   uint8_t **response, size_t *resp_len) {
     if (query == NULL || upstream == NULL || response == NULL || resp_len == NULL) {
         return -1;
     }
     
     /* Edge case: Query musí mať raw data */
     if (query->raw_data == NULL || query->raw_len == 0) {
         print_error("Query has no raw data to forward");
         return -1;
     }
     
     /* Resolve upstream adresy */
     char upstream_ip[16];
     if (resolve_upstream_address(upstream, upstream_ip) != 0) {
         return -1;
     }
     
     /* Vytvorenie UDP socketu */
     int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
     if (sockfd < 0) {
         print_error("Failed to create upstream socket: %s", strerror(errno));
         return -1;
     }
     
     /* Nastavenie timeoutu pre recv */
     struct timeval timeout;
     timeout.tv_sec = UPSTREAM_TIMEOUT_SEC;
     timeout.tv_usec = 0;
     
     if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
         print_error("Failed to set socket timeout: %s", strerror(errno));
         close(sockfd);
         return -1;
     }
     
     /* Nastavenie upstream adresy */
     struct sockaddr_in upstream_addr;
     memset(&upstream_addr, 0, sizeof(upstream_addr));
     upstream_addr.sin_family = AF_INET;
     upstream_addr.sin_port = htons(UPSTREAM_PORT);
     
     if (inet_pton(AF_INET, upstream_ip, &upstream_addr.sin_addr) != 1) {
         print_error("Invalid upstream IP address: %s", upstream_ip);
         close(sockfd);
         return -1;
     }
     
     /* Alokácia bufferu pre odpoveď */
     uint8_t *resp_buffer = (uint8_t *)malloc(DNS_UDP_MAX_SIZE);
     if (resp_buffer == NULL) {
         print_error("Failed to allocate response buffer");
         close(sockfd);
         return -1;
     }
     
     /* Retry loop */
     int attempt;
     ssize_t recv_len = -1;
     
     for (attempt = 0; attempt < UPSTREAM_RETRIES; attempt++) {
         if (attempt > 0) {
             verbose_log_raw("  Retry attempt %d/%d...", attempt + 1, UPSTREAM_RETRIES);
         }
         
         /* Odoslanie dotazu na upstream */
         ssize_t sent_len = sendto(sockfd, query->raw_data, query->raw_len, 0,
                                   (struct sockaddr *)&upstream_addr, 
                                   sizeof(upstream_addr));
         
         if (sent_len < 0) {
             print_error("Failed to send to upstream: %s", strerror(errno));
             continue;  /* Retry */
         }
         
         if ((size_t)sent_len != query->raw_len) {
             print_error("Partial send to upstream (%zd/%zu bytes)", 
                        sent_len, query->raw_len);
             continue;  /* Retry */
         }
         
         /* Prijatie odpovede od upstream */
         socklen_t addr_len = sizeof(upstream_addr);
         recv_len = recvfrom(sockfd, resp_buffer, DNS_UDP_MAX_SIZE, 0,
                            (struct sockaddr *)&upstream_addr, &addr_len);
         
         if (recv_len < 0) {
             if (errno == EAGAIN || errno == EWOULDBLOCK) {
                 verbose_log_raw("  Upstream timeout");
                 continue;  /* Timeout - retry */
             }
             
             print_error("Failed to receive from upstream: %s", strerror(errno));
             continue;  /* Retry */
         }
         
         /* Úspešne sme prijali odpoveď */
         break;
     }
     
     /* Check či sa podarilo prijať odpoveď */
     if (recv_len < 0) {
         print_error("Failed to get response from upstream after %d attempts", 
                    UPSTREAM_RETRIES);
         free(resp_buffer);
         close(sockfd);
         return -1;
     }
     
     /* Edge case: Odpoveď príliš krátka */
     if (recv_len < DNS_HEADER_SIZE) {
         print_error("Upstream response too short (%zd bytes)", recv_len);
         free(resp_buffer);
         close(sockfd);
         return -1;
     }
     
     /* Validácia odpovede - check Transaction ID */
     dns_header_t query_header, resp_header;
     
     if (parse_dns_header(query->raw_data, query->raw_len, &query_header) != 0) {
         print_error("Failed to parse query header for validation");
         free(resp_buffer);
         close(sockfd);
         return -1;
     }
     
     if (parse_dns_header(resp_buffer, (size_t)recv_len, &resp_header) != 0) {
         print_error("Failed to parse upstream response header");
         free(resp_buffer);
         close(sockfd);
         return -1;
     }
     
     /* Edge case: Transaction ID mismatch */
     if (query_header.id != resp_header.id) {
         print_error("Transaction ID mismatch (query: 0x%04X, response: 0x%04X)",
                    query_header.id, resp_header.id);
         free(resp_buffer);
         close(sockfd);
         return -1;
     }
     
     /* Edge case: TC flag set (truncated response) */
     if (resp_header.flags & DNS_FLAG_TC) {
         verbose_log_raw("  Warning: Upstream response truncated (TC flag set)");
         /* Pokračujeme aj tak - UDP limit */
     }
     
     /* Edge case: Check QR flag (musí byť response) */
     if (!(resp_header.flags & DNS_FLAG_QR)) {
         print_error("Upstream response has QR=0 (not a response)");
         free(resp_buffer);
         close(sockfd);
         return -1;
     }
     
     /* Všetko OK - nastavíme výstupné parametre */
     *response = resp_buffer;
     *resp_len = (size_t)recv_len;
     
     close(sockfd);
     
     verbose_log_raw("  Upstream response: %zu bytes, RCODE=%u, ANCOUNT=%u",
                    *resp_len, 
                    resp_header.flags & 0x0F,
                    resp_header.ancount);
     
     return 0;
 }
 
 /**
  * @brief Vytvorí UDP socket pre upstream komunikáciu
  * 
  * Helper funkcia pre testovanie.
  * 
  * @return Socket descriptor, -1 pri chybe
  */
 int create_upstream_socket(void) {
     int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
     if (sockfd < 0) {
         return -1;
     }
     
     /* Nastavenie timeoutu */
     struct timeval timeout;
     timeout.tv_sec = UPSTREAM_TIMEOUT_SEC;
     timeout.tv_usec = 0;
     
     if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
         close(sockfd);
         return -1;
     }
     
     return sockfd;
 }