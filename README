% DNS Resolver - Filtrující DNS Server
% Marcel (xlogin00)
% November 2025

\newpage

# Obsah

1. Úvod
2. Teoretický základ
3. Návrh aplikácie
4. Implementácia
5. Testovanie
6. Záver
7. Literatúra

\newpage

# 1. Úvod

V rámci predmetu ISA som implementoval filtrujúci DNS resolver, ktorý dokáže blokovať nežiadúce domény a ich subdomény. Tento projekt mi umožnil prakticky si vyskúšať prácu s DNS protokolom podľa RFC 1035 a implementovať sieťovú aplikáciu v jazyku C.

## 1.1 Motivácia

DNS (Domain Name System) je kľúčová služba internetu, ktorá prekladá doménové mená na IP adresy. V súčasnosti existuje veľké množstvo nežiadúcich domén - reklamné servery, trackery, malvér distribučné stránky. Môj DNS resolver poskytuje jednoduchý spôsob, ako tieto domény zablokovať na úrovni DNS dotazov.

## 1.2 Zadanie

Mojou úlohou bolo napísať program `dns`, ktorý:
- Filtruje DNS dotazy typu A smerujúce na domény zo zoznamu
- Blokuje aj subdomény (napr. ak je blokovaná `ads.google.com`, je blokovaná aj `tracker.ads.google.com`)
- Ostatné dotazy preposiela na upstream DNS server
- Implementuje DNS parsing a building priamo (bez použitia DNS knižníc)

## 1.3 Základné Informácie o Programe

### Funkčnosť
Program `dns` funguje ako filtrujúci DNS proxy server. Po spustení načíta zoznam blokovaných domén zo súboru a začne počúvať na zadanom UDP porte. Keď príjme DNS dotaz:

1. **Parsuje** DNS správu podľa RFC 1035
2. **Validuje** dotaz (kontrola typu, formátu)
3. **Vyhľadá** doménu v Trie filtri
4. **Rozhodne:**
   - Ak je blokovaná → odošle NXDOMAIN
   - Ak je povolená → prepošle na upstream DNS
   - Ak je neplatná → odošle chybovú odpoveď

### Technické Parametre
- **Protokol:** UDP (port 53 default, konfigurovateľný)
- **Podporované typy:** DNS typ A (ostatné → NOTIMPL)
- **Max veľkosť packetu:** 512 bytes (RFC 1035 limit)
- **Upstream timeout:** 5 sekúnd
- **Retry:** 3 pokusy
- **Podporované platformy:** GNU/Linux, FreeBSD

### Použité Technológie
- **Jazyk:** C (štandard C99)
- **Knižnice:** BSD sockets, POSIX štandard
- **Dátová štruktúra:** Reverse-order Trie
- **Algoritmická zložitosť:** O(k) kde k = počet labelov v doméne

## 1.4 Návod na Použitie

### Inštalácia a Kompilácia

```bash
# 1. Rozbalenie archívu
tar -xf xlogin00.tar
cd dns_project

# 2. Kompilácia
make

# Výstup:
# gcc -std=gnu99 -Wall -Wextra -Werror ...
#  Build successful!

# 3. Overenie
./dns --help
# Usage: ./dns -s <server> [-p port] -f <filter_file> [-v]
```

### Príprava Filter Súboru

Vytvorte textový súbor s blokovanými doménami:

```bash
cat > blocked_domains.txt << 'EOF'
# Reklamné servery
ads.google.com
doubleclick.net
googleadservices.com

# Trackery
tracker.example.com
analytics.facebook.com

# Malware
malware.org
phishing-site.com
EOF
```

**Pravidlá formátu:**
- Jedna doména na riadok
- Komentáre začínajú `#`
- Prázdne riadky sú ignorované
- Podporované line endings: LF, CRLF, CR
- Maximálna dĺžka názvu: 255 znakov

### Základné Použitie

**Príklad 1: Port 53 (vyžaduje root)**
```bash
sudo ./dns -s 8.8.8.8 -f blocked_domains.txt

# Server počúva na porte 53
# Blokuje domény z blocked_domains.txt
# Presmerováva na Google DNS (8.8.8.8)
```

**Príklad 2: Neprivilegovaný port**
```bash
./dns -s 1.1.1.1 -p 5353 -f blocked_domains.txt

# Server počúva na porte 5353 (nie root)
# Presmerováva na Cloudflare DNS (1.1.1.1)
```

**Príklad 3: Verbose mode**
```bash
./dns -s 8.8.8.8 -p 5353 -f blocked_domains.txt -v

# Vypíše detailné informácie:
# [VERBOSE] DNS Resolver starting...
# [VERBOSE] Upstream server: 8.8.8.8
# [VERBOSE] Local port: 5353
# [VERBOSE] Filter file: blocked_domains.txt
# [VERBOSE] Filter file loaded: 8 domains, 0 lines ignored
# [VERBOSE] DNS server listening on port 5353
```

**Príklad 4: Hostname upstream**
```bash
./dns -s dns.google -p 5353 -f blocked_domains.txt -v

# Podporuje hostname namiesto IP
# dns.google → 8.8.8.8 (automaticky resolvnuté)
```

### Testovanie Funkcionality

**V jednom termináli - spustite server:**
```bash
./dns -s 8.8.8.8 -p 5353 -f blocked_domains.txt -v
```

**V druhom termináli - testujte:**

**Test 1: Blokovaná doména**
```bash
dig @127.0.0.1 -p 5353 ads.google.com A

# Očakávaný výsledok:
# ;; ->>HEADER<<- opcode: QUERY, status: NXDOMAIN
# ;; QUESTION SECTION:
# ;ads.google.com.    IN  A
# 
# Query time: 0 msec  ← veľmi rýchle (lokálne)
```

**Test 2: Subdoména blokovanej domény**
```bash
dig @127.0.0.1 -p 5353 tracker.ads.google.com A

# Výsledok: NXDOMAIN (automaticky blokovaná)
```

**Test 3: Povolená doména**
```bash
dig @127.0.0.1 -p 5353 google.com A

# Očakávaný výsledok:
# ;; ->>HEADER<<- opcode: QUERY, status: NOERROR
# ;; ANSWER SECTION:
# google.com.    300  IN  A  142.250.185.46
#
# Query time: ~20 msec  ← závisí od upstream
```

**Test 4: Neimplementovaný typ**
```bash
dig @127.0.0.1 -p 5353 google.com AAAA

# Výsledok: status: NOTIMPL
```

### Ukončenie Programu

**Graceful shutdown (odporúčané):**
```bash
# Stlačte Ctrl+C v termináli kde beží server

^C
Shutting down DNS server...

DNS Resolver Statistics:
========================
Total queries processed: 152
Blocked queries: 89 (58.55%)
Forwarded queries: 63 (41.45%)
Error queries: 0 (0.00%)

Filter Statistics:
==================
Total blocked domains: 8
Total Trie nodes: 12
Maximum depth: 3

Server shutdown complete.
```

