/**
 * @file filter.h
 * @author Marcel Feiler (xfeile00)
 * @date 10.11.2025
 * @brief Filtering DNS Resolver
 */

#ifndef FILTER_H
#define FILTER_H

#include "dns.h"

/**
 * @brief Inicializuje nový filter node
 * @return Nový filter node alebo NULL pri chybe
 */
filter_node_t *filter_node_create(void);

/**
 * @brief Uvoľní celú Trie štruktúru
 * @param root Koreň Trie
 */
void filter_node_free(filter_node_t *root);

/**
 * @brief Načíta filter súbor a vytvorí Trie štruktúru
 * @param filename Cesta k filter súboru
 * @param verbose Verbose logging
 * @return Koreň Trie alebo NULL pri chybe
 * 
 * Formát súboru:
 * - Každá doména na samostatnom riadku
 * - Prázdne riadky ignorovať
 * - Riadky začínajúce '#' ignorovať (komentáre)
 * - Podporuje LF, CRLF, CR konce riadkov
 * 
 * Edge cases:
 * - Neexistujúci súbor
 * - Prázdny súbor
 * - Duplicitné domény
 * - Neplatné doménové mená
 * - Príliš dlhé riadky
 * - Encoding issues
 */
filter_node_t *load_filter_file(const char *filename, bool verbose);

/**
 * @brief Pridá doménu do Trie
 * @param root Koreň Trie
 * @param domain Doménové meno na pridanie
 * @return 0 pri úspechu, -1 pri chybe
 * 
 * Domény sa ukladajú v reverznom poradí (TLD najprv):
 * Príklad: "ads.google.com" -> "com" -> "google" -> "ads"
 */
int filter_add_domain(filter_node_t *root, const char *domain);

/**
 * @brief Kontroluje či je doména blokovaná
 * @param root Koreň Trie
 * @param domain Doménové meno na kontrolu
 * @return true ak je blokovaná, false inak
 * 
 * Kontroluje aj všetky subdomény:
 * Ak je blokovaná "google.com", tak aj "ads.google.com" je blokovaná
 * 
 * Edge cases:
 * - NULL root alebo domain
 * - Prázdna doména
 * - Veľké/malé písmená (case-insensitive)
 */
bool is_domain_blocked(const filter_node_t *root, const char *domain);

/**
 * @brief Normalizuje doménové meno
 * @param domain Pôvodné meno
 * @param normalized Výstupný buffer
 * @param len Veľkosť bufferu
 * @return 0 pri úspechu, -1 pri chybe
 * 
 * Normalizácia:
 * - Lowercase
 * - Odstránenie trailing dot ('.')
 * - Odstránenie whitespace
 */
int normalize_domain(const char *domain, char *normalized, size_t len);

/**
 * @brief Vypíše štatistiky o filtroch (ak verbose)
 * @param root Koreň Trie
 * @param verbose Verbose mode
 */
void filter_print_stats(const filter_node_t *root, bool verbose);

#endif /* FILTER_H */