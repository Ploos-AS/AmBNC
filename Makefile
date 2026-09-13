ifeq ($(origin CC), default)
CC := m68k-amigaos-gcc
endif
CFLAGS ?= -Os -Wall -Wextra -Werror -m68000 -Iinclude
LDFLAGS ?= -mcrt=nix20
TARGET := AmBNC
SOURCES := src/main.c src/net.c src/irc.c src/upstream.c
OBJECTS := $(SOURCES:.c=.o)
HEADERS := include/ambnc.h include/net.h include/irc.h include/upstream.h

.PHONY: all clean check

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

src/%.o: src/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

check:
	@grep -q 'AMBNC_REXX_PORT "AMBNC"' include/ambnc.h
	@grep -q 'AMBNC_VERSION "0.1.0-m1"' include/ambnc.h
	@grep -q -- '-m68000' Makefile
	@grep -q -- '-mcrt=nix20' Makefile
	@grep -q 'bsdsocket.library' src/net.c
	@grep -q 'PING ' src/upstream.c
	@grep -q 'PONG ' src/upstream.c
	@grep -q 'backoff_seconds' src/upstream.c
	@grep -q 'AMBNC_IRC_LINE_MAX 510' include/irc.h
	@echo "M1 static checks: PASS"

clean:
	rm -f $(OBJECTS) $(TARGET)