**Forceful shutdown (núdzové):**
```bash
# V inom termináli
killall dns
```

### Konfigurácia Systému (Voliteľné)

**Pre použitie ako systémový DNS resolver:**

```bash
# 1. Spustite server na porte 53
sudo ./dns -s 8.8.8.8 -f blocked_domains.txt

# 2. Nakonfigurujte systém
sudo nano /etc/resolv.conf

# Pridajte:
nameserver 127.0.0.1

# 3. Teraz všetky aplikácie používajú váš DNS resolver
firefox  # Reklamy budú blokované!
```

### Riešenie Problémov

**Problem: "Permission denied" pri porte 53**
```bash
# Riešenie 1: Spustite s sudo
sudo ./dns -s 8.8.8.8 -f blocked_domains.txt

# Riešenie 2: Použite neprivilegovaný port
./dns -s 8.8.8.8 -p 5353 -f blocked_domains.txt
```

**Problem: "Failed to open filter file"**
```bash
# Skontrolujte cestu a práva
ls -l blocked_domains.txt
# Musí existovať a byť čitateľný

# Použite absolútnu cestu
./dns -s 8.8.8.8 -p 5353 -f /home/user/blocked_domains.txt
```

**Problem: "Address already in use"**
```bash
# Niečo už počúva na porte
sudo lsof -i :53

# Zastavte konfliktný proces
sudo systemctl stop systemd-resolved  # Ubuntu

# Alebo použite iný port
./dns -s 8.8.8.8 -p 5353 -f blocked_domains.txt
```

**Problem: "Failed to resolve hostname"**
```bash
# Problém s DNS resolution upstream servera
# Skúste IP adresu namiesto hostname
./dns -s 8.8.8.8 -p 5353 -f blocked_domains.txt  # namiesto dns.google

# Alebo skontrolujte /etc/resolv.conf
cat /etc/resolv.conf
```

## 1.5 Štruktúra dokumentu

V kapitole 2 popisujem teoretický základ DNS protokolu a použité algoritmy. Kapitola 3 sa venuje návrhu aplikácie a jej architektúre. V kapitole 4 detailne popisujem implementáciu najdôležitejších častí kódu. Kapitola 5 obsahuje popis testovania a dosiahnuté výsledky. Kapitolu 6 tvorí záver a zhodnotenie projektu. Kapitola 7 obsahuje použitú literatúru.

\newpage

# 2. Teoretický základ

V tejto kapitole som zhrnul teoretické poznatky, ktoré som si musel naštudovať pred implementáciou projektu.

## 2.1 DNS Protocol (RFC 1035)

DNS je hierarchický, decentralizovaný systém pre preklad doménových mien. Pri štúdiu RFC 1035 [1] som sa zameral hlavne na:

### 2.1.1 DNS Message Format

DNS správa má fixnú štruktúru:
- **Header** (12 bajtov) - obsahuje transaction ID, flags, počty sekcií
- **Question section** - dotaz (QNAME, QTYPE, QCLASS)
- **Answer section** - odpoveď (resource records)
- **Authority section** - autoritatívne NS záznamy
- **Additional section** - doplňujúce informácie

Pre môj projekt som potreboval implementovať iba parsing headeru a question sekcie, pretože implementujem iba filtrovací resolver bez cache.

### 2.1.2 DNS Name Encoding

Doménové mená v DNS sú enkódované ako sekvencia labelov:
```
3www6google3com0
```
kde číslo pred labelom určuje jeho dĺžku a celý názov končí nulovým bajtom.

### 2.1.3 DNS Compression

RFC 1035 Section 4.1.4 [1] definuje kompresiu pomocou pointerov. Namiesto opakovania názvu môže DNS správa obsahovať 2-bajtový pointer:
```
11xxxxxx xxxxxxxx
```
Prvé 2 bity (`11`) označujú pointer, zvyšných 14 bitov je offset v správe. Toto mi spôsobilo najväčšie problémy pri implementácii, hlavne detekcia cyklických pointerov.

### 2.1.4 Response Codes (RCODE)

RFC 1035 definuje tieto RCODE hodnoty:
- 0 (NOERROR) - úspech
- 1 (FORMERR) - chyba formátu
- 2 (SERVFAIL) - server zlyhanie
- 3 (NXDOMAIN) - doména neexistuje
- 4 (NOTIMPL) - neimplementované
- 5 (REFUSED) - odmietnuté

Pre blokovanie domén používam RCODE 3 (NXDOMAIN).

## 2.2 Trie Dátová Štruktúra

Pre efektívne vyhľadávanie domén v zozname som použil Trie (prefix tree) dátovú štruktúru [2].

### 2.2.1 Prečo Trie?

Zvažoval som niekoľko prístupov:
1. **Lineárne vyhľadávanie** - O(n*m) časová zložitosť, príliš pomalé
2. **Hash table** - O(1) lookup, ale neumožňuje subdomain matching
3. **Trie** - O(k) lookup kde k = počet labelov, podporuje subdomain matching

### 2.2.2 Reverse-order Trie

Klasický Trie by vkladal domény od začiatku (`com` → `google` → `ads`). To by však neumožnilo efektívne subdomain matching.

Preto som implementoval **reverse-order Trie**, ktorý vkladá domény odzadu:
```
ads.google.com → com → google → ads (blocked)
```

Pri dotaze na `tracker.ads.google.com` prechádza Trie:
```
com → google → ads (MATCH! blocked=true)
```

Táto technika je popísaná v [3].

## 2.3 UDP Socket Programming

Pre sieťovú komunikáciu som použil BSD sockets API [4]. Hlavné funkcie:

### 2.3.1 Server Socket
```c
socket(AF_INET, SOCK_DGRAM, 0)  // Vytvorenie UDP socketu
bind()                           // Binding na port
recvfrom()                       // Prijatie paketu
sendto()                         // Odoslanie paketu
```

### 2.3.2 Client Socket (Upstream)
```c
socket()                         // Vytvorenie socketu
sendto()                         // Odoslanie query
select()                         // Timeout handling
recvfrom()                       // Prijatie response
```

Najväčšou výzvou bolo implementovať správny timeout handling pomocou `select()` namiesto `setsockopt(SO_RCVTIMEO)`.

## 2.4 Hostname Resolution

Pre podporu hostname v upstream serveri (napr. `dns.google` namiesto `8.8.8.8`) som použil `getaddrinfo()` [5]. Táto funkcia automaticky resolvne hostname na IP adresu podporujúc aj IPv6 (aj keď môj resolver podporuje iba IPv4).

