# Makefile pre Filtering DNS Resolver
# Autor: Marcel Feiler (xfeile00)
# Dátum: 10.11.2025

# Kompilátor a flagy
CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pedantic -g
LDFLAGS = -lpthread

# Súbory
SOURCES = main.c dns_server.c dns_parser.c dns_builder.c filter.c resolver.c utils.c
HEADERS = dns.h dns_server.h dns_parser.h dns_builder.h filter.h resolver.h utils.h
OBJECTS = $(SOURCES:.c=.o)
TARGET = dns

# Test súbory
TEST_SOURCES = test_filter.c test_dns_parser.c test_dns_builder.c test_dns_server.c test_resolver.c test_integration.c
TEST_TARGETS = test_filter test_dns_parser test_dns_builder test_dns_server test_resolver test_integration

# Farby pre výstup
COLOR_RESET = \033[0m
COLOR_GREEN = \033[32m
COLOR_YELLOW = \033[33m
COLOR_BLUE = \033[34m


# HLAVNÉ CIELE

.PHONY: all clean test help

all: $(TARGET)
	@echo "$(COLOR_GREEN) Build successful!$(COLOR_RESET)"
	@echo "$(COLOR_BLUE)Usage: ./$(TARGET) -s <server> [-p port] -f <filter_file> [-v]$(COLOR_RESET)"

# Linkovanie
$(TARGET): $(OBJECTS)
	@echo "$(COLOR_YELLOW)Linking $(TARGET)...$(COLOR_RESET)"
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Kompilácia object súborov
%.o: %.c $(HEADERS)
	@echo "$(COLOR_YELLOW)Compiling $<...$(COLOR_RESET)"
	$(CC) $(CFLAGS) -c $< -o $@

# ČISTENIE

clean:
	@echo "$(COLOR_YELLOW)Cleaning...$(COLOR_RESET)"
	rm -f $(OBJECTS) $(TARGET)
	rm -f $(TEST_TARGETS) test_*.o
	rm -f *.core core
	rm -f vgcore.*
	rm -f valgrind.log
	@echo "$(COLOR_GREEN) Clean complete$(COLOR_RESET)"


# TESTOVANIE

# Hlavný test target (REQUIRED BY ASSIGNMENT)
test: $(TEST_TARGETS)
	@echo "$(COLOR_BLUE)Running all tests...$(COLOR_RESET)"
	@./test_filter
	@./test_dns_parser
	@./test_dns_builder
	@./test_dns_server
	@./test_resolver
	@./test_integration
	@echo ""
	@echo "$(COLOR_GREEN) All tests passed!$(COLOR_RESET)"

# Alternatívny test pomocou scriptu
test_script: $(TARGET)
	@echo "$(COLOR_BLUE)Running tests via script...$(COLOR_RESET)"
	@if [ -f run_tests.sh ]; then \
		chmod +x run_tests.sh; \
		./run_tests.sh; \
	else \
		echo "$(COLOR_YELLOW)Warning: run_tests.sh not found$(COLOR_RESET)"; \
		echo "Run 'make test' instead"; \
	fi

# Kompilácia jednotlivých testov
test_filter: test_filter.o filter.o utils.o
	@echo "$(COLOR_YELLOW)Building test_filter...$(COLOR_RESET)"
	$(CC) $(CFLAGS) -o test_filter test_filter.o filter.o utils.o

test_dns_parser: test_dns_parser.o dns_parser.o utils.o
	@echo "$(COLOR_YELLOW)Building test_dns_parser...$(COLOR_RESET)"
	$(CC) $(CFLAGS) -o test_dns_parser test_dns_parser.o dns_parser.o utils.o

test_dns_builder: test_dns_builder.o dns_builder.o dns_parser.o utils.o
	@echo "$(COLOR_YELLOW)Building test_dns_builder...$(COLOR_RESET)"
	$(CC) $(CFLAGS) -o test_dns_builder test_dns_builder.o dns_builder.o dns_parser.o utils.o

test_dns_server: test_dns_server.o dns_server.o dns_parser.o dns_builder.o filter.o resolver.o utils.o
	@echo "$(COLOR_YELLOW)Building test_dns_server...$(COLOR_RESET)"
	$(CC) $(CFLAGS) -o test_dns_server test_dns_server.o dns_server.o dns_parser.o dns_builder.o filter.o resolver.o utils.o

test_resolver: test_resolver.o resolver.o dns_parser.o utils.o
	@echo "$(COLOR_YELLOW)Building test_resolver...$(COLOR_RESET)"
	$(CC) $(CFLAGS) -o test_resolver test_resolver.o resolver.o dns_parser.o utils.o

test_integration: test_integration.o dns_server.o dns_parser.o dns_builder.o filter.o resolver.o utils.o
	@echo "$(COLOR_YELLOW)Building test_integration...$(COLOR_RESET)"
	$(CC) $(CFLAGS) -o test_integration test_integration.o dns_server.o dns_parser.o dns_builder.o filter.o resolver.o utils.o


# DEBUG & MEMORY CHECK


debug: CFLAGS += -DDEBUG -O0
debug: clean all
	@echo "$(COLOR_GREEN) Debug build complete$(COLOR_RESET)"

# Valgrind memory check
memcheck: $(TARGET)
	@echo "$(COLOR_BLUE)Running Valgrind memory check...$(COLOR_RESET)"
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
		--verbose --log-file=valgrind.log ./$(TARGET) -s 8.8.8.8 -f serverlist_neziaduce_domeny.txt || true
	@echo "$(COLOR_YELLOW)Check valgrind.log for details$(COLOR_RESET)"


# RELEASE BUILD


release: CFLAGS = -std=gnu99 -Wall -Wextra -Werror -O2 -DNDEBUG
release: clean all
	@echo "$(COLOR_GREEN) Release build complete$(COLOR_RESET)"
	strip $(TARGET)


# POMOCNÉ CIELE


help:
	@echo "$(COLOR_BLUE)DNS Resolver Makefile$(COLOR_RESET)"
	@echo "Available targets:"
	@echo "  make           - Build project (default)"
	@echo "  make clean     - Remove build artifacts"
	@echo "  make test      - Run unit tests (97 tests)"
	@echo "  make test_script - Run tests via run_tests.sh"
	@echo "  make debug     - Build with debug symbols"
	@echo "  make release   - Build optimized release version"
	@echo "  make memcheck  - Run valgrind memory check"
	@echo "  make help      - Show this help"

# Závislosť pre automatické generovanie dependencies
-include $(OBJECTS:.o=.d)

%.d: %.c
	@$(CC) -MM $(CFLAGS) $< > $@