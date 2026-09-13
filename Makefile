ifeq ($(origin CC), default)
CC := m68k-amigaos-gcc
endif
CFLAGS ?= -Os -Wall -Wextra -Werror -m68000 -Iinclude
LDFLAGS ?= -mcrt=nix20
TARGET := AmBNC
SOURCES := src/main.c src/net.c src/irc.c src/downstream.c src/state.c src/rexx.c src/rexx_events.c src/upstream.c
OBJECTS := $(SOURCES:.c=.o)
HEADERS := include/ambnc.h include/net.h include/irc.h include/downstream.h include/state.h include/rexx.h include/rexx_events.h include/upstream.h

.PHONY: all clean check

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

src/%.o: src/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

check:
	@grep -q 'AMBNC_REXX_PORT "AMBNC"' include/ambnc.h
	@grep -q 'AMBNC_VERSION "0.5.0-m5"' include/ambnc.h
	@grep -q -- '-m68000' Makefile
	@grep -q -- '-mcrt=nix20' Makefile
	@grep -q 'rexxsyslib.library' src/rexx.c
	@grep -q 'STATUS' src/rexx.c
	@grep -q 'CONNECT' src/rexx.c
	@grep -q 'DISCONNECT' src/rexx.c
	@grep -q 'BUFFER' src/rexx.c
	@grep -q 'RAW' src/rexx.c
	@grep -q 'ambnc_rexx_signal_mask' src/upstream.c
	@grep -q 'ON_CONNECT' src/upstream.c
	@grep -q 'ON_PRIVMSG' src/rexx_events.c
	@grep -q 'REXX:AmBNC' src/rexx_events.c
	@echo "M5 static checks: PASS"

clean:
	rm -f $(OBJECTS) $(TARGET)