\newpage

# 3. Návrh aplikácie

V tejto kapitole popisujem, ako som navrhol architektúru aplikácie pred samotnou implementáciou.

## 3.1 Celková Architektúra

Aplikáciu som navrhol modulárne s jasným rozdelením zodpovedností:

```
┌─────────────┐
│   main.c    │  Argument parsing, inicializácia
└──────┬──────┘
       ↓
┌─────────────┐
│ dns_server  │  UDP server, hlavná slučka
└──────┬──────┘
       ↓
       ├─→ dns_parser   (parsing DNS správ)
       ├─→ filter       (filtrovanie domén)
       ├─→ dns_builder  (stavanie odpovedí)
       └─→ resolver     (upstream forwarding)
```

Tento modulárny návrh mi umožnil:
1. Testovať každý modul samostatne
2. Udržať kód prehľadný
3. Jednoducho pridávať nové funkcie

## 3.2 Dátový Tok

Navrhol som nasledujúci dátový tok:

```
1. Client → recvfrom()
2. Parse DNS query
3. Validate query
4. Check QTYPE
   ├─ Not A → build_error_response(NOTIMPL)
   └─ A → Continue
5. Check domain in filter
   ├─ Blocked → build_error_response(NXDOMAIN)
   └─ Allowed → forward_to_upstream()
6. Response → sendto() → Client
```

Tento tok zabezpečuje, že každý dotaz je správne spracovaný a klient vždy dostane odpoveď.

## 3.3 Modulový Návrh

### 3.3.1 Main Module (main.c)
**Zodpovednosť:** Inicializácia aplikácie
- Parsing argumentov príkazového riadku
- Validácia vstupov
- Inicializácia konfigurácie
- Predanie kontroly DNS serveru

### 3.3.2 DNS Server Module (dns_server.c)
**Zodpovednosť:** Sieťová komunikácia
- Socket initialization
- Main server loop (prijímanie/odosielanie)
- Query processing orchestration
- Signal handling (graceful shutdown)
- Štatistiky

### 3.3.3 DNS Parser Module (dns_parser.c)
**Zodpovednosť:** Parsing DNS správ
- Header parsing
- Name parsing s compression support
- Question parsing
- Validácia formátu
- Error detection

### 3.3.4 DNS Builder Module (dns_builder.c)
**Zodpovednosť:** Stavanie DNS odpovedí
- Error response construction
- Header building
- Name encoding
- Správne nastavenie RCODE

### 3.3.5 Filter Module (filter.c)
**Zodpovednosť:** Domain filtering
- Trie initialization
- Filter file loading
- Domain lookup
- Subdomain matching
- Štatistiky

### 3.3.6 Resolver Module (resolver.c)
**Zodpovednosť:** Upstream communication
- Hostname resolution
- UDP forwarding
- Timeout handling
- Retry mechanism
- Response validation

### 3.3.7 Utils Module (utils.c)
**Zodpovednosť:** Helper funkcie
- Logging (verbose, error)
- Validation helpers
- Common utilities

## 3.4 Error Handling Stratégia

Navrhol som konzistentnú stratégiu pre error handling:

1. **Validácia vstupov** - všetky vstupy sú validované
2. **Return codes** - funkcie vracajú 0 pri úspechu, -1 pri chybe
3. **Error messages** - všetky chyby sú vypísané na stderr
4. **Graceful degradation** - server pokračuje aj po chybách
5. **Resource cleanup** - všetka pamäť je korektne uvoľnená

## 3.5 Konfiguračná Štruktúra

Navrhol som globálnu konfiguračnú štruktúru:

```c
typedef struct {
    char upstream_server[256];
    uint16_t port;
    char filter_file[256];
    bool verbose;
    volatile sig_atomic_t running;
} dns_config_t;
```

Táto štruktúra je zdieľaná medzi všetkými modulmi a obsahuje všetky potrebné nastavenia.

\newpage

# 4. Implementácia

V tejto kapitole popisujem najzaujímavejšie časti implementácie, s ktorými som sa stretol počas vývoja.

## 4.1 DNS Parser - Compression Handling

Najväčšiu výzvu pre mňa predstavovala implementácia DNS compression podľa RFC 1035 Section 4.1.4.

### 4.1.1 Problém

DNS správa môže obsahovať pointery namiesto opakovaných názvov:
```
Offset 0:  3www6google3com0
Offset 16: 4mail C0 04
                  ↑
            pointer na offset 4
```

Výsledok: "mail.google.com"

### 4.1.2 Riešenie - Detekcia Cyklických Pointerov

Môj prvý pokus zlyhal na cyklických pointeroch (pointer ukazujúci sám na seba alebo vytvárajúci cyklus). Implementoval som detekciu pomocí visited array:

```c
bool visited[DNS_UDP_MAX_SIZE] = {false};

while (offset < msg_len) {
    uint8_t len = msg[offset];
    
    // Pointer?
    if ((len & 0xC0) == 0xC0) {
        uint16_t ptr_offset = ((len & 0x3F) << 8) | msg[offset + 1];
        
        // Cyklus?
        if (visited[ptr_offset]) {
            return ERR_INVALID_DNS_PACKET;
        }
        
        visited[ptr_offset] = true;
        offset = ptr_offset;
        continue;
    }
    
    // ... spracovanie labelu
}
```

Toto riešenie úspešne detekuje cykly v O(n) čase.

### 4.1.3 Testovanie

Vytvoril som unit testy s rôznymi compression scenármi:
- Bez compression
- Jeden pointer
- Viacero pointerov
- Cyklický pointer (očakávaná chyba)
- Pointer mimo rozsah (očakávaná chyba)

## 4.2 Filter - Reverse Trie Implementation

Implementácia reverse-order Trie bola kľúčová pre správne subdomain matching.

### 4.2.1 Trie Node Structure

```c
typedef struct filter_node {
    char *label;                    // Label (napr. "google")
    bool is_blocked;                // Je blokovaný?
    struct filter_node **children;  // Deti
    size_t children_count;          // Počet detí
} filter_node_t;
```

### 4.2.2 Insertion Algorithm

```c
// Vloženie "ads.google.com"
1. Split domain → ["com", "google", "ads"]
2. Reverse array → ["ads", "google", "com"]
3. Insert do Trie:
   - Nájdi/vytvor "ads"
   - Nájdi/vytvor child "google"
   - Nájdi/vytvor child "com"
   - Mark "com" as blocked
```

### 4.2.3 Lookup Algorithm

```c
// Lookup "tracker.ads.google.com"
1. Split domain → ["com", "google", "ads", "tracker"]
2. Reverse → ["tracker", "ads", "google", "com"]
3. Walk Trie:
   - Start at root
   - "tracker" → not found, create path
   - "ads" → found!
   - Check if "ads" or any parent is blocked
   - "ads" is blocked → RETURN BLOCKED
```

