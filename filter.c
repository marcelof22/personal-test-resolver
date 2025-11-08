/**
 * @file filter.c
 * @brief Filter nežiadúcich domén pomocou Trie štruktúry
 */

#include "filter.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 * @brief Inicializuje nový filter node
 */
filter_node_t *filter_node_create(void) {
    filter_node_t *node = (filter_node_t *)malloc(sizeof(filter_node_t));
    if (node == NULL) {
        return NULL;
    }
    
    node->label = NULL;
    node->children = NULL;
    node->children_count = 0;
    node->children_capacity = 0;
    node->is_blocked = false;
    
    return node;
}

/**
 * @brief Uvoľní celú Trie štruktúru
 */
void filter_node_free(filter_node_t *root) {
    if (root == NULL) {
        return;
    }
    
    /* Rekurzívne uvoľnenie všetkých detí */
    for (size_t i = 0; i < root->children_count; i++) {
        filter_node_free(root->children[i]);
    }
    
    /* Uvoľnenie vlastných zdrojov */
    if (root->label != NULL) {
        free(root->label);
    }
    if (root->children != NULL) {
        free(root->children);
    }
    free(root);
}

/**
 * @brief Načíta filter súbor a vytvorí Trie štruktúru
 */
filter_node_t *load_filter_file(const char *filename, bool verbose) {
    /* TODO: Implementácia v ďalšej fáze */
    (void)filename;
    (void)verbose;
    
    /* Pre testovaciu kompiláciu vytvoríme prázdny root */
    return filter_node_create();
}

/**
 * @brief Pridá doménu do Trie
 */
int filter_add_domain(filter_node_t *root, const char *domain) {
    /* TODO: Implementácia v ďalšej fáze */
    (void)root;
    (void)domain;
    
    return -1;
}

/**
 * @brief Kontroluje či je doména blokovaná
 */
bool is_domain_blocked(const filter_node_t *root, const char *domain) {
    /* TODO: Implementácia v ďalšej fáze */
    (void)root;
    (void)domain;
    
    return false;
}

/**
 * @brief Normalizuje doménové meno
 */
int normalize_domain(const char *domain, char *normalized, size_t len) {
    /* TODO: Implementácia v ďalšej fáze */
    (void)domain;
    (void)normalized;
    (void)len;
    
    return -1;
}

/**
 * @brief Vypíše štatistiky o filtroch (ak verbose)
 */
void filter_print_stats(const filter_node_t *root, bool verbose) {
    /* TODO: Implementácia v ďalšej fáze */
    (void)root;
    (void)verbose;
}