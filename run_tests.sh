#!/bin/bash
# run_tests.sh - Spustenie všetkých unit testov
# Autor: Marcel Feiler (xfeile00)
# Note: Test source files are in tests/ directory, executables are built in root

set -e  # Exit on error

# Farby
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}         DNS Resolver - Unit Tests Suite${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo ""

# Kompilácia testov
echo -e "${YELLOW}[1/2] Compiling tests...${NC}"
make -s test_filter test_dns_parser test_dns_builder test_dns_server test_resolver test_integration
echo -e "${GREEN}✓ Compilation successful${NC}"
echo ""

# Spustenie testov
echo -e "${YELLOW}[2/2] Running tests...${NC}"
echo ""

TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Test 1: Filter
echo -e "${BLUE}[1/6] Filter Module Tests${NC}"
if ./test_filter; then
    PASSED_TESTS=$((PASSED_TESTS + 47))
    echo -e "${GREEN}✓ Filter: 47/47 passed${NC}"
else
    FAILED_TESTS=$((FAILED_TESTS + 1))
    echo -e "${RED}✗ Filter: FAILED${NC}"
fi
TOTAL_TESTS=$((TOTAL_TESTS + 47))
echo ""

# Test 2: DNS Parser
echo -e "${BLUE}[2/6] DNS Parser Tests${NC}"
if ./test_dns_parser; then
    PASSED_TESTS=$((PASSED_TESTS + 17))
    echo -e "${GREEN}✓ DNS Parser: 17/17 passed${NC}"
else
    FAILED_TESTS=$((FAILED_TESTS + 1))
    echo -e "${RED}✗ DNS Parser: FAILED${NC}"
fi
TOTAL_TESTS=$((TOTAL_TESTS + 17))
echo ""

# Test 3: DNS Builder
echo -e "${BLUE}[3/6] DNS Builder Tests${NC}"
if ./test_dns_builder; then
    PASSED_TESTS=$((PASSED_TESTS + 20))
    echo -e "${GREEN}✓ DNS Builder: 20/20 passed${NC}"
else
    FAILED_TESTS=$((FAILED_TESTS + 1))
    echo -e "${RED}✗ DNS Builder: FAILED${NC}"
fi
TOTAL_TESTS=$((TOTAL_TESTS + 20))
echo ""

# Test 4: DNS Server
echo -e "${BLUE}[4/6] DNS Server Tests${NC}"
if ./test_dns_server; then
    PASSED_TESTS=$((PASSED_TESTS + 5))
    echo -e "${GREEN}✓ DNS Server: 5/5 passed${NC}"
else
    FAILED_TESTS=$((FAILED_TESTS + 1))
    echo -e "${RED}✗ DNS Server: FAILED${NC}"
fi
TOTAL_TESTS=$((TOTAL_TESTS + 5))
echo ""

# Test 5: Resolver
echo -e "${BLUE}[5/6] Resolver Tests${NC}"
if ./test_resolver; then
    PASSED_TESTS=$((PASSED_TESTS + 5))
    echo -e "${GREEN}✓ Resolver: 5/5 passed${NC}"
else
    FAILED_TESTS=$((FAILED_TESTS + 1))
    echo -e "${RED}✗ Resolver: FAILED${NC}"
fi
TOTAL_TESTS=$((TOTAL_TESTS + 5))
echo ""

# Test 6: Integration
echo -e "${BLUE}[6/6] Integration Tests${NC}"
if ./test_integration; then
    PASSED_TESTS=$((PASSED_TESTS + 3))
    echo -e "${GREEN}✓ Integration: 3/3 passed${NC}"
else
    FAILED_TESTS=$((FAILED_TESTS + 1))
    echo -e "${RED}✗ Integration: FAILED${NC}"
fi
TOTAL_TESTS=$((TOTAL_TESTS + 3))
echo ""

# Zhrnutie
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}                    TEST SUMMARY${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════${NC}"
echo -e "Total Tests:    ${TOTAL_TESTS}"
echo -e "Passed:         ${GREEN}${PASSED_TESTS}${NC}"
echo -e "Failed:         ${RED}${FAILED_TESTS}${NC}"

if [ $FAILED_TESTS -eq 0 ]; then
    echo ""
    echo -e "${GREEN}✓ ALL TESTS PASSED!${NC}"
    echo ""
    exit 0
else
    echo ""
    echo -e "${RED}✗ SOME TESTS FAILED!${NC}"
    echo ""
    exit 1
fi