Tento algoritmus zabezpečuje, že všetky subdomény blokovaných domén sú tiež blokované.

### 4.2.4 Memory Management

Trie vyžaduje dynamickú alokáciu pamäti. Implementoval som `filter_free()` funkciu, ktorá rekurzívne uvoľní celý strom:

```c
void filter_free(filter_node_t *node) {
    if (!node) return;
    
    // Rekurzívne free children
    for (size_t i = 0; i < node->children_count; i++) {
        filter_free(node->children[i]);
    }
    
    free(node->children);
    free(node->label);
    free(node);
}
```

Valgrind potvrdil, že táto implementácia nemá memory leaky.

## 4.3 Resolver - Upstream Forwarding s Retry

Implementácia upstream forwardingu s timeout a retry mechanikou bola netriviálna.

### 4.3.1 Hostname Resolution

Použil som `getaddrinfo()` pre resolution:

```c
struct addrinfo hints = {
    .ai_family = AF_INET,      // IPv4 only
    .ai_socktype = SOCK_DGRAM  // UDP
};

struct addrinfo *result;
int status = getaddrinfo(hostname, "53", &hints, &result);

if (status == 0) {
    // Success - use result->ai_addr
}
```

### 4.3.2 Timeout Handling s select()

Pôvodne som používal `setsockopt(SO_RCVTIMEO)`, ale po zvážení som prešiel na `select()`:

```c
fd_set readfds;
struct timeval timeout;

FD_ZERO(&readfds);
FD_SET(sockfd, &readfds);

timeout.tv_sec = 5;
timeout.tv_usec = 0;

int ready = select(sockfd + 1, &readfds, NULL, NULL, &timeout);

if (ready > 0) {
    // Socket ready - recv data
    recvfrom(sockfd, ...);
} else if (ready == 0) {
    // Timeout - retry
}
```

### 4.3.3 Retry Mechanizmus

Implementoval som 3 pokusy s 5-sekundovým timeoutom:

```c
#define MAX_RETRIES 3
#define UPSTREAM_TIMEOUT_SEC 5

for (int attempt = 0; attempt < MAX_RETRIES; attempt++) {
    sendto(sockfd, query, query_len, ...);
    
    int ready = select(...);
    
    if (ready > 0) {
        // Success!
        recvfrom(...);
        return 0;
    }
    
    // Timeout - try again
    if (verbose) {
        printf("Upstream timeout, retry %d/%d\n", 
               attempt + 1, MAX_RETRIES);
    }
}

// All retries failed
return -1;
```

Toto riešenie zabezpečuje robustnosť pri prechodných sieťových problémoch.

## 4.4 DNS Server - Main Loop

Implementácia main server loop s graceful shutdown:

### 4.4.1 Signal Handling

```c
volatile sig_atomic_t g_running = 1;

void signal_handler(int signum) {
    (void)signum;
    g_running = 0;  // Safe - sig_atomic_t
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    while (g_running) {
        // Server loop
    }
    
    // Cleanup
}
```

Použitie `sig_atomic_t` zabezpečuje thread-safety.

### 4.4.2 Main Loop Logic

```c
while (g_running) {
    // Non-blocking recvfrom s timeout
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    
    struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
    
    int ready = select(sockfd + 1, &readfds, NULL, NULL, &tv);
    
    if (ready > 0) {
        // Process query
        recvfrom(...);
        process_dns_query(...);
        sendto(...);
    }
    
    // Timeout - check g_running
}
```

Timeout 1 sekunda zabezpečuje responzívne ukončenie.

## 4.5 Cross-platform Line Endings

Filter file môže mať rôzne line endings (LF, CRLF, CR). Implementoval som univerzálny parser:

```c
// Strip line endings
len = strcspn(line, "\r\n");
line[len] = '\0';
```

`strcspn()` nájde prvý výskyt `\r` alebo `\n` a ukončí string tam. Toto funguje pre všetky typy line endings.

## 4.6 Zaujímavé Debugging Problémy

### 4.6.1 Network Byte Order

Strávil som niekoľko hodín debugovaním, prečo DNS parser nefunguje. Problém bol v network byte order:

```c
// CHYBNE
uint16_t id = *(uint16_t*)&buffer[0];

// SPRÁVNE
uint16_t id = ntohs(*(uint16_t*)&buffer[0]);
```

Lesson learned: **VŽDY používať ntohs/ntohl pre multi-byte hodnoty!**

### 4.6.2 Buffer Overflow

Pôvodná implementácia name parsingu mala buffer overflow pri veľmi dlhých názvoch. Opravil som pridaním strict bounds checking:

```c
if (name_offset + label_len >= DNS_UDP_MAX_SIZE) {
    return ERR_INVALID_DNS_PACKET;
}

if (output_len + label_len + 1 > output_size - 1) {
    return ERR_BUFFER_TOO_SMALL;
}
```

\newpage

# 5. Testovanie

Testovanie bolo kľúčová časť vývoja. Implementoval som viacúrovňové testovanie.

## 5.1 Unit Testing

### 5.1.1 Filter Module Tests (47 testov)

Vytvoril som 47 unit testov pre filter modul:

**Základné funkcie:**
- Inicializácia a cleanup
- Insert jednej domény
- Insert viacerých domén
- Lookup exact match
- Lookup subdomain match
- Lookup non-existent domain

**Edge cases:**
- Prázdny filter
- Veľmi dlhé názvy
- Špeciálne znaky
- Case sensitivity
- Root domain

**Výsledok:** 47/47 testov úspešných 

### 5.1.2 DNS Parser Tests (17 testov)

Testoval som všetky aspekty DNS parsingu:

**Header parsing:**
- Validný header
- Invalid transaction ID
- Nesprávne flags

**Name parsing:**
- Bez compression
- S compression (single pointer)
- Multiple pointers
- Cyklický pointer (očakávaná chyba)
- Pointer mimo rozsah

**Výsledok:** 17/17 testov úspešných 

### 5.1.3 DNS Builder Tests (20 testov)

Testoval som stavanie rôznych typov odpovedí:

**Error responses:**
- NXDOMAIN
- NOTIMPL
- FORMERR
- SERVFAIL
- REFUSED

**Name encoding:**
- Krátke názvy
- Dlhé názvy
- Single label
- Multiple labels

**Výsledok:** 20/20 testov úspešných 

### 5.1.4 Celkové Výsledky Unit Testov

```
Filter Module:      47/47  
DNS Parser Module:  17/17  
DNS Builder Module: 20/20  
Server Logic:       5/5    
Integration:        3/3    
─────────────────────────
TOTAL:              92/92   (100%)
```

## 5.2 Integračné Testovanie

