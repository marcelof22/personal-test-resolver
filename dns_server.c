/**
 * @file dns_server.c
 * @brief UDP DNS Server implementation
 */

#include "dns_server.h"
#include "utils.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

/**
 * @brief Inicializuje UDP socket na zadanom porte
 */
int init_udp_server(uint16_t port) {
    int sockfd;
    struct sockaddr_in server_addr;
    int reuse = 1;
    
    /* Vytvorenie UDP socketu */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        print_error("Failed to create socket");
        return -1;
    }
    
    /* SO_REUSEADDR pre rýchly restart */
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        print_error("Failed to set SO_REUSEADDR");
        close(sockfd);
        return -1;
    }
    
    /* Nastavenie server adresy */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    
    /* Bind na port */
    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        print_error("Failed to bind to port %u", port);
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

/**
 * @brief Hlavná slučka DNS servera
 */
int run_server(server_config_t *config) {
    /* TODO: Implementácia v ďalšej fáze */
    (void)config;  /* Unused parameter */
    
    print_error("DNS server not yet implemented");
    return ERR_SUCCESS;
}