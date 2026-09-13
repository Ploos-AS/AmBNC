#include <stdio.h>
#include <string.h>

#include <dos/dos.h>
#include <proto/dos.h>
#include <proto/exec.h>

#include "downstream.h"
#include "irc.h"
#include "net.h"
#include "rexx.h"
#include "rexx_events.h"
#include "state.h"
#include "upstream.h"

#define AMBNC_RECV_BUFFER 512

struct upstream_line_context {
    int sock;
    struct ambnc_downstream *downstream;
    struct ambnc_session_state *state;
    struct ambnc_rexx_control *control;
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

static unsigned long count_buffered(const struct ambnc_session_state *state)
{
    unsigned long total = 0;
    unsigned int i;
    for (i = 0; i < state->ring_count; ++i) total += state->rings[i].count;
    return total;
}

static void sync_control(struct ambnc_rexx_control *control,
                         const struct ambnc_session_state *state,
                         const struct ambnc_downstream *downstream,
                         int sock)
{
    control->upstream_sock = sock;
    control->upstream_connected = sock >= 0;
    control->downstream_attached = downstream->client >= 0;
    control->channel_count = state->channel_count;
    control->ring_count = state->ring_count;
    control->buffered_lines = count_buffered(state);
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
    sync_control(context->control, context->state, context->downstream, context->sock);

    if (strncmp(line, "PING ", 5) == 0) {
        char response[AMBNC_IRC_LINE_MAX + 1];
        int written = snprintf(response, sizeof(response), "PONG %s", line + 5);
        if (written > 0 && written < (int)sizeof(response)) {
            printf("U> %s\n", response);
            (void)ambnc_irc_send_line(context->sock, response);
        }
        return;
    }

    ambnc_rexx_emit_irc_line(line);
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
                         struct ambnc_session_state *state,
                         struct ambnc_rexx *rexx,
                         struct ambnc_rexx_control *control)
{
    struct ambnc_irc_framer upstream_framer;
    struct upstream_line_context upstream_context;
    struct downstream_line_context downstream_context;
    char buffer[AMBNC_RECV_BUFFER];
    unsigned long rexx_mask = ambnc_rexx_signal_mask(rexx);

    ambnc_irc_framer_init(&upstream_framer);
    ambnc_state_init(state, config->nick, config->backlog_lines);
    upstream_context.sock = sock;
    upstream_context.downstream = downstream;
    upstream_context.state = state;
    upstream_context.control = control;
    downstream_context.upstream_sock = sock;
    downstream_context.downstream = downstream;
    sync_control(control, state, downstream, sock);

    if (ambnc_irc_send_registration(sock, config->nick, config->user, config->pass) != 0)
        return -1;

    ambnc_rexx_emit_lifecycle("ON_CONNECT", config->host);