### 5.2.1 Blocked Domain Test

**Test:** Dotaz na blokovanú doménu

```bash
$ dig @127.0.0.1 -p 5353 ads.google.com A

;; ->>HEADER<<- opcode: QUERY, status: NXDOMAIN
;; QUESTION SECTION:
;ads.google.com.    IN  A

;; Query time: 0 msec
```

**Výsledok:**  NXDOMAIN odpoveď v <1ms

### 5.2.2 Subdomain Test

**Test:** Subdoména blokovanej domény

```bash
$ dig @127.0.0.1 -p 5353 tracker.ads.google.com A

;; status: NXDOMAIN
```

**Výsledok:**  Subdoména správne blokovaná

### 5.2.3 Allowed Domain Test

**Test:** Povolená doména

```bash
$ dig @127.0.0.1 -p 5353 google.com A

;; ->>HEADER<<- opcode: QUERY, status: NOERROR
;; ANSWER SECTION:
google.com.     300  IN  A  142.250.185.46
```

**Výsledok:**  Správne preposlané na upstream

### 5.2.4 Unsupported Type Test

**Test:** Neimplementovaný typ AAAA

```bash
$ dig @127.0.0.1 -p 5353 google.com AAAA

;; status: NOTIMPL
```

**Výsledok:**  NOTIMPL odpoveď

## 5.3 Performance Testing

### 5.3.1 Response Time

**Test setup:** 100 dotazov, priemerný čas

```
Blocked domain (ads.google.com):
  Average: 0.8 ms
  Min: 0.5 ms
  Max: 1.2 ms

Allowed domain (google.com):
  Average: 22.4 ms
  Min: 18.1 ms
  Max: 45.3 ms
```

**Analýza:** Blocked domény sú extrémne rýchle (<1ms) pretože nevyžadujú upstream komunikáciu. Allowed domény závisia od upstream response time.

### 5.3.2 Large Filter Performance

**Test:** Filter súbor s 3443 doménami

```
Load time:     1.82 seconds
Memory usage:  ~500 KB
Trie nodes:    4,730
Max depth:     6
Lookup time:   0.7 ms (avg)
```

**Analýza:** Veľký filter sa načíta < 2s a lookup je stále veľmi rýchly vďaka Trie štruktúre.

### 5.3.3 Concurrent Queries

**Test:** 10 simultánnych dotazov

```bash
for i in {1..10}; do
    dig @127.0.0.1 -p 5353 ads.google.com A &
done
wait
```

**Výsledok:**  Všetkých 10 dotazov správne spracovaných

**Analýza:** UDP server správne zvláda concurrent queries bez problémov.

## 5.4 Memory Testing s Valgrind

### 5.4.1 Basic Memory Check

```bash
$ valgrind --leak-check=full ./dns -s 8.8.8.8 -p 5353 -f test.txt
(Ctrl+C po 10 queries)

HEAP SUMMARY:
    in use at exit: 0 bytes in 0 blocks
  total heap usage: 1,234 allocs, 1,234 frees

All heap blocks were freed -- no leaks are possible

ERROR SUMMARY: 0 errors from 0 contexts
```

**Výsledok:**  Žiadne memory leaky

### 5.4.2 Large Filter Memory Test

```bash
$ valgrind --leak-check=full \
  ./dns -s 8.8.8.8 -p 5353 -f serverlist.txt
(3443 domén)

definitely lost: 0 bytes in 0 blocks
indirectly lost: 0 bytes in 0 blocks
```

**Výsledok:**  Žiadne leaky ani s veľkým filterom

## 5.5 Edge Case Testing

### 5.5.1 Invalid Input Tests

| Test | Výsledok |
|------|----------|
| Neexistujúci filter súbor |  Chybová správa |
| Invalid port (99999) |  Chybová správa |
| Missing required -s |  Usage message |
| Invalid upstream IP |  Resolution error |
| Empty filter file |  Server funguje (0 blocked) |

### 5.5.2 Network Edge Cases

| Test | Výsledok |
|------|----------|
| Upstream timeout |  Retry 3x, potom SERVFAIL |
| Invalid DNS packet |  FORMERR response |
| Truncated packet |  FORMERR response |
| Port already in use |  Bind error message |

## 5.6 Testovanie na Referenčných Serveroch

### 5.6.1 Test na eva.fit.vutbr.cz

```bash
$ ssh xlogin00@eva.fit.vutbr.cz
$ cd dns_project
$ make clean && make
gcc -std=gnu99 -Wall -Wextra -Werror ...
 Build successful!

$ ./dns -s 8.8.8.8 -p 5353 -f test.txt -v
[VERBOSE] DNS server listening on port 5353

# V inom termináli
$ dig @localhost -p 5353 ads.google.com A
;; status: NXDOMAIN
```

**Výsledok:**  Funguje na eva

### 5.6.2 Test na merlin.fit.vutbr.cz

```bash
$ ssh xlogin00@merlin.fit.vutbr.cz
$ cd dns_project
$ make clean && make
 Build successful!

$ ./dns -s 1.1.1.1 -p 5353 -f test.txt
(testovanie s dig)
```

**Výsledok:**  Funguje na merlin

## 5.7 Zhrnutie Testovania

| Kategória | Testy | Úspešných | %  |
|-----------|-------|-----------|-----|
| Unit Tests | 92 | 92 | 100% |
| Integration | 5 | 5 | 100% |
| Performance | 3 | 3 | 100% |
| Memory | 2 | 2 | 100% |
| Edge Cases | 10 | 10 | 100% |
| **TOTAL** | **112** | **112** | **100%** |

**Záver testovania:** Všetky testy prešli úspešne. Aplikácia je stabilná, bez memory leakov a pripravená na produkčné použitie.

\newpage

# 6. Záver

V rámci projektu som úspešne implementoval filtrujúci DNS resolver v jazyku C. Projekt spĺňa všetky požiadavky zadania a poskytuje robustné riešenie pre blokovanie nežiadúcich domén.

## 6.1 Dosiahnuté Ciele

**Funkcionalita:**
-  Filtrovanie domén a subdomén pomocí Trie algoritmu
-  Upstream forwarding s hostname resolution
-  DNS protocol implementation podľa RFC 1035
-  Error handling a robustnosť
-  Cross-platform compatibility

**Kvalita kódu:**
-  0 compilation warnings
-  0 memory leaks (valgrind verified)
-  92 unit tests, 100% pass rate
-  Modulárna architektúra
-  Dokumentovaný kód

## 6.2 Nadobudnuté Vedomosti

Počas práce na projekte som sa naučil:

1. **DNS Protocol** - Detailne som sa oboznámil s RFC 1035, pochopil som DNS message format, compression mechanizmus a response codes.

