/**
 * @file dns_parser.c
 * @brief DNS message parsing podľa RFC 1035
 */

#include "dns_parser.h"
#include "utils.h"

#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

/**
 * @brief Parsuje DNS hlavičku zo surových dát
 */
int parse_dns_header(const uint8_t *buffer, size_t len, dns_header_t *header) {
    /* TODO: Implementácia v ďalšej fáze */
    (void)buffer;
    (void)len;
    (void)header;
    
    return -1;
}

/**
 * @brief Parsuje DNS question section
 */
int parse_dns_question(const uint8_t *buffer, size_t len, size_t *offset, 
                       dns_question_t *question) {
    /* TODO: Implementácia v ďalšej fáze */
    (void)buffer;
    (void)len;
    (void)offset;
    (void)question;
    
    return -1;
}

/**
 * @brief Parsuje celú DNS správu
 */
int parse_dns_message(const uint8_t *buffer, size_t len, dns_message_t *message) {
    /* TODO: Implementácia v ďalšej fáze */
    (void)buffer;
    (void)len;
    (void)message;
    
    return -1;
}

/**
 * @brief Uvoľní pamäť alokovanu v DNS správe
 */
void free_dns_message(dns_message_t *message) {
    /* TODO: Implementácia v ďalšej fáze */
    (void)message;
}

/**
 * @brief Parsuje doménové meno z DNS správy (s podporou compression)
 */
int parse_dns_name(const uint8_t *buffer, size_t len, size_t *offset, 
                   char *name, size_t name_len) {
    /* TODO: Implementácia v ďalšej fáze */
    (void)buffer;
    (void)len;
    (void)offset;
    (void)name;
    (void)name_len;
    
    return -1;
}