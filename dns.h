/**
 * @file dns.h
 * @author Marcel Feiler (xfeile00)
 * @date 10.11.2025
 * @brief Filtering DNS Resolver
 */

#ifndef DNS_H
#define DNS_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

/* ============================================================================
 * DNS KONŠTANTY podľa RFC 1035
 * ============================================================================ */

/* DNS Message limits (RFC 1035 Section 2.3.4) */
#define DNS_MAX_LABEL_LEN       63      /* Maximálna dĺžka jedného labelu */
#define DNS_MAX_NAME_LEN        255     /* Maximálna dĺžka celého mena */
#define DNS_UDP_MAX_SIZE        512     /* Maximálna veľkosť UDP správy */
#define DNS_HEADER_SIZE         12      /* Veľkosť DNS hlavičky */

/* DNS Port */
#define DNS_DEFAULT_PORT        53      /* Predvolený DNS port */

/* DNS QTYPE values (RFC 1035 Section 3.2.2) */
#define DNS_TYPE_A              1       /* Host address */
#define DNS_TYPE_NS             2       /* Authoritative name server */
#define DNS_TYPE_CNAME          5       /* Canonical name */
#define DNS_TYPE_SOA            6       /* Start of authority */
#define DNS_TYPE_PTR            12      /* Domain name pointer */
#define DNS_TYPE_MX             15      /* Mail exchange */
#define DNS_TYPE_TXT            16      /* Text strings */
#define DNS_TYPE_AAAA           28      /* IPv6 address */

/* DNS QCLASS values (RFC 1035 Section 3.2.4) */
#define DNS_CLASS_IN            1       /* Internet */

/* DNS Response Codes (RFC 1035 Section 4.1.1) */
#define DNS_RCODE_NOERROR       0       /* No error */
#define DNS_RCODE_FORMERR       1       /* Format error */
#define DNS_RCODE_SERVFAIL      2       /* Server failure */
#define DNS_RCODE_NXDOMAIN      3       /* Non-existent domain */
#define DNS_RCODE_NOTIMPL       4       /* Not implemented */
#define DNS_RCODE_REFUSED       5       /* Query refused */

/* DNS Header Flags (RFC 1035 Section 4.1.1) */
#define DNS_FLAG_QR             0x8000  /* Query/Response bit */
#define DNS_FLAG_AA             0x0400  /* Authoritative Answer */
#define DNS_FLAG_TC             0x0200  /* Truncation */
#define DNS_FLAG_RD             0x0100  /* Recursion Desired */
#define DNS_FLAG_RA             0x0080  /* Recursion Available */

/* DNS Label compression (RFC 1035 Section 4.1.4) */
#define DNS_COMPRESSION_MASK    0xC0    /* 11000000 - pointer prefix */

/* ============================================================================
 * DNS ŠTRUKTÚRY podľa RFC 1035
 * ============================================================================ */

/**
 * @brief DNS hlavička (RFC 1035 Section 4.1.1)
 * 
 * Fixná veľkosť 12 bajtov:
 *                                  1  1  1  1  1  1
 *    0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                      ID                       |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                    QDCOUNT                    |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                    ANCOUNT                    |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                    NSCOUNT                    |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                    ARCOUNT                    |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 */
typedef struct {
    uint16_t id;        /* Identifikátor transakcie */
    uint16_t flags;     /* Príznaky (QR, Opcode, AA, TC, RD, RA, Z, RCODE) */
    uint16_t qdcount;   /* Počet entries v question section */
    uint16_t ancount;   /* Počet resource records v answer section */
    uint16_t nscount;   /* Počet name server records v authority section */
    uint16_t arcount;   /* Počet resource records v additional section */
} dns_header_t;

/**
 * @brief DNS otázka (RFC 1035 Section 4.1.2)
 * 
 * Question section obsahuje QNAME, QTYPE, QCLASS
 */
typedef struct {
    char *qname;        /* Doménové meno (dynamicky alokované) */
    uint16_t qtype;     /* Typ dotazu (A, NS, CNAME, ...) */
    uint16_t qclass;    /* Trieda dotazu (IN = Internet) */
} dns_question_t;

/**
 * @brief Kompletná DNS správa
 */
typedef struct {
    dns_header_t header;
    dns_question_t *questions;  /* Pole otázok [qdcount] */
    uint8_t *raw_data;          /* Surové prijatá data (pre odpoveď) */
    size_t raw_len;             /* Dĺžka surových dát */
} dns_message_t;

/* ============================================================================
 * FILTER ŠTRUKTÚRA (Trie pre efektívne vyhľadávanie)
 * ============================================================================ */

/**
 * @brief Filter node pre Trie štruktúru
 * 
 * Trie je organizovaný odzadu (TLD najprv):
 * Príklad: ads.google.com -> com -> google -> ads
 * 
 * Edge cases:
 * - Prázdne domény
 * - Veľmi dlhé domény (>255 znakov)
 * - Duplicitné záznamy v filter súbore
 * - Špeciálne znaky v doménach
 */
typedef struct filter_node {
    char *label;                    /* Časť domény (napr. "com", "google") */
    struct filter_node **children;  /* Pole detí */
    size_t children_count;          /* Počet detí */
    size_t children_capacity;       /* Kapacita poľa detí */
    bool is_blocked;                /* True = táto doména je blokovaná */
} filter_node_t;

/* ============================================================================
 * KONFIGURÁCIA SERVERA
 * ============================================================================ */

/**
 * @brief Konfigurácia DNS servera
 */
typedef struct {
    char *upstream_server;      /* IP/hostname upstream DNS servera */
    uint16_t local_port;        /* Lokálny port (default 53) */
    char *filter_file;          /* Cesta k filter súboru */
    bool verbose;               /* Verbose logging (-v parameter) */
    filter_node_t *filter_root; /* Koreň Trie štruktúry filtrov */
} server_config_t;

/* ============================================================================
 * CHYBOVÉ KÓDY
 * ============================================================================ */

#define ERR_SUCCESS             0
#define ERR_INVALID_ARGS        1   /* Neplatné argumenty */
#define ERR_SOCKET_CREATE       2   /* Chyba vytvorenia socketu */
#define ERR_SOCKET_BIND         3   /* Chyba bind() */
#define ERR_FILTER_FILE         4   /* Chyba čítania filter súboru */
#define ERR_DNS_PARSE           5   /* Chyba parsovania DNS správy */
#define ERR_UPSTREAM_FAIL       6   /* Chyba komunikácie s upstream */
#define ERR_MEMORY              7   /* Chyba alokácie pamäte */

/* ============================================================================
 * GLOBÁLNE FUNKCIE (deklarácie v príslušných .h súboroch)
 * ============================================================================ */

/* Funkcie budú deklarované v príslušných header súboroch:
 * - dns_server.h
 * - dns_parser.h
 * - dns_builder.h
 * - filter.h
 * - resolver.h
 * - utils.h
 */

#endif /* DNS_H */