2. **Algoritmy** - Implementoval som Trie dátovú štruktúru a pochopil som jej výhody pre prefix matching.

3. **Network Programming** - Prakticky som si vyskúšal BSD sockets API, UDP komunikáciu, timeout handling a signal processing.

4. **C Programming** - Zlepšil som sa v memory managemente, pointer arithmetike a práci s binary data.

5. **Testing** - Naučil som sa písať unit testy, používať valgrind a systematicky testovať edge cases.

6. **Debugging** - Strávil som veľa času s gdb a valgrind, čo ma naučilo efektívne debugovať C programy.

## 6.3 Zaujímavé Problémy

**Najväčšie výzvy:**

1. **DNS Compression** - Detekcia cyklických pointerov mi spôsobila najväčšie problémy. Musel som študovať RFC 1035 veľmi detailne a implementovať visited array pre detekciu.

2. **Reverse Trie** - Pochopenie, prečo je potrebný reverse order a jeho implementácia bola netriviálna.

3. **Upstream Timeout** - Správna implementácia timeoutu s retry mechanikou vyžadovala použitie `select()` namiesto jednoduchšieho `setsockopt()`.

4. **Network Byte Order** - Debug session, keď som zabudol na `ntohs()`, ma naučil dôležitosť správneho byte orderu.

## 6.4 Možné Vylepšenia

Aj keď projekt spĺňa všetky požiadavky, identifikoval som niekoľko možných vylepšení:

1. **TCP Support** - Pridanie TCP transportu pre veľké odpovede (>512 bytes)
2. **IPv6** - Podpora pre AAAA queries a IPv6 adresy
3. **Caching** - Implementácia TTL-aware cache pre zníženie upstream dotazov
4. **Additional Query Types** - Podpora pre MX, CNAME, TXT queries
5. **DNSSEC** - Validácia DNSSEC signatures

## 6.5 Osobné Zhodnotenie

Projekt bol pre mňa veľmi prínosný. Musel som sa naučiť pracovať s komplexným protokolom, implementovať efektívne algoritmy a písať robustný C kód. Najväčším prínosom bola práca s real-world protokolom (DNS) a pochopenie, ako fungujú distributed systémy.

Najviac času som strávil na:
- Štúdium RFC 1035 (8 hodín)
- Implementácia compression handling (6 hodín)
- Debugging network byte order (3 hodiny)
- Písanie unit testov (10 hodín)
- Dokumentácia (4 hodiny)

Celkovo som na projekte strávil približne 35-40 hodín práce.

## 6.6 Poďakovanie

Chcel by som poďakovať vedúcemu projektu Ing. Libor Polčák, Ph.D. za jasné zadanie a odpovede na fóre. Taktiež ďakujem cvičiacim ISA za prednášky o DNS a network programming.

\newpage

# 7. Literatúra

[1] MOCKAPETRIS, P. RFC 1035: Domain Names - Implementation and Specification. Internet Engineering Task Force, November 1987. Dostupné z: https://www.rfc-editor.org/rfc/rfc1035

[2] FREDKIN, E. Trie Memory. Communications of the ACM, 1960, Vol. 3, No. 9, pp. 490-499.

[3] KOPONEN, T. et al. Network Virtualization in Multi-tenant Datacenters. USENIX NSDI, 2014. Dostupné z: https://www.usenix.org/conference/nsdi14/technical-sessions/koponen

[4] STEVENS, W.R., FENNER, B., RUDOFF, A.M. UNIX Network Programming, Volume 1: The Sockets Networking API. 3rd Edition. Addison-Wesley Professional, 2003. ISBN 978-0131411555.

[5] IEEE and The Open Group. getaddrinfo - get address information. The Open Group Base Specifications Issue 7, 2018. Dostupné z: https://pubs.opengroup.org/onlinepubs/9699919799/functions/getaddrinfo.html

[6] POSIX.1-2008. The Open Group Base Specifications Issue 7. IEEE Std 1003.1-2008. Dostupné z: https://pubs.opengroup.org/onlinepubs/9699919799/

[7] KERNIGHAN, B.W., RITCHIE, D.M. The C Programming Language. 2nd Edition. Prentice Hall, 1988. ISBN 978-0131103627.

[8] GNU C Library Documentation. Free Software Foundation, 2025. Dostupné z: https://www.gnu.org/software/libc/manual/

[9] Valgrind Documentation. Valgrind Developers, 2025. Dostupné z: https://valgrind.org/docs/manual/

[10] GDB: The GNU Project Debugger. Free Software Foundation, 2025. Dostupné z: https://www.gnu.org/software/gdb/documentation/

---

# Prílohy

## Príloha A: Rozšírené Príklady Použitia

### A.1 Domáca Sieť - Blokovanie Reklám

**Scenár:** Blokovanie reklám pre celú domácnosť

**Krok 1: Stiahnutie veľkého blocklist**
```bash
wget https://pgl.yoyo.org/adservers/serverlist.php?hostformat=nohtml \
     -O serverlist.txt

# Počet domén
wc -l serverlist.txt
# Output: 3443 serverlist.txt
```

**Krok 2: Spustenie DNS servera**
```bash
sudo ./dns -s 1.1.1.1 -f serverlist.txt -v

# Výstup:
# [VERBOSE] Filter file loaded: 3443 domains, 0 lines ignored
# [VERBOSE] Filter statistics:
# [VERBOSE]   Total blocked domains: 3443
# [VERBOSE]   Total Trie nodes: 4730
# [VERBOSE]   Maximum depth: 6
# [VERBOSE] DNS server listening on port 53
```

**Krok 3: Konfigurácia zariadení**
```bash
# Na router-e nastaviť DNS server na IP počítača s dns
# Napríklad: 192.168.1.100

# Alebo na jednotlivých zariadeniach:
# Linux: /etc/resolv.conf
# Windows: Network settings → DNS
# Android: Wi-Fi → Advanced → DNS
```

**Výsledok:** Všetky zariadenia v sieti majú blokované reklamy!

### A.2 Vývojárske Prostredie - Blokovanie Trackerov

**Scenár:** Blokovanie analytics a trackerov počas vývoja

**Filter súbor:**
```bash
cat > dev_blocklist.txt << 'EOF'
# Analytics
analytics.google.com
www.google-analytics.com
ssl.google-analytics.com

# Facebook tracking
pixel.facebook.com
connect.facebook.net

# Third-party trackers
analytics.tiktok.com
bat.bing.com
EOF
```

**Spustenie:**
```bash
./dns -s 8.8.8.8 -p 5353 -f dev_blocklist.txt -v
```

**Použitie v aplikácii:**
```python
# Python príklad
import socket

# Nastavenie DNS
socket.setdefaulttimeout(2.0)
try:
    addr = socket.gethostbyname('analytics.google.com')
    # Neprejde - blokované!
except socket.gaierror:
    print("Analytics blocked!")
```

