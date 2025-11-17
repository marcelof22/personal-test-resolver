/**
 * @file filter.c
 * @author Marcel Feiler (xfeile00)
 * @date 10.11.2025
 * @brief Filtering DNS Resolver
 */

 #include "filter.h"
 #include "utils.h"
 
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <ctype.h>
 #include <stdbool.h>
 
 /* Inicializálna kapacita pre children array */
 #define INITIAL_CHILDREN_CAPACITY 4
 
 /* Štatistiky pre verbose výstup */
 typedef struct {
     size_t total_domains;
     size_t total_nodes;
     size_t max_depth;
 } filter_stats_t;
 
 /* Forward deklarácie pre rekurzívne funkcie */
 static void count_stats_recursive(const filter_node_t *node, size_t depth, filter_stats_t *stats);
 
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
  * @brief Normalizuje doménové meno
  * 
  * Normalizácia zahŕňa:
  * - Konverzia na lowercase (case-insensitive DNS)
  * - Odstránenie trailing dot ('.')
  * - Odstránenie whitespace na začiatku/konci
  * 
  * Edge cases:
  * - NULL domain
  * - Prázdna doména
  * - Buffer príliš malý
  * - Doména iba z bodiek/whitespace
  */
 int normalize_domain(const char *domain, char *normalized, size_t len) {
     if (domain == NULL || normalized == NULL || len == 0) {
         return -1;
     }
     
     /* Skip whitespace na začiatku */
     while (*domain && isspace((unsigned char)*domain)) {
         domain++;
     }
     
     /* Prázdna doména po odstránení whitespace */
     if (*domain == '\0') {
         return -1;
     }
     
     /* Kopírovanie a lowercase konverzia */
     size_t i = 0;
     while (*domain && i < len - 1) {
         /* Skip whitespace uprostred (neplatné v doménach, ale buďme robustní) */
         if (isspace((unsigned char)*domain)) {
             domain++;
             continue;
         }
         
         normalized[i++] = (char)tolower((unsigned char)*domain);
         domain++;
     }
     
     /* Kontrola či sa celá doména zmestila */
     if (*domain != '\0' && i >= len - 1) {
         return -1;  /* Doména príliš dlhá */
     }
     
     /* Odstránenie trailing dot */
     while (i > 0 && normalized[i - 1] == '.') {
         i--;
     }

     /* Doména iba z bodiek */
     if (i == 0) {
         return -1;
     }

     normalized[i] = '\0';

     /* Kontrola consecutive dots a leading dot */
     for (size_t j = 0; j < i; j++) {
         if (normalized[j] == '.') {
             if (j == 0 || (j + 1 < i && normalized[j + 1] == '.')) {
                 return -1;  /* Leading dot alebo consecutive dots */
             }
         }
     }

     return 0;
 }
 
 /**
  * @brief Rozdelí doménu na labels (v reverznom poradí)
  * 
  * Príklad: "ads.google.com" -> ["com", "google", "ads"]
  * 
  * @param domain Normalizovaná doména
  * @param labels Výstupné pole (alokuje sa)
  * @param count Počet labels (výstup)
  * @return 0 pri úspechu, -1 pri chybe
  */
 static int split_domain_labels(const char *domain, char ***labels, size_t *count) {
     if (domain == NULL || labels == NULL || count == NULL) {
         return -1;
     }
     
     /* Spočítame počet labels (počet bodiek + 1) */
     size_t label_count = 1;
     for (const char *p = domain; *p; p++) {
         if (*p == '.') {
             label_count++;
         }
     }
     
     /* Alokácia poľa pre labels */
     char **label_array = (char **)malloc(label_count * sizeof(char *));
     if (label_array == NULL) {
         return -1;
     }
     
     /* Inicializácia na NULL pre bezpečné uvoľnenie pri chybe */
     for (size_t i = 0; i < label_count; i++) {
         label_array[i] = NULL;
     }
     
     /* Rozdelenie domény na labels (v reverznom poradí) */
     const char *end = domain + strlen(domain);
     size_t idx = 0;
     
     while (end > domain) {
         /* Nájdeme predchádzajúcu bodku alebo začiatok */
         const char *start = end - 1;
         while (start > domain && *(start - 1) != '.') {
             start--;
         }
         if (start > domain && *(start - 1) == '.') {
             /* Nič, start je na správnej pozícii */
         } else if (start == domain) {
             /* Začiatok domény */
         }
         
         /* Dĺžka labelu */
         size_t label_len = (size_t)(end - start);
         
         /* Edge case: prázdny label (napr. "..") */
         if (label_len == 0) {
             /* Uvoľnenie už alokovaných labels */
             for (size_t i = 0; i < idx; i++) {
                 free(label_array[i]);
             }
             free(label_array);
             return -1;
         }
         
         /* Edge case: label dlhší ako 63 znakov (RFC 1035) */
         if (label_len > DNS_MAX_LABEL_LEN) {
             for (size_t i = 0; i < idx; i++) {
                 free(label_array[i]);
             }
             free(label_array);
             return -1;
         }
         
         /* Alokácia a kopírovanie labelu */
         label_array[idx] = (char *)malloc(label_len + 1);
         if (label_array[idx] == NULL) {
             for (size_t i = 0; i < idx; i++) {
                 free(label_array[i]);
             }
             free(label_array);
             return -1;
         }
         
         memcpy(label_array[idx], start, label_len);
         label_array[idx][label_len] = '\0';
         idx++;
         
         /* Posun na ďalší label */
         if (start > domain) {
             end = start - 1;  /* Preskočiť bodku */
         } else {
             break;
         }
     }
     
     *labels = label_array;
     *count = label_count;
     return 0;
 }
 
 /**
  * @brief Nájde child node s daným labelom
  * 
  * @return Pointer na child alebo NULL ak neexistuje
  */
 static filter_node_t *find_child(const filter_node_t *node, const char *label) {
     if (node == NULL || label == NULL) {
         return NULL;
     }
     
     for (size_t i = 0; i < node->children_count; i++) {
         if (node->children[i]->label != NULL && 
             strcmp(node->children[i]->label, label) == 0) {
             return node->children[i];
         }
     }
     
     return NULL;
 }
 
 /**
  * @brief Pridá child node do parent node
  * 
  * Dynamicky rozširuje children array pri potrebe.
  */
 static int add_child(filter_node_t *parent, filter_node_t *child) {
     if (parent == NULL || child == NULL) {
         return -1;
     }
     
     /* Rozšírenie kapacity ak je potrebné */
     if (parent->children_count >= parent->children_capacity) {
         size_t new_capacity = parent->children_capacity == 0 ? 
                               INITIAL_CHILDREN_CAPACITY : 
                               parent->children_capacity * 2;
         
         filter_node_t **new_children = (filter_node_t **)realloc(
             parent->children, 
             new_capacity * sizeof(filter_node_t *)
         );
         
         if (new_children == NULL) {
             return -1;
         }
         
         parent->children = new_children;
         parent->children_capacity = new_capacity;
     }
     
     /* Pridanie child */
     parent->children[parent->children_count++] = child;
     return 0;
 }
 
 /**
  * @brief Pridá doménu do Trie
  * 
  * Domény sa ukladajú v reverznom poradí (TLD najprv).
  * Príklad: "ads.google.com" -> com -> google -> ads
  * 
  * Edge cases:
  * - NULL root/domain
  * - Prázdna doména
  * - Duplicitné domény (nie je chyba, iba nastaví is_blocked)
  * - Neplatné doménové meno
  */
 int filter_add_domain(filter_node_t *root, const char *domain) {
     if (root == NULL || domain == NULL) {
         return -1;
     }
     
     /* Normalizácia domény */
     char normalized[DNS_MAX_NAME_LEN + 1];
     if (normalize_domain(domain, normalized, sizeof(normalized)) != 0) {
         return -1;
     }
     
     /* Rozdelenie na labels */
     char **labels = NULL;
     size_t label_count = 0;
     if (split_domain_labels(normalized, &labels, &label_count) != 0) {
         return -1;
     }
     
     /* Prechádzanie/vytváranie Trie */
     filter_node_t *current = root;
     
     for (size_t i = 0; i < label_count; i++) {
         /* Hľadáme existujúci child */
         filter_node_t *child = find_child(current, labels[i]);
         
         if (child == NULL) {
             /* Child neexistuje, vytvoríme nový */
             child = filter_node_create();
             if (child == NULL) {
                 /* Cleanup */
                 for (size_t j = 0; j < label_count; j++) {
                     free(labels[j]);
                 }
                 free(labels);
                 return -1;
             }
             
             /* Skopírujeme label */
             child->label = strdup(labels[i]);
             if (child->label == NULL) {
                 filter_node_free(child);
                 for (size_t j = 0; j < label_count; j++) {
                     free(labels[j]);
                 }
                 free(labels);
                 return -1;
             }
             
             /* Pridáme child do parent */
             if (add_child(current, child) != 0) {
                 filter_node_free(child);
                 for (size_t j = 0; j < label_count; j++) {
                     free(labels[j]);
                 }
                 free(labels);
                 return -1;
             }
         }
         
         current = child;
     }
     
     /* Označíme koncový node ako blokovaný */
     current->is_blocked = true;
     
     /* Cleanup labels */
     for (size_t i = 0; i < label_count; i++) {
         free(labels[i]);
     }
     free(labels);
     
     return 0;
 }
 
 /**
  * @brief Kontroluje či je doména blokovaná
  * 
  * Kontroluje aj všetky subdomény:
  * Ak je blokovaná "google.com", tak aj "ads.google.com" je blokovaná.
  * 
  * Edge cases:
  * - NULL root/domain
  * - Prázdna doména
  * - Case-insensitive matching
  */
 bool is_domain_blocked(const filter_node_t *root, const char *domain) {
     if (root == NULL || domain == NULL) {
         return false;
     }
     
     /* Normalizácia domény */
     char normalized[DNS_MAX_NAME_LEN + 1];
     if (normalize_domain(domain, normalized, sizeof(normalized)) != 0) {
         return false;
     }
     
     /* Rozdelenie na labels */
     char **labels = NULL;
     size_t label_count = 0;
     if (split_domain_labels(normalized, &labels, &label_count) != 0) {
         return false;
     }
     
     /* Prechádzanie Trie */
     const filter_node_t *current = root;
     bool is_blocked = false;
     
     for (size_t i = 0; i < label_count; i++) {
         /* Hľadáme child */
         const filter_node_t *child = find_child(current, labels[i]);
         
         if (child == NULL) {
             /* Label neexistuje v Trie -> doména nie je blokovaná */
             break;
         }
         
         /* Ak je tento node blokovaný, celá doména je blokovaná */
         if (child->is_blocked) {
             is_blocked = true;
             break;
         }
         
         current = child;
     }
     
     /* Cleanup */
     for (size_t i = 0; i < label_count; i++) {
         free(labels[i]);
     }
     free(labels);
     
     return is_blocked;
 }
 
 /**
  * @brief Načíta filter súbor a vytvorí Trie štruktúru
  * 
  * Formát súboru:
  * - Každá doména na samostatnom riadku
  * - Prázdne riadky ignorovať
  * - Riadky začínajúce '#' ignorovať (komentáre)
  * - Podporuje LF (Unix), CRLF (Windows), CR (Mac OS) konce riadkov
  * 
  * Edge cases:
  * - Neexistujúci súbor
  * - Prázdny súbor
  * - Súbor iba s komentármi
  * - Duplicitné domény (nie je chyba)
  * - Neplatné doménové mená (ignorujú sa s varovaním)
  * - Príliš dlhé riadky
  */
 filter_node_t *load_filter_file(const char *filename, bool verbose) {
     if (filename == NULL) {
         print_error("Filter filename is NULL");
         return NULL;
     }
     
     /* Otvorenie súboru */
     FILE *file = fopen(filename, "r");
     if (file == NULL) {
         print_error("Cannot open filter file: %s", filename);
         return NULL;
     }
     
     /* Vytvorenie root node */
     filter_node_t *root = filter_node_create();
     if (root == NULL) {
         print_error("Failed to create filter root node");
         fclose(file);
         return NULL;
     }
     
     /* Čítanie súboru po riadkoch */
     char line[DNS_MAX_NAME_LEN * 2];  /* Extra priestor pre dlhé riadky */
     size_t line_num = 0;
     size_t domains_loaded = 0;
     size_t lines_ignored = 0;
     
     while (fgets(line, sizeof(line), file) != NULL) {
         line_num++;
         
         /* Odstránenie všetkých typov koncov riadkov: \n, \r\n, \r */
         size_t len = strlen(line);
         while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
             line[--len] = '\0';
         }
         
         /* Skip prázdne riadky */
         if (len == 0) {
             continue;
         }
         
         /* Skip whitespace na začiatku */
         char *domain = line;
         while (*domain && isspace((unsigned char)*domain)) {
             domain++;
         }
         
         /* Skip prázdne riadky po odstránení whitespace */
         if (*domain == '\0') {
             continue;
         }
         
         /* Skip komentáre */
         if (*domain == '#') {
             lines_ignored++;
             continue;
         }
         
         /* Pridanie domény do Trie */
         if (filter_add_domain(root, domain) == 0) {
             domains_loaded++;
         } else {
             /* Neplatná doména - vypisujeme warning len ak verbose */
             if (verbose) {
                 printf("[VERBOSE] Warning: Invalid domain on line %zu: %s\n", 
                        line_num, domain);
             }
             lines_ignored++;
         }
     }
     
     fclose(file);
     
     /* Štatistiky */
     if (verbose) {
         printf("[VERBOSE] Filter file loaded: %zu domains, %zu lines ignored\n", 
                domains_loaded, lines_ignored);
     }
     
     /* Edge case: žiadne domény */
     if (domains_loaded == 0) {
         if (verbose) {
             printf("[VERBOSE] Warning: No valid domains found in filter file\n");
         }
     }
     
     return root;
 }
 
 /**
  * @brief Rekurzívne počítanie štatistík
  */
 static void count_stats_recursive(const filter_node_t *node, size_t depth, 
                                   filter_stats_t *stats) {
     if (node == NULL || stats == NULL) {
         return;
     }
     
     stats->total_nodes++;
     
     if (node->is_blocked) {
         stats->total_domains++;
     }
     
     if (depth > stats->max_depth) {
         stats->max_depth = depth;
     }
     
     for (size_t i = 0; i < node->children_count; i++) {
         count_stats_recursive(node->children[i], depth + 1, stats);
     }
 }
 
 /**
  * @brief Vypíše štatistiky o filtroch (ak verbose)
  */
 void filter_print_stats(const filter_node_t *root, bool verbose) {
     if (!verbose || root == NULL) {
         return;
     }
     
     filter_stats_t stats = {0, 0, 0};
     
     /* Počítanie štatistík zo všetkých children root node */
     for (size_t i = 0; i < root->children_count; i++) {
         count_stats_recursive(root->children[i], 1, &stats);
     }
     
     printf("[VERBOSE] Filter statistics:\n");
     printf("[VERBOSE]   Total blocked domains: %zu\n", stats.total_domains);
     printf("[VERBOSE]   Total Trie nodes: %zu\n", stats.total_nodes);
     printf("[VERBOSE]   Maximum depth: %zu\n", stats.max_depth);
     
     if (stats.total_nodes > 0) {
         double avg_branching = (double)stats.total_nodes / 
                               (stats.max_depth > 0 ? stats.max_depth : 1);
         printf("[VERBOSE]   Average branching factor: %.2f\n", avg_branching);
     }
 }
/* ============================================================================
 * WRAPPER API PRE TESTY
 * ============================================================================ */

/**
 * @brief Inicializuje nový filter
 */
filter_t *filter_init(void) {
    filter_t *filter = (filter_t *)malloc(sizeof(filter_t));
    if (filter == NULL) {
        return NULL;
    }

    filter->root = filter_node_create();
    if (filter->root == NULL) {
        free(filter);
        return NULL;
    }

    return filter;
}

/**
 * @brief Uvoľní filter
 */
void filter_free(filter_t *filter) {
    if (filter == NULL) {
        return;
    }

    if (filter->root != NULL) {
        filter_node_free(filter->root);
    }

    free(filter);
}

/**
 * @brief Pridá doménu do filtra
 */
int filter_insert(filter_t *filter, const char *domain) {
    if (filter == NULL || filter->root == NULL) {
        return -1;
    }

    return filter_add_domain(filter->root, domain);
}

/**
 * @brief Kontroluje či je doména blokovaná
 */
bool filter_lookup(const filter_t *filter, const char *domain) {
    if (filter == NULL || filter->root == NULL) {
        return false;
    }

    return is_domain_blocked(filter->root, domain);
}
