#include <stdio.h>
#include <string.h>

#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "irc.h"
#include "net.h"
#include "upstream.h"

#define AMBNC_UPSTREAM_RECV_BUFFER 512

struct upstream_line_context {
    int sock;
};

static int stop_requested(void)
{
    return (SetSignal(0, 0) & SIGBREAKF_CTRL_C) != 0;
}

static void handle_line(const char *line, void *userdata)
{
    struct upstream_line_context *context = (struct upstream_line_context *)userdata;

    printf("< %s\n", line);
    if (strncmp(line, "PING ", 5) == 0) {
        char response[AMBNC_IRC_LINE_MAX + 1];
        int written = snprintf(response, sizeof(response), "PONG %s", line + 5);
        if (written > 0 && written < (int)sizeof(response)) {
            printf("> %s\n", response);
            (void)ambnc_irc_send_line(context->sock, response);
        }
    }
}

static int run_connected(const struct ambnc_upstream_config *config, int sock)
{
    struct ambnc_irc_framer framer;
    struct upstream_line_context line_context;
    char buffer[AMBNC_UPSTREAM_RECV_BUFFER];

    ambnc_irc_framer_init(&framer);
    line_context.sock = sock;

    if (ambnc_irc_send_registration(sock, config->nick, config->user, config->pass) != 0)
        return -1;

    while (!stop_requested()) {
        int received = ambnc_net_recv(sock, buffer, sizeof(buffer));
        if (received <= 0) return -1;
        ambnc_irc_framer_feed(&framer,
                              buffer,
                              (unsigned int)received,
                              handle_line,
                              &line_context);
    }
    return 0;
}

int ambnc_upstream_run(const struct ambnc_upstream_config *config)
{
    static const unsigned int backoff_seconds[] = { 1, 2, 4, 8, 16, 30 };
    unsigned int backoff_index = 0;

    if (config == 0 || config->host == 0 || config->nick == 0 || config->user == 0)
        return 10;

    if (ambnc_net_open() != 0) {
        puts("AmBNC: cannot open bsdsocket.library");
        return 20;
    }

    while (!stop_requested()) {
        int sock;

        printf("AmBNC: connecting upstream %s:%u\n",
               config->host, (unsigned int)config->port);
        sock = ambnc_net_connect_ipv4(config->host, config->port);
        if (sock >= 0) {
            puts("AmBNC: upstream connected");
            backoff_index = 0;
            (void)run_connected(config, sock);
            ambnc_net_close_socket(sock);
            if (stop_requested()) break;
            puts("AmBNC: upstream disconnected");
        } else {
            puts("AmBNC: upstream connect failed");
        }

        if (!stop_requested()) {
            unsigned int delay = backoff_seconds[backoff_index];
            printf("AmBNC: reconnect in %u second(s)\n", delay);
            Delay(delay * 50U);
            if (backoff_index + 1U < sizeof(backoff_seconds) / sizeof(backoff_seconds[0]))
                ++backoff_index;
        }
    }

    ambnc_net_close();
    puts("AmBNC: stopped");
    return 0;
}
