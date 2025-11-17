/**
 * @file dns_server.c
 * @author Marcel Feiler (xfeile00)
 * @date 10.11.2025
 * @brief Filtering DNS Resolver
 */

 #include "dns_server.h"
 #include "dns_parser.h"
 #include "dns_builder.h"
 #include "filter.h"
 #include "resolver.h"
 #include "utils.h"
 
 #include <sys/socket.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include <unistd.h>
 #include <stdlib.h>
 #include <string.h>
 #include <errno.h>
 #include <signal.h>
 
 /* Globálna premenná pre graceful shutdown */
 static volatile sig_atomic_t server_running = 1;
 
 /**
  * @brief Signal handler pre SIGINT (Ctrl+C)
  */
 static void signal_handler(int signum) {
     (void)signum;
     server_running = 0;
 }
 
 /**
  * @brief Inicializuje UDP socket na zadanom porte
  * 
  * Edge cases:
  * - Privilegovaný port (<1024) bez root
  * - Port už používaný
  * - Socket creation failure
  */
 int init_udp_server(uint16_t port) {
     int sockfd;
     struct sockaddr_in server_addr;
     int reuse = 1;
     
     /* Vytvorenie UDP socketu */
     sockfd = socket(AF_INET, SOCK_DGRAM, 0);
     if (sockfd < 0) {
         print_error("Failed to create socket: %s", strerror(errno));
         return -1;
     }
     
     /* SO_REUSEADDR pre rýchly restart servera */
     if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
         print_error("Failed to set SO_REUSEADDR: %s", strerror(errno));
         close(sockfd);
         return -1;
     }
     
     /* Nastavenie server adresy */
     memset(&server_addr, 0, sizeof(server_addr));
     server_addr.sin_family = AF_INET;
     server_addr.sin_addr.s_addr = INADDR_ANY;  /* Listen na všetkých interfacoch */
     server_addr.sin_port = htons(port);
     
     /* Bind na port */
     if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
         print_error("Failed to bind to port %u: %s", port, strerror(errno));
         close(sockfd);
         return -1;
     }
     
     return sockfd;
 }
 
 /**
  * @brief Spracuje jeden DNS dotaz
  * 
  * Logika:
  * 1. Parse DNS query
  * 2. Check QTYPE (len A je podporovaný)
  * 3. Check filter (blokovaná doména?)
  * 4. Ak blokovaná → NXDOMAIN
  * 5. Ak neplatný typ → NOTIMPL
  * 6. Ak povolená → forward upstream (FÁZA 6)
  * 
  * @param config Server konfigurácia
  * @param query_buffer Buffer s DNS dotazom
  * @param query_len Dĺžka dotazu
  * @param response_buffer Buffer pre odpoveď (alokuje sa)
  * @param response_len Dĺžka odpovede
  * @return 0 pri úspechu, -1 pri chybe
  */
 static int process_dns_query(server_config_t *config,
                              const uint8_t *query_buffer, size_t query_len,
                              uint8_t **response_buffer, size_t *response_len) {
     dns_message_t query;
     
     /* Parse DNS query */
     if (parse_dns_message(query_buffer, query_len, &query) != 0) {
         verbose_log(config, "Failed to parse DNS query - sending FORMERR");
         
         /* Ak sa nepodarí parsovať, nemôžeme ani postaviť response */
         /* V reále by sme mali aspoň skúsiť extrahovať ID */
         return -1;
     }
     
     verbose_log(config, "Received DNS query:");
     verbose_log(config, "  Transaction ID: 0x%04X", query.header.id);
     verbose_log(config, "  Questions: %u", query.header.qdcount);
     
     /* Edge case: Žiadne otázky */
     if (query.header.qdcount == 0) {
         verbose_log(config, "  No questions in query - sending FORMERR");
         
         if (build_error_response(&query, DNS_RCODE_FORMERR, 
                                 response_buffer, response_len) != 0) {
             free_dns_message(&query);
             return -1;
         }
         
         free_dns_message(&query);
         return 0;
     }
     
     /* Pre simplicity, spracujeme len prvú otázku */
     dns_question_t *question = &query.questions[0];
     
     verbose_log(config, "  Domain: %s", question->qname);
     verbose_log(config, "  Type: %u (%s)", 
                 question->qtype,
                 question->qtype == DNS_TYPE_A ? "A" :
                 question->qtype == DNS_TYPE_AAAA ? "AAAA" :
                 question->qtype == DNS_TYPE_MX ? "MX" :
                 question->qtype == DNS_TYPE_CNAME ? "CNAME" : "Other");
     
     /* Check QTYPE - podporujeme len A */
     if (question->qtype != DNS_TYPE_A) {
         verbose_log(config, "  Unsupported query type - sending NOTIMPL");
         
         if (build_error_response(&query, DNS_RCODE_NOTIMPL,
                                 response_buffer, response_len) != 0) {
             free_dns_message(&query);
             return -1;
         }
         
         free_dns_message(&query);
         return 0;
     }
     
     /* Check filter - je doména blokovaná? */
     bool blocked = is_domain_blocked(config->filter_root, question->qname);
     
     if (blocked) {
         verbose_log(config, "  Domain is BLOCKED - sending NXDOMAIN");
         
         if (build_error_response(&query, DNS_RCODE_NXDOMAIN,
                                 response_buffer, response_len) != 0) {
             free_dns_message(&query);
             return -1;
         }
         
         free_dns_message(&query);
         return 0;
     }
     
     /* Doména nie je blokovaná - forward na upstream */
     verbose_log(config, "  Domain is allowed - forwarding to upstream %s", 
                 config->upstream_server);
     
     /* Forward na upstream server (implementované v FÁZE 6) */
     if (forward_query(&query, config->upstream_server, 
                      response_buffer, response_len) != 0) {
         verbose_log(config, "  Upstream forwarding failed - sending SERVFAIL");
         
         /* Ak forwarding zlyhal, vrátime SERVFAIL */
         if (build_error_response(&query, DNS_RCODE_SERVFAIL,
                                 response_buffer, response_len) != 0) {
             free_dns_message(&query);
             return -1;
         }
         
         free_dns_message(&query);
         return 0;
     }
     
     verbose_log(config, "  Response received from upstream (%zu bytes)", *response_len);
     
     free_dns_message(&query);
     return 0;
 }
 
 /**
  * @brief Hlavná slučka DNS servera
  * 
  * Nekonečná slučka ktorá:
  * 1. Čaká na UDP dotaz (recvfrom)
  * 2. Spracuje dotaz
  * 3. Pošle odpoveď (sendto)
  * 
  * Edge cases:
  * - Socket errors
  * - Truncated packets
  * - Invalid source addresses
  * - Timeout pri upstream
  * 
  * @param config Server konfigurácia
  * @return ERR_SUCCESS alebo chybový kód
  */
 int run_server(server_config_t *config) {
     if (config == NULL) {
         return ERR_INVALID_ARGS;
     }
     
     /* Inicializácia socketu */
     int sockfd = init_udp_server(config->local_port);
     if (sockfd < 0) {
         return ERR_SOCKET_CREATE;
     }
     
     verbose_log(config, "DNS server listening on port %u", config->local_port);
     verbose_log(config, "Press Ctrl+C to stop");
     
     /* Nastavenie signal handlera pre graceful shutdown */
     signal(SIGINT, signal_handler);
     signal(SIGTERM, signal_handler);
     
     /* Buffer pre prijímanie DNS dotazov */
     uint8_t query_buffer[DNS_UDP_MAX_SIZE];
     
     /* Client address */
     struct sockaddr_in client_addr;
     socklen_t client_addr_len;
     
     /* Počítadlo dotazov */
     unsigned long query_count = 0;
     unsigned long blocked_count = 0;
     unsigned long forwarded_count = 0;
     unsigned long error_count = 0;
     
     /* Hlavná slučka servera */
     while (server_running) {
         client_addr_len = sizeof(client_addr);
         
         /* Prijatie UDP dotazu */
         ssize_t recv_len = recvfrom(sockfd, query_buffer, sizeof(query_buffer), 0,
                                     (struct sockaddr *)&client_addr, &client_addr_len);
         
         if (recv_len < 0) {
             if (errno == EINTR) {
                 /* Interrupted by signal - continue alebo exit */
                 if (!server_running) {
                     break;
                 }
                 continue;
             }
             
             print_error("recvfrom() failed: %s", strerror(errno));
             error_count++;
             continue;
         }
         
         /* Edge case: Príliš krátky packet */
         if (recv_len < DNS_HEADER_SIZE) {
             verbose_log(config, "Received packet too short (%zd bytes) from %s:%u",
                        recv_len,
                        inet_ntoa(client_addr.sin_addr),
                        ntohs(client_addr.sin_port));
             error_count++;
             continue;
         }
         
         query_count++;
         
         verbose_log(config, "\n[Query #%lu] from %s:%u (%zd bytes)",
                    query_count,
                    inet_ntoa(client_addr.sin_addr),
                    ntohs(client_addr.sin_port),
                    recv_len);
         
         /* Spracovanie dotazu */
         uint8_t *response_buffer = NULL;
         size_t response_len = 0;
         
         int result = process_dns_query(config, query_buffer, (size_t)recv_len,
                                        &response_buffer, &response_len);
         
         if (result != 0 || response_buffer == NULL) {
             verbose_log(config, "Failed to process query");
             error_count++;
             
             if (response_buffer != NULL) {
                 free(response_buffer);
             }
             continue;
         }
         
         /* Určenie typu odpovede pre štatistiky */
         if (response_len >= DNS_HEADER_SIZE) {
             dns_header_t resp_header;
             if (parse_dns_header(response_buffer, response_len, &resp_header) == 0) {
                 uint16_t rcode = resp_header.flags & 0x0F;
                 
                 if (rcode == DNS_RCODE_NXDOMAIN) {
                     blocked_count++;
                 } else if (rcode == DNS_RCODE_NOERROR) {
                     forwarded_count++;
                 }
             }
         }
         
         /* Odoslanie odpovede */
         ssize_t sent_len = sendto(sockfd, response_buffer, response_len, 0,
                                   (struct sockaddr *)&client_addr, client_addr_len);
         
         if (sent_len < 0) {
             print_error("sendto() failed: %s", strerror(errno));
             error_count++;
         } else if ((size_t)sent_len != response_len) {
             verbose_log(config, "Warning: Partial send (%zd/%zu bytes)",
                        sent_len, response_len);
         } else {
             verbose_log(config, "Response sent (%zu bytes)", response_len);
         }
         
         free(response_buffer);
     }
     
     /* Shutdown */
     close(sockfd);
     
     /* Finálne štatistiky */
     printf("\n==============================================\n");
     printf("DNS Server Statistics:\n");
     printf("==============================================\n");
     printf("  Total queries:     %lu\n", query_count);
     printf("  Blocked (NXDOMAIN): %lu (%.1f%%)\n", 
            blocked_count, 
            query_count > 0 ? (100.0 * blocked_count / query_count) : 0.0);
     printf("  Forwarded:         %lu (%.1f%%)\n",
            forwarded_count,
            query_count > 0 ? (100.0 * forwarded_count / query_count) : 0.0);
     printf("  Errors:            %lu\n", error_count);
     printf("==============================================\n");
     
     return ERR_SUCCESS;
 }