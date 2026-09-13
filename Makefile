ifeq ($(origin CC), default)
CC := m68k-amigaos-gcc
endif
CFLAGS ?= -Os -Wall -Wextra -Werror -m68000 -Iinclude
LDFLAGS ?= -mcrt=nix20
TARGET := AmBNC
SOURCES := src/main.c src/net.c src/irc.c src/downstream.c src/state.c src/rexx.c src/rexx_events.c src/networks.c src/multinet.c src/upstream.c
OBJECTS := $(SOURCES:.c=.o)
HEADERS := include/ambnc.h include/net.h include/irc.h include/downstream.h include/state.h include/rexx.h include/rexx_events.h include/networks.h include/multinet.h include/upstream.h

.PHONY: all clean check

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

src/%.o: src/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

check:
	@grep -q 'AMBNC_REXX_PORT "AMBNC"' include/ambnc.h
	@grep -q 'AMBNC_VERSION "0.6.0-m6"' include/ambnc.h
	@grep -q -- '-m68000' Makefile
	@grep -q -- '-mcrt=nix20' Makefile
	@grep -q 'AMBNC_NETWORKS_MAX 4' include/networks.h
	@grep -q '\[NETWORK ' src/networks.c
	@grep -q 'ambnc_multinet_run' src/main.c
	@grep -q 'ambnc_net_wait_many_timeout' src/multinet.c
	@grep -q 'SOCKET_UPSTREAM' src/multinet.c
	@grep -q 'SOCKET_LISTENER' src/multinet.c
	@grep -q 'SOCKET_DOWNSTREAM' src/multinet.c
	@grep -q 'reconnect_backoff' src/multinet.c
	@echo "M6 static checks: PASS"

clean:
	rm -f $(OBJECTS) $(TARGET)
