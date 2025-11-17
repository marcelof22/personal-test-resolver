/**
 * @file dns_builder.c
 * @author Marcel Feiler (xfeile00)
 * @date 10.11.2025
 * @brief Filtering DNS Resolver - DNS Response Builder
 */

#include "dns_builder.h"
#include "dns_parser.h"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

/**
 * @brief Vytvorí chybovú DNS odpoveď
 */
int build_error_response(const dns_message_t *query, uint8_t rcode,
                         uint8_t **response, size_t *resp_len) {
    if (!query || !response || !resp_len) {
        return -1;
    }

    /* Validácia RCODE (0-5 podľa RFC 1035) */
    if (rcode > DNS_RCODE_REFUSED) {
        return -1;
    }

    /* Potrebujeme: header + question section */
    size_t question_size = 0;

    /* Vypočítame veľkosť question section z raw_data */
    if (query->raw_data && query->raw_len > DNS_HEADER_SIZE) {
        /* Question section začína za headerom */
        const uint8_t *qname_ptr = query->raw_data + DNS_HEADER_SIZE;
        const uint8_t *data_end = query->raw_data + query->raw_len;

        /* Prejdi cez QNAME (zakončené nulou) */
        while (qname_ptr < data_end && *qname_ptr != 0) {
            uint8_t label_len = *qname_ptr;

            /* Kompresia nie je povolená v question section */
            if (label_len >= 64) {
                return -1;
            }

            qname_ptr += label_len + 1;
        }

        if (qname_ptr >= data_end) {
            return -1;
        }

        /* +1 pre nulovú bajt, +4 pre QTYPE a QCLASS */
        qname_ptr += 5;

        if (qname_ptr > data_end) {
            return -1;
        }

        question_size = qname_ptr - (query->raw_data + DNS_HEADER_SIZE);
    } else {
        return -1;
    }

    /* Alokujeme buffer pre odpoveď */
    *resp_len = DNS_HEADER_SIZE + question_size;
    *response = (uint8_t *)malloc(*resp_len);

    if (!*response) {
        return -1;
    }

    /* Vytvoríme header */
    dns_header_t resp_header;
    resp_header.id = query->header.id;  /* Rovnaké ID ako v query */

    /* Nastavíme flags:
     * - QR=1 (response)
     * - Opcode=0 (standard query)
     * - AA=0 (not authoritative)
     * - TC=0 (not truncated)
     * - RD=1 ak bol v query
     * - RA=0 (recursion not available)
     * - Z=0 (reserved)
     * - RCODE=zadaný kód
     */
    uint16_t rd = query->header.flags & DNS_FLAG_RD;
    resp_header.flags = DNS_FLAG_QR | rd | rcode;

    resp_header.qdcount = query->header.qdcount;  /* Skopírujeme otázku */
    resp_header.ancount = 0;  /* Žiadne answers */
    resp_header.nscount = 0;  /* Žiadne authority */
    resp_header.arcount = 0;  /* Žiadne additional */

    /* Zapíšeme header do bufferu pomocou build_dns_header */
    if (build_dns_header(*response, &resp_header) != 0) {
        free(*response);
        *response = NULL;
        return -1;
    }

    /* Skopírujeme question section z pôvodného query */
    memcpy(*response + DNS_HEADER_SIZE,
           query->raw_data + DNS_HEADER_SIZE,
           question_size);

    return 0;
}

/**
 * @brief Vytvorí DNS header do bufferu
 */
int build_dns_header(uint8_t *buffer, const dns_header_t *header) {
    if (!buffer || !header) {
        return -1;
    }

    /* ID - 2 bajty */
    buffer[0] = (header->id >> 8) & 0xFF;
    buffer[1] = header->id & 0xFF;

    /* Flags - 2 bajty */
    buffer[2] = (header->flags >> 8) & 0xFF;
    buffer[3] = header->flags & 0xFF;

    /* QDCOUNT - 2 bajty */
    buffer[4] = (header->qdcount >> 8) & 0xFF;
    buffer[5] = header->qdcount & 0xFF;

    /* ANCOUNT - 2 bajty */
    buffer[6] = (header->ancount >> 8) & 0xFF;
    buffer[7] = header->ancount & 0xFF;

    /* NSCOUNT - 2 bajty */
    buffer[8] = (header->nscount >> 8) & 0xFF;
    buffer[9] = header->nscount & 0xFF;

    /* ARCOUNT - 2 bajty */
    buffer[10] = (header->arcount >> 8) & 0xFF;
    buffer[11] = header->arcount & 0xFF;

    return 0;
}

/**
 * @brief Zakóduje doménové meno do DNS formátu
 */
int encode_dns_name(const char *domain, uint8_t *buffer, size_t buf_len) {
    if (!domain || !buffer || buf_len == 0) {
        return -1;
    }

    /* Prázdna doména = len nulový bajt */
    if (domain[0] == '\0') {
        if (buf_len < 1) {
            return -1;
        }
        buffer[0] = 0;
        return 1;
    }

    size_t domain_len = strlen(domain);
    if (domain_len > DNS_MAX_NAME_LEN) {
        return -1;
    }

    /* Skopírujeme doménu pre manipuláciu */
    char *domain_copy = strdup(domain);
    if (!domain_copy) {
        return -1;
    }

    uint8_t *ptr = buffer;
    size_t bytes_written = 0;

    /* Rozdelíme doménu na labely */
    char *label = strtok(domain_copy, ".");

    while (label != NULL) {
        size_t label_len = strlen(label);

        /* Validácia dĺžky labelu */
        if (label_len == 0 || label_len > DNS_MAX_LABEL_LEN) {
            free(domain_copy);
            return -1;
        }

        /* Kontrola bufferu */
        if (bytes_written + label_len + 1 >= buf_len) {
            free(domain_copy);
            return -1;
        }

        /* Zapíšeme dĺžku labelu */
        *ptr++ = (uint8_t)label_len;
        bytes_written++;

        /* Zapíšeme label */
        memcpy(ptr, label, label_len);
        ptr += label_len;
        bytes_written += label_len;

        label = strtok(NULL, ".");
    }

    /* Ukončujúca nula */
    if (bytes_written >= buf_len) {
        free(domain_copy);
        return -1;
    }

    *ptr = 0;
    bytes_written++;

    free(domain_copy);
    return bytes_written;
}

/* ============================================================================
 * WRAPPER API PRE INTEGRATION TESTY
 * ============================================================================ */

/**
 * @brief Build DNS error response (simplified wrapper for tests)
 */
uint16_t dns_build_error_response(const uint8_t *query_buffer, size_t query_len,
                                   uint8_t *response_buffer, size_t response_max_len,
                                   uint8_t rcode) {
    if (!query_buffer || !response_buffer || query_len == 0 || response_max_len == 0) {
        return 0;
    }

    /* Parse query first */
    dns_message_t query;
    if (parse_dns_message(query_buffer, query_len, &query) != 0) {
        return 0;
    }

    /* Build error response using internal API */
    uint8_t *response = NULL;
    size_t resp_len = 0;

    if (build_error_response(&query, rcode, &response, &resp_len) != 0) {
        free_dns_message(&query);
        return 0;
    }

    /* Copy to output buffer if it fits */
    if (resp_len > response_max_len) {
        free(response);
        free_dns_message(&query);
        return 0;
    }

    memcpy(response_buffer, response, resp_len);
    free(response);
    free_dns_message(&query);

    return (uint16_t)resp_len;
}
