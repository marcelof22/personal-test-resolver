/**
 * @file utils.h
 * @author Marcel Feiler (xfeile00)
 * @date 10.11.2025
 * @brief Filtering DNS Resolver
 */

 #ifndef UTILS_H
 #define UTILS_H
 
 #include "dns.h"
 #include <stdio.h>
 #include <stdarg.h>
 
 /**
  * @brief Vypíše chybovú správu na stderr
  * @param format Printf-style format string
  */
 void print_error(const char *format, ...);
 
 /**
  * @brief Vypíše verbose log ak je verbose mode zapnutý
  * @param config Server konfigurácia
  * @param format Printf-style format string
  */
 void verbose_log(server_config_t *config, const char *format, ...);
 
 /**
  * @brief Vypíše usage informácie
  * @param program_name Názov programu
  */
 void print_usage(const char *program_name);
 
 /**
  * @brief Vypíše verbose log správu (bez config parametra)
  * @param format Printf-style formát
  * @param ... Argumenty
  * 
  * Používa sa v prípadoch kde config nie je dostupný.
  */
 void verbose_log_raw(const char *format, ...);
 
 #endif /* UTILS_H */