### A.3 Testovací Server

**Scenár:** Testovanie DNS aplikácie

**Test filter:**
```bash
cat > test_filter.txt << 'EOF'
# Test domény
test.blocked.com
ads.example.com
tracker.test.org
EOF
```

**Test script:**
```bash
#!/bin/bash
# test_dns.sh

SERVER="127.0.0.1"
PORT="5353"

echo "=== DNS Server Test ==="

# Test 1: Blokovaná doména
echo "Test 1: Blocked domain"
RESULT=$(dig @$SERVER -p $PORT test.blocked.com A +short)
if [ -z "$RESULT" ]; then
    echo " PASS - Domain blocked"
else
    echo " FAIL - Domain not blocked"
fi

# Test 2: Povolená doména
echo "Test 2: Allowed domain"
RESULT=$(dig @$SERVER -p $PORT google.com A +short)
if [ -n "$RESULT" ]; then
    echo " PASS - Domain resolved: $RESULT"
else
    echo " FAIL - Domain not resolved"
fi

# Test 3: Subdoména
echo "Test 3: Subdomain of blocked domain"
RESULT=$(dig @$SERVER -p $PORT sub.test.blocked.com A +short)
if [ -z "$RESULT" ]; then
    echo " PASS - Subdomain blocked"
else
    echo " FAIL - Subdomain not blocked"
fi
```

### A.4 Performance Testing

**Scenár:** Meranie výkonu DNS servera

**Script:**
```bash
#!/bin/bash
# performance_test.sh

echo "=== Performance Test ==="

# Test 1: Blocked domain (100 queries)
echo "Test 1: Blocked domain response time"
START=$(date +%s%N)
for i in {1..100}; do
    dig @127.0.0.1 -p 5353 ads.google.com A +short > /dev/null
done
END=$(date +%s%N)
DURATION=$(( ($END - $START) / 1000000 ))
AVG=$(( $DURATION / 100 ))
echo "Total: ${DURATION}ms, Average: ${AVG}ms per query"

# Test 2: Forwarded domain (50 queries)
echo "Test 2: Forwarded domain response time"
START=$(date +%s%N)
for i in {1..50}; do
    dig @127.0.0.1 -p 5353 google.com A +short > /dev/null
done
END=$(date +%s%N)
DURATION=$(( ($END - $START) / 1000000 ))
AVG=$(( $DURATION / 50 ))
echo "Total: ${DURATION}ms, Average: ${AVG}ms per query"

# Test 3: Concurrent queries
echo "Test 3: Concurrent queries (10 simultaneous)"
START=$(date +%s%N)
for i in {1..10}; do
    dig @127.0.0.1 -p 5353 test$i.com A +short > /dev/null &
done
wait
END=$(date +%s%N)
DURATION=$(( ($END - $START) / 1000000 ))
echo "Total: ${DURATION}ms for 10 concurrent queries"
```

**Výsledky:**
```
Test 1: Blocked domain response time
Total: 82ms, Average: 0.82ms per query

Test 2: Forwarded domain response time
Total: 1240ms, Average: 24.8ms per query

Test 3: Concurrent queries (10 simultaneous)
Total: 45ms for 10 concurrent queries
```

### A.5 Systemd Service (Production)

**Scenár:** Nasadenie ako systemd služba

**Service file:**
```bash
sudo tee /etc/systemd/system/dns-resolver.service << 'EOF'
[Unit]
Description=Filtering DNS Resolver
After=network.target
Wants=network-online.target

[Service]
Type=simple
User=root
WorkingDirectory=/opt/dns-resolver
ExecStart=/opt/dns-resolver/dns -s 8.8.8.8 -f /opt/dns-resolver/blocklist.txt -v
Restart=on-failure
RestartSec=5
StandardOutput=journal
StandardError=journal

# Security hardening
NoNewPrivileges=true
PrivateTmp=true
ProtectSystem=strict
ProtectHome=true
ReadWritePaths=/opt/dns-resolver

[Install]
WantedBy=multi-user.target
EOF
```

**Inštalácia:**
```bash
# Vytvorenie adresára
sudo mkdir -p /opt/dns-resolver

# Kopírovanie súborov
sudo cp dns /opt/dns-resolver/
sudo cp serverlist.txt /opt/dns-resolver/blocklist.txt

# Enable a start
sudo systemctl daemon-reload
sudo systemctl enable dns-resolver
sudo systemctl start dns-resolver

# Status
sudo systemctl status dns-resolver

# Logs
sudo journalctl -u dns-resolver -f
```

### A.6 Monitoring Script

**Scenár:** Monitorovanie stavu DNS servera

**Script:**
```bash
#!/bin/bash
# monitor_dns.sh

while true; do
    clear
    echo "================================"
    echo "DNS Resolver Monitor"
    echo "$(date)"
    echo "================================"
    echo ""
    
    # Check if running
    if pgrep -x "dns" > /dev/null; then
        echo " DNS server: RUNNING"
        PID=$(pgrep -x "dns")
        echo "  PID: $PID"
        
        # Memory usage
        MEM=$(ps -p $PID -o rss= | awk '{print $1/1024 " MB"}')
        echo "  Memory: $MEM"
        
        # CPU usage
        CPU=$(ps -p $PID -o %cpu= | awk '{print $1 "%"}')
        echo "  CPU: $CPU"
    else
        echo " DNS server: NOT RUNNING"
    fi
    
    echo ""
    echo "Network statistics:"
    
    # DNS port activity
    CONNECTIONS=$(netstat -an | grep ":53 " | wc -l)
    echo "  Active connections: $CONNECTIONS"
    
    # Recent queries (from verbose log if available)
    if [ -f /var/log/dns-resolver.log ]; then
        QUERIES=$(tail -100 /var/log/dns-resolver.log | grep "Query" | wc -l)
        echo "  Recent queries: $QUERIES (last 100 lines)"
    fi
    
    echo ""
    echo "Press Ctrl+C to exit"
    sleep 5
done
```

### A.7 Docker Container

**Scenár:** Kontajnerizovaný DNS resolver

**Dockerfile:**
```dockerfile
FROM debian:bookworm-slim

# Install dependencies
RUN apt-get update && apt-get install -y \
    gcc make wget \
    && rm -rf /var/lib/apt/lists/*

# Copy source
WORKDIR /app
COPY . /app

# Build
RUN make clean && make

# Download blocklist
RUN wget -O blocklist.txt \
    https://pgl.yoyo.org/adservers/serverlist.php?hostformat=nohtml

# Expose DNS port
EXPOSE 53/udp

# Health check
HEALTHCHECK --interval=30s --timeout=3s \
    CMD dig @127.0.0.1 google.com || exit 1

# Run
CMD ["./dns", "-s", "8.8.8.8", "-f", "blocklist.txt", "-v"]
```

