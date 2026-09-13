ifeq ($(origin CC), default)
CC := m68k-amigaos-gcc
endif
CFLAGS ?= -Os -Wall -Wextra -Werror -m68000 -Iinclude
LDFLAGS ?= -mcrt=nix20
TARGET := AmBNC
SOURCES := src/main.c src/net.c src/irc.c src/downstream.c src/upstream.c
OBJECTS := $(SOURCES:.c=.o)
HEADERS := include/ambnc.h include/net.h include/irc.h include/downstream.h include/upstream.h

.PHONY: all clean check

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

src/%.o: src/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

check:
	@grep -q 'AMBNC_REXX_PORT "AMBNC"' include/ambnc.h
	@grep -q 'AMBNC_VERSION "0.2.0-m2"' include/ambnc.h
	@grep -q -- '-m68000' Makefile
	@grep -q -- '-mcrt=nix20' Makefile
	@grep -q 'ambnc_net_listen_ipv4' src/net.c
	@grep -q 'WaitSelect' src/net.c
	@grep -q '001 %s :Attached to AmBNC' src/downstream.c
	@grep -q 'ambnc_downstream_accept' src/upstream.c
	@grep -q 'command_is(line, "QUIT")' src/upstream.c
	@echo "M2 static checks: PASS"

clean:
	rm -f $(OBJECTS) $(TARGET)
