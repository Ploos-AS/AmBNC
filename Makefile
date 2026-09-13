CC ?= m68k-amigaos-gcc
CFLAGS ?= -Os -Wall -Wextra -Werror -m68000 -Iinclude
LDFLAGS ?=
TARGET := AmBNC
SOURCES := src/main.c
OBJECTS := $(SOURCES:.c=.o)

.PHONY: all clean check

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

src/%.o: src/%.c include/ambnc.h
	$(CC) $(CFLAGS) -c $< -o $@

check:
	@grep -q 'AMBNC_REXX_PORT "AMBNC"' include/ambnc.h
	@grep -q -- '-m68000' Makefile
	@grep -q 'AmigaOS 2.04+' README.md
	@echo "M0 static checks: PASS"

clean:
	rm -f $(OBJECTS) $(TARGET)
