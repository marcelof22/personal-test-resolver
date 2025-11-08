/**
 * @file dns_server.h
 * @brief UDP DNS Server implementation
 */

#ifndef DNS_SERVER_H
#define DNS_SERVER_H

#include "dns.h"

/**
 * @brief Inicializuje UDP socket na zadanom porte
 * @param port Port number (default DNS_DEFAULT_PORT)
 * @return Socket file descriptor alebo -1 pri chybe
 */
int init_udp_server(uint16_t port);

/**
 * @brief Hlavná slučka DNS servera
 * @param config Server konfigurácia
 * @return ERR_SUCCESS alebo chybový kód
 */
int run_server(server_config_t *config);

#endif /* DNS_SERVER_H */