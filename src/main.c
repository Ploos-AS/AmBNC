#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ambnc.h"
#include "multinet.h"
#include "networks.h"
#include "state.h"
#include "upstream.h"

#define AMBNC_DEFAULT_LISTEN_PORT 16667

static void usage(const char *program)
{
    printf("Usage: %s HOST PORT NICK [USER] [PASS] [LISTEN_PORT] [BACKLOG_LINES]\n", program);
    printf("       %s -c CONFIG\n", program);
    puts("Example: AmBNC irc.libera.chat 6667 AmBNC ambnc secret 16667 32");
    puts("Example: AmBNC -c S:AmBNC.cfg");
}

static int run_config(const char *path)
{
    struct ambnc_networks_config networks;
    char error[128];

    if (ambnc_networks_load(&networks, path, error, sizeof(error)) != 0) {
        printf("AmBNC: config error: %s\n", error);
        return 10;
    }
    printf("AmBNC: loaded %u network(s) from %s\n", networks.count, path);
    return ambnc_multinet_run(&networks);
}

int ambnc_run(int argc, char **argv)
{
    struct ambnc_upstream_config config;
    long port;
    long listen_port = AMBNC_DEFAULT_LISTEN_PORT;
    long backlog_lines = AMBNC_STATE_RING_LINES_DEFAULT;

    puts(AMBNC_NAME " " AMBNC_VERSION);
    puts("ARexx port reserved: " AMBNC_REXX_PORT);

    if (argc == 3 && strcmp(argv[1], "-c") == 0)
        return run_config(argv[2]);

    if (argc < 4 || argc > 8) {
        usage(argv[0]);
        return 10;
    }

    port = strtol(argv[2], 0, 10);
    if (port <= 0 || port > 65535) {
        puts("AmBNC: invalid upstream TCP port");
        return 10;
    }
    if (argc >= 7) {
        listen_port = strtol(argv[6], 0, 10);
        if (listen_port <= 0 || listen_port > 65535) {
            puts("AmBNC: invalid downstream TCP port");
            return 10;
        }
    }
    if (argc >= 8) {
        backlog_lines = strtol(argv[7], 0, 10);
        if (backlog_lines <= 0 || backlog_lines > AMBNC_STATE_RING_LINES_MAX) {
            printf("AmBNC: backlog lines must be 1-%u\n", AMBNC_STATE_RING_LINES_MAX);
            return 10;
        }
    }

    config.host = argv[1];
    config.port = (unsigned short)port;
    config.nick = argv[3];
    config.user = argc >= 5 ? argv[4] : argv[3];
    config.pass = argc >= 6 ? argv[5] : 0;
    config.listen_port = (unsigned short)listen_port;
    config.backlog_lines = (unsigned int)backlog_lines;

    return ambnc_upstream_run(&config);
}

int main(int argc, char **argv)
{
    return ambnc_run(argc, argv);
}
