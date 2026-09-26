ifeq ($(origin CC), default)
CC := m68k-amigaos-gcc
endif
CFLAGS ?= -Os -Wall -Wextra -Werror -m68000 -Iinclude
LDFLAGS ?= -mcrt=nix20
TARGET := AmBNC
SOURCES := src/main.c src/net.c src/irc.c src/downstream.c src/state.c src/rexx.c src/rexx_events.c src/networks.c src/multinet.c src/upstream.c src/pbmp.c
OBJECTS := $(SOURCES:.c=.o)
HEADERS := include/ambnc.h include/net.h include/irc.h include/downstream.h include/state.h include/rexx.h include/rexx_events.h include/networks.h include/multinet.h include/upstream.h include/modernirc.h include/pbmp.h
AMIGA_OUT ?= build/fs-uae/native
M8_2_OUT ?= build/m8_2
M8_2_HOST ?= HOST_IP_HERE
M8_2_SYSTEM_DIR ?= SYSTEM_DIR_HERE
M8_2_KICKSTART_FILE ?= KICKSTART_FILE_HERE

.PHONY: all clean check check-pbmp amiga qualify-m8_1 qualify-m8_2 review-m8_2-evidence

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS)

src/%.o: src/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

check:
	@grep -q 'AMBNC_REXX_PORT "AMBNC"' include/ambnc.h
	@grep -q 'AMBNC_VERSION "0.7.0-m7"' include/ambnc.h
	@grep -q -- '-m68000' Makefile
	@grep -q -- '-mcrt=nix20' Makefile
	@grep -q 'AMBNC_NETWORKS_MAX 4' include/networks.h
	@grep -q 'NETWORK ' src/networks.c
	@grep -q 'ambnc_multinet_run' src/main.c
	@grep -q 'ambnc_net_wait_many_timeout' src/multinet.c
	@grep -q 'cap_enabled' include/networks.h
	@grep -q 'sasl_plain' include/networks.h
	@grep -q 'AMBNC_TLS_PROXY' include/networks.h
	@grep -q 'ambnc_m7_send_registration' src/networks.c
	@grep -q 'AMBNC_PBMP_ENDPOINT_KIND "bouncer"' include/pbmp.h
	@grep -q '"endpoint.info"' src/pbmp.c
	@bash ci/m8_2/check-harness.sh
	@echo "M7 static checks: PASS"

check-pbmp:
	@mkdir -p build/tests
	@cc -std=c89 -Wall -Wextra -Werror -Iinclude tests/pbmp_adapter_test.c src/pbmp.c -o build/tests/pbmp_adapter_test
	@build/tests/pbmp_adapter_test

amiga:
	@bash ci/fs-uae/build-native.sh "$(AMIGA_OUT)"

qualify-m8_1: check amiga
	@bash ci/fs-uae/run-aros-guest-smoke.sh

qualify-m8_2: check amiga
	@M8_2_HOST='$(M8_2_HOST)' \
	 M8_2_SYSTEM_DIR='$(M8_2_SYSTEM_DIR)' \
	 M8_2_KICKSTART_FILE='$(M8_2_KICKSTART_FILE)' \
	 bash ci/m8_2/prepare.sh "$(M8_2_OUT)" "$(AMIGA_OUT)/AmBNC"

review-m8_2-evidence:
	@bash ci/m8_2/summarize-evidence.sh "$(M8_2_OUT)"

clean:
	rm -f $(OBJECTS) $(TARGET)
