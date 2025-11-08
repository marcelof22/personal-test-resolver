/**
 * @file resolver.c
 * @brief Upstream DNS resolver komunikácia
 */

#include "resolver.h"
#include "utils.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

/**
 * @brief Prepošle DNS dotaz na upstream server
 */
int forward_query(const dns_message_t *query, const char *upstream, 
                  uint8_t **response, size_t *resp_len) {
    /* TODO: Implementácia v ďalšej fáze */
    (void)query;
    (void)upstream;
    (void)response;
    (void)resp_len;
    
    return -1;
}

/**
 * @brief Zistí IP adresu z hostname
 */
int resolve_upstream_address(const char *hostname, char *ip_addr) {
    /* TODO: Implementácia v ďalšej fáze */
    (void)hostname;
    (void)ip_addr;
    
    return -1;
}