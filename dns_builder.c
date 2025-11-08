/**
 * @file dns_builder.c
 * @brief DNS message building podľa RFC 1035
 */

#include "dns_builder.h"
#include "utils.h"

#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <ctype.h>

/**
 * @brief Vytvorí chybovú DNS odpoveď
 */
int build_error_response(const dns_message_t *query, uint8_t rcode, 
                         uint8_t **response, size_t *resp_len) {
    /* TODO: Implementácia v ďalšej fáze */
    (void)query;
    (void)rcode;
    (void)response;
    (void)resp_len;
    
    return -1;
}

/**
 * @brief Zakóduje doménové meno do DNS formátu
 */
int encode_dns_name(const char *domain, uint8_t *buffer, size_t buf_len) {
    /* TODO: Implementácia v ďalšej fáze */
    (void)domain;
    (void)buffer;
    (void)buf_len;
    
    return -1;
}