    while (!stop_requested()) {
        int sockets[3];
        unsigned long signals = 0;
        unsigned long ready = 0;
        int rc;

        sockets[0] = sock;
        sockets[1] = downstream->listener;
        sockets[2] = downstream->client;
        sync_control(control, state, downstream, sock);
        rc = ambnc_net_wait_many(sockets, 3, SIGBREAKF_CTRL_C | rexx_mask,
                                 &signals, &ready);
        if ((signals & SIGBREAKF_CTRL_C) != 0) return 2;
        if ((signals & rexx_mask) != 0) {
            ambnc_rexx_process(rexx, control);
            if (control->request_quit) return 2;
            if (control->request_disconnect || control->request_reconnect) return 1;
            if (control->request_reload) control->request_reload = 0;
        }
        if (rc < 0) return -1;

        if ((ready & 1UL) != 0) {
            int received = ambnc_net_recv(sock, buffer, sizeof(buffer));
            if (received <= 0) return -1;
            ambnc_irc_framer_feed(&upstream_framer, buffer, (unsigned int)received,
                                  handle_upstream_line, &upstream_context);
        }

        if ((ready & 2UL) != 0) {
            int accepted = ambnc_downstream_accept(downstream, state->nick);
            if (accepted == 0 && ambnc_state_replay(state, replay_send_line, downstream) != 0) {
                puts("AmBNC: backlog replay failed; detaching downstream");
                ambnc_downstream_close_client(downstream);
            }
        }

        if ((ready & 4UL) != 0 && downstream->client >= 0) {
            int received = ambnc_net_recv(downstream->client, buffer, sizeof(buffer));
            if (received <= 0) {
                ambnc_downstream_close_client(downstream);
            } else {
                ambnc_irc_framer_feed(&downstream->framer, buffer, (unsigned int)received,
                                      handle_downstream_line, &downstream_context);
            }
        }
    }
    return 2;
}

static int wait_for_connect(struct ambnc_rexx *rexx,
                            struct ambnc_rexx_control *control,
                            struct ambnc_session_state *state,
                            struct ambnc_downstream *downstream)
{
    unsigned long rexx_mask = ambnc_rexx_signal_mask(rexx);
    while (!stop_requested()) {
        sync_control(control, state, downstream, -1);
        if ((Wait(SIGBREAKF_CTRL_C | rexx_mask) & SIGBREAKF_CTRL_C) != 0) return 2;
        ambnc_rexx_process(rexx, control);
        if (control->request_quit) return 2;
        if (control->request_reconnect) {
            control->request_reconnect = 0;
            control->request_disconnect = 0;
            return 0;
        }
        if (control->request_reload) control->request_reload = 0;
    }
    return 2;
}

int ambnc_upstream_run(const struct ambnc_upstream_config *config)
{
    static const unsigned int backoff_seconds[] = { 1, 2, 4, 8, 16, 30 };
    struct ambnc_downstream downstream;
    static struct ambnc_session_state state;
    struct ambnc_rexx rexx;
    struct ambnc_rexx_control control;
    unsigned int backoff_index = 0;
    int manually_disconnected = 0;

    if (config == 0 || config->host == 0 || config->nick == 0 || config->user == 0 ||
        config->listen_port == 0 || config->backlog_lines == 0 ||
        config->backlog_lines > AMBNC_STATE_RING_LINES_MAX)
        return 10;

    memset(&control, 0, sizeof(control));
    control.upstream_sock = -1;

    if (ambnc_net_open() != 0) {
        puts("AmBNC: cannot open bsdsocket.library");
        return 20;
    }
    if (ambnc_rexx_open(&rexx) != 0) {
        puts("AmBNC: cannot create ARexx port AMBNC");
        ambnc_net_close();
        return 20;
    }
    ambnc_rexx_events_bind(&rexx, &control);

    ambnc_downstream_init(&downstream);
    ambnc_state_init(&state, config->nick, config->backlog_lines);
    if (ambnc_downstream_listen(&downstream, config->listen_port) != 0) {
        printf("AmBNC: cannot listen on TCP port %u\n", (unsigned int)config->listen_port);
        ambnc_rexx_close(&rexx);
        ambnc_net_close();
        return 20;
    }
    printf("AmBNC: downstream listener on TCP port %u, backlog %u line(s)/target\n",
           (unsigned int)config->listen_port, config->backlog_lines);
    puts("AmBNC: ARexx port AMBNC active");

    while (!stop_requested() && !control.request_quit) {
        int sock;
        int result;

        if (manually_disconnected) {
            result = wait_for_connect(&rexx, &control, &state, &downstream);
            if (result == 2) break;
            manually_disconnected = 0;
        }

        printf("AmBNC: connecting upstream %s:%u\n",
               config->host, (unsigned int)config->port);
        sock = ambnc_net_connect_ipv4(config->host, config->port);
        if (sock >= 0) {
            puts("AmBNC: upstream connected");
            backoff_index = 0;
            control.request_disconnect = 0;
            control.request_reconnect = 0;
            result = run_connected(config, sock, &downstream, &state, &rexx, &control);
            ambnc_rexx_emit_lifecycle("ON_DISCONNECT", config->host);
            ambnc_net_close_socket(sock);
            sync_control(&control, &state, &downstream, -1);
            if (downstream.client >= 0) {
                (void)ambnc_downstream_send_line(&downstream,
                                                  ":AmBNC NOTICE * :Upstream disconnected");
                ambnc_downstream_close_client(&downstream);
            }
            if (result == 2 || control.request_quit) break;
            if (control.request_disconnect && !control.request_reconnect) {
                control.request_disconnect = 0;
                manually_disconnected = 1;
                continue;
            }
            control.request_disconnect = 0;
            control.request_reconnect = 0;
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
    ambnc_rexx_close(&rexx);
    ambnc_net_close();
    puts("AmBNC: stopped");
    return 0;
}
