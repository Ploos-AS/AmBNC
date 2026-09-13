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
	@grep -q 'AMBNC_VERSION "0.4.0-m4"' include/ambnc.h
	@grep -q -- '-m68000' Makefile
	@grep -q -- '-mcrt=nix20' Makefile
	@grep -q 'AMBNC_STATE_RING_LINES_MAX 32' include/state.h
	@grep -q 'ring_lines_limit' include/state.h
	@grep -q 'DateStamp' src/state.c
	@grep -q 'ambnc_state_replay' src/state.c
	@grep -q 'Backlog target=' src/state.c
	@grep -q 'BACKLOG_LINES' src/main.c
	@grep -q 'ambnc_state_replay' src/upstream.c
	@echo "M4 static checks: PASS"

clean:
	rm -f $(OBJECTS) $(TARGET)
