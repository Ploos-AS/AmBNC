#include <stdio.h>
#include <string.h>

#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "downstream.h"
#include "irc.h"
#include "net.h"
#include "state.h"
#include "upstream.h"

#define AMBNC_RECV_BUFFER 512

struct upstream_line_context {
    int sock;
    struct ambnc_downstream *downstream;
    struct ambnc_session_state *state;
};

struct downstream_line_context {
    int upstream_sock;
    struct ambnc_downstream *downstream;
};

static int stop_requested(void)
{
    return (SetSignal(0, 0) & SIGBREAKF_CTRL_C) != 0;
}

static int command_is(const char *line, const char *command)
{
    unsigned int i = 0;
    while (command[i] != '\0' && line[i] == command[i]) ++i;
    return command[i] == '\0' && (line[i] == '\0' || line[i] == ' ');
}

static int replay_send_line(void *userdata, const char *line)
{
    return ambnc_downstream_send_line((struct ambnc_downstream *)userdata, line);
}

static void handle_upstream_line(const char *line, void *userdata)
{
    struct upstream_line_context *context = (struct upstream_line_context *)userdata;
    int attached = context->downstream->client >= 0;

    printf("U< %s\n", line);
    ambnc_state_observe_line(context->state, line, attached);

    if (strncmp(line, "PING ", 5) == 0) {
        char response[AMBNC_IRC_LINE_MAX + 1];
        int written = snprintf(response, sizeof(response), "PONG %s", line + 5);
        if (written > 0 && written < (int)sizeof(response)) {
            printf("U> %s\n", response);
            (void)ambnc_irc_send_line(context->sock, response);
        }
        return;
    }

    if (attached && ambnc_downstream_send_line(context->downstream, line) != 0)
        ambnc_downstream_close_client(context->downstream);
}

static void handle_downstream_line(const char *line, void *userdata)
{
    struct downstream_line_context *context = (struct downstream_line_context *)userdata;

    printf("D< %s\n", line);
    if (command_is(line, "PASS") || command_is(line, "NICK") ||
        command_is(line, "USER"))
        return;

    if (command_is(line, "QUIT")) {
        ambnc_downstream_close_client(context->downstream);
        return;
    }

    printf("U> %s\n", line);
    if (ambnc_irc_send_line(context->upstream_sock, line) != 0)
        ambnc_downstream_close_client(context->downstream);
}

static int run_connected(const struct ambnc_upstream_config *config,
                         int sock,
                         struct ambnc_downstream *downstream,
                         struct ambnc_session_state *state)
{
    struct ambnc_irc_framer upstream_framer;
    struct upstream_line_context upstream_context;
    struct downstream_line_context downstream_context;
    char buffer[AMBNC_RECV_BUFFER];

    ambnc_irc_framer_init(&upstream_framer);
    ambnc_state_init(state, config->nick, config->backlog_lines);
    upstream_context.sock = sock;
    upstream_context.downstream = downstream;
    upstream_context.state = state;
    downstream_context.upstream_sock = sock;
    downstream_context.downstream = downstream;

    if (ambnc_irc_send_registration(sock, config->nick, config->user, config->pass) != 0)
        return -1;

    while (!stop_requested()) {
        int sockets[3];
        unsigned long signals = 0;
        unsigned long ready = 0;
        int rc;

        sockets[0] = sock;
        sockets[1] = downstream->listener;
        sockets[2] = downstream->client;
        rc = ambnc_net_wait_many(sockets, 3, SIGBREAKF_CTRL_C, &signals, &ready);
        if ((signals & SIGBREAKF_CTRL_C) != 0) return 0;
        if (rc < 0) return -1;

        if ((ready & 1UL) != 0) {
            int received = ambnc_net_recv(sock, buffer, sizeof(buffer));
            if (received <= 0) return -1;
            ambnc_irc_framer_feed(&upstream_framer,
                                  buffer,
                                  (unsigned int)received,
                                  handle_upstream_line,
                                  &upstream_context);
        }

        if ((ready & 2UL) != 0) {
            int accepted = ambnc_downstream_accept(downstream, state->nick);
            if (accepted == 0) {
                if (ambnc_state_replay(state, replay_send_line, downstream) != 0) {
                    puts("AmBNC: backlog replay failed; detaching downstream");
                    ambnc_downstream_close_client(downstream);
                }
            }
        }

        if ((ready & 4UL) != 0 && downstream->client >= 0) {
            int received = ambnc_net_recv(downstream->client, buffer, sizeof(buffer));
            if (received <= 0) {
                ambnc_downstream_close_client(downstream);
            } else {
                ambnc_irc_framer_feed(&downstream->framer,
                                      buffer,
                                      (unsigned int)received,
                                      handle_downstream_line,
                                      &downstream_context);
            }
        }
    }
    return 0;
}

int ambnc_upstream_run(const struct ambnc_upstream_config *config)
{
    static const unsigned int backoff_seconds[] = { 1, 2, 4, 8, 16, 30 };
    struct ambnc_downstream downstream;
    static struct ambnc_session_state state;
    unsigned int backoff_index = 0;

    if (config == 0 || config->host == 0 || config->nick == 0 || config->user == 0 ||
        config->listen_port == 0 || config->backlog_lines == 0 ||
        config->backlog_lines > AMBNC_STATE_RING_LINES_MAX)
        return 10;

    if (ambnc_net_open() != 0) {
        puts("AmBNC: cannot open bsdsocket.library");
        return 20;
    }

    ambnc_downstream_init(&downstream);
    if (ambnc_downstream_listen(&downstream, config->listen_port) != 0) {
        printf("AmBNC: cannot listen on TCP port %u\n", (unsigned int)config->listen_port);
        ambnc_net_close();
        return 20;
    }
    printf("AmBNC: downstream listener on TCP port %u, backlog %u line(s)/target\n",
           (unsigned int)config->listen_port,
           config->backlog_lines);

    while (!stop_requested()) {
        int sock;

        printf("AmBNC: connecting upstream %s:%u\n",
               config->host, (unsigned int)config->port);
        sock = ambnc_net_connect_ipv4(config->host, config->port);
        if (sock >= 0) {
            puts("AmBNC: upstream connected");
            backoff_index = 0;
            (void)run_connected(config, sock, &downstream, &state);
            ambnc_net_close_socket(sock);
            if (downstream.client >= 0) {
                (void)ambnc_downstream_send_line(&downstream,
                                                  ":AmBNC NOTICE * :Upstream disconnected");
                ambnc_downstream_close_client(&downstream);
            }
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

    ambnc_downstream_close(&downstream);
    ambnc_net_close();
    puts("AmBNC: stopped");
    return 0;
}
