ifeq ($(origin CC), default)
CC := m68k-amigaos-gcc
endif
CFLAGS ?= -Os -Wall -Wextra -Werror -m68000 -Iinclude
LDFLAGS ?= -mcrt=nix20
TARGET := AmBNC
SOURCES := src/main.c src/net.c src/irc.c src/downstream.c src/state.c src/upstream.c
OBJECTS := $(SOURCES:.c=.o)
HEADERS := include/ambnc.h include/net.h include/irc.h include/downstream.h include/state.h include/upstream.h

.PHONY: all clean check

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

src/%.o: src/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

check:
	@grep -q 'AMBNC_REXX_PORT "AMBNC"' include/ambnc.h
	@grep -q 'AMBNC_VERSION "0.3.0-m3"' include/ambnc.h
	@grep -q -- '-m68000' Makefile
	@grep -q -- '-mcrt=nix20' Makefile
	@grep -q 'AMBNC_STATE_CHANNELS_MAX 16' include/state.h
	@grep -q 'AMBNC_STATE_RING_LINES 32' include/state.h
	@grep -q 'ambnc_state_observe_line' src/upstream.c
	@grep -q 'PRIVMSG' src/state.c
	@grep -q 'NOTICE' src/state.c
	@echo "M3 static checks: PASS"

clean:
	rm -f $(OBJECTS) $(TARGET)
