/**
 * @file main.c
 * @brief DNS Resolver - hlavný entry point
 * @author ISA projekt
 * 
 * Použitie: dns -s server [-p port] -f filter_file [-v]
 */

#include "dns.h"
#include "dns_server.h"
#include "dns_parser.h"
#include "dns_builder.h"
#include "filter.h"
#include "resolver.h"
#include "utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>

/* Globálna konfigurácia pre signal handling */
static server_config_t *g_config = NULL;

/**
 * @brief Signal handler pre SIGINT (Ctrl+C) a SIGTERM
 */
void signal_handler(int signum) {
    (void)signum;  /* Unused */
    
    printf("\nShutting down DNS resolver...\n");
    
    /* Cleanup */
    if (g_config != NULL) {
        if (g_config->filter_root != NULL) {
            filter_node_free(g_config->filter_root);
        }
        if (g_config->upstream_server != NULL) {
            free(g_config->upstream_server);
        }
        if (g_config->filter_file != NULL) {
            free(g_config->filter_file);
        }
        free(g_config);
    }
    
    exit(0);
}

/**
 * @brief Inicializuje server_config_t s defaultnými hodnotami
 */
server_config_t *init_config(void) {
    server_config_t *config = (server_config_t *)malloc(sizeof(server_config_t));
    if (config == NULL) {
        print_error("Failed to allocate memory for configuration");
        return NULL;
    }
    
    /* Defaultné hodnoty */
    config->upstream_server = NULL;
    config->local_port = DNS_DEFAULT_PORT;
    config->filter_file = NULL;
    config->verbose = false;
    config->filter_root = NULL;
    
    return config;
}

/**
 * @brief Parsuje command-line argumenty
 * 
 * Edge cases:
 * - Chýbajúce povinné parametre (-s, -f)
 * - Duplicitné parametre
 * - Neplatné číslo portu (0, > 65535, neplatný formát)
 * - Neexistujúci filter súbor (kontrola až pri načítaní)
 * - Neznáme parametre
 * - Prázdne hodnoty parametrov
 */
int parse_arguments(int argc, char *argv[], server_config_t *config) {
    int opt;
    bool has_server = false;
    bool has_filter = false;
    
    /* getopt pre parsing argumentov */
    while ((opt = getopt(argc, argv, "s:p:f:vh")) != -1) {
        switch (opt) {
            case 's':
                /* Upstream server */
                if (has_server) {
                    print_error("Duplicate -s parameter");
                    return -1;
                }
                if (optarg == NULL || strlen(optarg) == 0) {
                    print_error("Empty server address");
                    return -1;
                }
                config->upstream_server = strdup(optarg);
                if (config->upstream_server == NULL) {
                    print_error("Memory allocation failed for server address");
                    return -1;
                }
                has_server = true;
                break;
                
            case 'p':
                /* Port */
                if (optarg == NULL || strlen(optarg) == 0) {
                    print_error("Empty port number");
                    return -1;
                }
                
                /* Konverzia na číslo s error checkingom */
                char *endptr;
                long port = strtol(optarg, &endptr, 10);
                
                if (*endptr != '\0') {
                    print_error("Invalid port number: '%s' (non-numeric characters)", optarg);
                    return -1;
                }
                if (port <= 0 || port > 65535) {
                    print_error("Port number out of range: %ld (must be 1-65535)", port);
                    return -1;
                }
                
                config->local_port = (uint16_t)port;
                
                /* Varovanie pri privilegovaných portoch */
                if (port < 1024 && geteuid() != 0) {
                    print_error("Port %ld requires root privileges", port);
                    return -1;
                }
                break;
                
            case 'f':
                /* Filter file */
                if (has_filter) {
                    print_error("Duplicate -f parameter");
                    return -1;
                }
                if (optarg == NULL || strlen(optarg) == 0) {
                    print_error("Empty filter file path");
                    return -1;
                }
                config->filter_file = strdup(optarg);
                if (config->filter_file == NULL) {
                    print_error("Memory allocation failed for filter file path");
                    return -1;
                }
                has_filter = true;
                break;
                
            case 'v':
                /* Verbose mode */
                config->verbose = true;
                break;
                
            case 'h':
                /* Help */
                print_usage(argv[0]);
                return 1;  /* Return 1 to indicate help was printed */
                
            case '?':
                /* Unknown option */
                print_error("Unknown option: -%c", optopt);
                print_usage(argv[0]);
                return -1;
                
            default:
                print_usage(argv[0]);
                return -1;
        }
    }
    
    /* Check pre extra non-option argumenty */
    if (optind < argc) {
        print_error("Unexpected argument: %s", argv[optind]);
        print_usage(argv[0]);
        return -1;
    }
    
    /* Validácia povinných parametrov */
    if (!has_server) {
        print_error("Missing required parameter: -s (upstream server)");
        print_usage(argv[0]);
        return -1;
    }
    
    if (!has_filter) {
        print_error("Missing required parameter: -f (filter file)");
        print_usage(argv[0]);
        return -1;
    }
    
    return 0;
}

/**
 * @brief Hlavná funkcia programu
 */
int main(int argc, char *argv[]) {
    int ret = ERR_SUCCESS;
    
    /* Inicializácia konfigurácie */
    g_config = init_config();
    if (g_config == NULL) {
        return ERR_MEMORY;
    }
    
    /* Parsovanie argumentov */
    ret = parse_arguments(argc, argv, g_config);
    if (ret != 0) {
        if (ret > 0) {
            /* Help bol vypísaný */
            free(g_config);
            return ERR_SUCCESS;
        }
        /* Chyba pri parsovaní */
        if (g_config->upstream_server != NULL) free(g_config->upstream_server);
        if (g_config->filter_file != NULL) free(g_config->filter_file);
        free(g_config);
        return ERR_INVALID_ARGS;
    }
    
    /* Verbose output */
    verbose_log(g_config, "DNS Resolver starting...");
    verbose_log(g_config, "Upstream server: %s", g_config->upstream_server);
    verbose_log(g_config, "Local port: %u", g_config->local_port);
    verbose_log(g_config, "Filter file: %s", g_config->filter_file);
    
    /* Načítanie filter súboru */
    verbose_log(g_config, "Loading filter file...");
    g_config->filter_root = load_filter_file(g_config->filter_file, g_config->verbose);
    if (g_config->filter_root == NULL) {
        print_error("Failed to load filter file: %s", g_config->filter_file);
        free(g_config->upstream_server);
        free(g_config->filter_file);
        free(g_config);
        return ERR_FILTER_FILE;
    }
    
    /* Vypísať štatistiky filtrov */
    filter_print_stats(g_config->filter_root, g_config->verbose);
    
    /* Signal handling pre graceful shutdown */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* Spustenie DNS servera */
    verbose_log(g_config, "Starting DNS server on port %u...", g_config->local_port);
    ret = run_server(g_config);
    
    /* Cleanup (v prípade že server končí bez signálu) */
    if (g_config->filter_root != NULL) {
        filter_node_free(g_config->filter_root);
    }
    if (g_config->upstream_server != NULL) {
        free(g_config->upstream_server);
    }
    if (g_config->filter_file != NULL) {
        free(g_config->filter_file);
    }
    free(g_config);
    
    return ret;
}