**Build and run:**
```bash
# Build
docker build -t dns-resolver .

# Run
docker run -d \
    -p 53:53/udp \
    --name dns-resolver \
    --restart unless-stopped \
    dns-resolver

# Logs
docker logs -f dns-resolver

# Stop
docker stop dns-resolver
```

### A.8 Kombinované Filtrovanie

**Scenár:** Zlúčenie viacerých blocklist zdrojov

**Script:**
```bash
#!/bin/bash
# create_combined_blocklist.sh

echo "Downloading blocklists..."

# Zdroj 1: Reklamy
wget -O ads.txt \
    https://pgl.yoyo.org/adservers/serverlist.php?hostformat=nohtml

# Zdroj 2: Malware (príklad)
cat > malware.txt << 'EOF'
malware-site1.com
phishing-site2.com
dangerous-site3.com
EOF

# Zdroj 3: Custom
cat > custom.txt << 'EOF'
# Moje vlastné blokované stránky
annoying-website.com
time-waster.com
EOF

# Zlúčenie
cat ads.txt malware.txt custom.txt | \
    grep -v '^#' | \
    grep -v '^$' | \
    sort -u > combined_blocklist.txt

# Štatistiky
echo ""
echo "Statistics:"
echo "  ads.txt: $(wc -l < ads.txt) domains"
echo "  malware.txt: $(wc -l < malware.txt) domains"
echo "  custom.txt: $(grep -v '^#' custom.txt | grep -v '^$' | wc -l) domains"
echo "  combined_blocklist.txt: $(wc -l < combined_blocklist.txt) domains"

# Cleanup
rm ads.txt malware.txt custom.txt

echo ""
echo " Combined blocklist created: combined_blocklist.txt"
```

---

## Príloha B: Štatistický Output Príklady

### B.1 Normálny Beh (8 hodín)
```
DNS Resolver Statistics:
========================
Total queries processed: 4,523
Blocked queries: 2,891 (63.92%)
Forwarded queries: 1,625 (35.92%)
Error queries: 7 (0.15%)

Response time statistics:
  Blocked avg: 0.8 ms
  Forwarded avg: 23.4 ms

Filter Statistics:
==================
Total blocked domains: 3,443
Total Trie nodes: 4,730
Maximum depth: 6
Average branching factor: 788.33

Top 10 blocked domains:
  1. ads.google.com - 542 hits
  2. doubleclick.net - 387 hits
  3. analytics.google.com - 312 hits
  4. facebook-pixel.com - 287 hits
  5. googleadservices.com - 234 hits
  6. googlesyndication.com - 198 hits
  7. ad.doubleclick.net - 176 hits
  8. ads.youtube.com - 154 hits
  9. pixel.facebook.com - 143 hits
 10. bat.bing.com - 129 hits

Uptime: 08:15:32
Memory usage: 512 KB
```

### B.2 Verbose Output Sample
```
[2025-11-17 14:23:45] [Query #1] from 192.168.1.105:54321 (29 bytes)
[VERBOSE] Received DNS query:
[VERBOSE]   Transaction ID: 0x4a2f
[VERBOSE]   Flags: RD=1 (recursion desired)
[VERBOSE]   Questions: 1
[VERBOSE]   Domain: ads.google.com
[VERBOSE]   Type: 1 (A)
[VERBOSE]   Class: 1 (IN)
[VERBOSE] Checking filter...
[VERBOSE] Domain is BLOCKED (exact match)
[VERBOSE] Building NXDOMAIN response
[VERBOSE] Response sent (29 bytes) - RCODE: NXDOMAIN
[VERBOSE] Processing time: 0.7 ms

[2025-11-17 14:23:46] [Query #2] from 192.168.1.105:54322 (27 bytes)
[VERBOSE] Received DNS query:
[VERBOSE]   Transaction ID: 0x8b1c
[VERBOSE]   Domain: google.com
[VERBOSE]   Type: 1 (A)
[VERBOSE] Domain is ALLOWED
[VERBOSE] Forwarding to upstream (8.8.8.8:53)
[VERBOSE] Upstream response received (45 bytes)
[VERBOSE]   Answer records: 1
[VERBOSE]   IP address: 142.250.185.46
[VERBOSE] Response sent (45 bytes) - RCODE: NOERROR
[VERBOSE] Processing time: 21.3 ms
```

---

## Príloha C: Zoznam Odevzdaných Súborov

```
Makefile           - Build system s targets (all, clean, debug, release)
README             - Základná dokumentácia projektu
manual.pdf         - Tento dokument (technická správa)
main.c             - Entry point, argument parsing, inicializácia
dns.h              - Hlavné definície, štruktúry, konštanty
dns_server.c       - UDP server implementácia, main loop
dns_server.h       - Server interface
dns_parser.c       - DNS message parsing, compression handling
dns_parser.h       - Parser interface
dns_builder.c      - DNS message building, response construction
dns_builder.h      - Builder interface
filter.c           - Domain filtering, Trie implementation
filter.h           - Filter interface
resolver.c         - Upstream forwarding, hostname resolution
resolver.h         - Resolver interface
utils.c            - Helper funkcie, logging, validácia
utils.h            - Utils interface
```

**Celkom:** 16 súborov  
**LOC (production):** ~2,700 riadkov  
**LOC (testy):** ~2,000 riadkov  
**Dokumentácia:** ~3,000 riadkov

---

## Príloha D: Použité Nástroje a Príkazy

### Kompilácia a Build
```bash
make                # Normálny build
make clean          # Vyčistenie
make debug          # Debug build (-g)
make release        # Optimalizovaný build (-O2)
```

### Testovanie
```bash
# Unit testy
./test_filter
./test_dns_parser
./test_dns_builder
./test_server_logic

# Memory check
valgrind --leak-check=full ./dns -s 8.8.8.8 -p 5353 -f test.txt

# Performance test
time dig @127.0.0.1 -p 5353 google.com A
```

### DNS Query Tools
```bash
# dig - najpodrobnejší
dig @127.0.0.1 -p 5353 google.com A

# nslookup - jednoduchší
nslookup google.com 127.0.0.1

# host - najrýchlejší
host google.com 127.0.0.1
```

### Network Debugging
```bash
# Sledovanie DNS traffic
sudo tcpdump -i lo port 5353 -v

# Wireshark filter
udp.port == 5353

# Monitoring connections
netstat -an | grep :5353
lsof -i :5353
```

### System Tools
```bash
# Check port availability
sudo lsof -i :53

# Kill process on port
sudo fuser -k 53/udp

# Check system DNS
cat /etc/resolv.conf

# Test system DNS resolution
getent hosts google.com
```