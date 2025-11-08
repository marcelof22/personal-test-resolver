/**
 * @file utils.c
 * @brief Implementácia pomocných funkcií
 */

 #include "utils.h"
 #include <stdio.h>
 #include <stdlib.h>
 #include <stdarg.h>
 
 /**
  * @brief Vypíše chybovú správu na stderr
  */
 void print_error(const char *format, ...) {
     va_list args;
     va_start(args, format);
     
     fprintf(stderr, "ERROR: ");
     vfprintf(stderr, format, args);
     fprintf(stderr, "\n");
     
     va_end(args);
 }
 
 /**
  * @brief Vypíše verbose log ak je verbose mode zapnutý
  */
 void verbose_log(server_config_t *config, const char *format, ...) {
     if (config == NULL || !config->verbose) {
         return;
     }
     
     va_list args;
     va_start(args, format);
     
     printf("[VERBOSE] ");
     vprintf(format, args);
     printf("\n");
     
     va_end(args);
 }
 
 /**
  * @brief Vypíše verbose log správu (bez config parametra)
  * 
  * Používa sa v prípadoch kde config nie je dostupný (napr. resolver.c).
  */
 void verbose_log_raw(const char *format, ...) {
     va_list args;
     
     fprintf(stderr, "[VERBOSE] ");
     
     va_start(args, format);
     vfprintf(stderr, format, args);
     va_end(args);
     
     fprintf(stderr, "\n");
 }
 
 /**
  * @brief Vypíše usage informácie
  */
 void print_usage(const char *program_name) {
     printf("Usage: %s -s server [-p port] -f filter_file [-v]\n", program_name);
     printf("\n");
     printf("Filtrujúci DNS resolver\n");
     printf("\n");
     printf("Povinné parametre:\n");
     printf("  -s server        IP adresa alebo hostname upstream DNS servera\n");
     printf("  -f filter_file   Súbor so zoznamom nežiadúcich domén\n");
     printf("\n");
     printf("Voliteľné parametre:\n");
     printf("  -p port          Port pre prijímanie dotazov (default: 53)\n");
     printf("  -v               Verbose mode - vypisuje informácie o preklade\n");
     printf("\n");
     printf("Príklad:\n");
     printf("  sudo %s -s 8.8.8.8 -p 5353 -f blocked_domains.txt -v\n", program_name);
     printf("\n");
 }