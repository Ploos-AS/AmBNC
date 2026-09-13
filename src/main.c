#include <stdio.h>
#include <stdlib.h>

#include "ambnc.h"
#include "upstream.h"

static void usage(const char *program)
{
    printf("Usage: %s HOST PORT NICK [USER] [PASS]\n", program);
    puts("Example: AmBNC irc.libera.chat 6667 AmBNC ambnc secret");
}

int ambnc_run(int argc, char **argv)
{
    struct ambnc_upstream_config config;
    long port;

    puts(AMBNC_NAME " " AMBNC_VERSION);
    puts("ARexx port reserved: " AMBNC_REXX_PORT);

    if (argc < 4 || argc > 6) {
        usage(argv[0]);
        return 10;
    }

    port = strtol(argv[2], 0, 10);
    if (port <= 0 || port > 65535) {
        puts("AmBNC: invalid TCP port");
        return 10;
    }

    config.host = argv[1];
    config.port = (unsigned short)port;
    config.nick = argv[3];
    config.user = argc >= 5 ? argv[4] : argv[3];
    config.pass = argc >= 6 ? argv[5] : 0;

    return ambnc_upstream_run(&config);
}

int main(int argc, char **argv)
{
    return ambnc_run(argc, argv);
}
