#include <stdio.h>
#include <string.h>

#include <proto/exec.h>

#include "downstream.h"
#include "irc.h"
#include "multinet.h"
#include "net.h"
#include "rexx.h"
#include "rexx_events.h"
#include "state.h"

#define AMBNC_MULTI_SOCKET_MAX (AMBNC_NETWORKS_MAX * 3)
#define AMBNC_MULTI_RECV_BUFFER 512

static const unsigned int reconnect_backoff[] = { 1, 2, 4, 8, 16, 30 };

enum socket_kind {
    SOCKET_UPSTREAM = 1,
    SOCKET_LISTENER = 2,
    SOCKET_DOWNSTREAM = 3
};

struct network_runtime {
    const struct ambnc_network_config *config;
    int upstream;
    struct ambnc_downstream downstream;
    struct ambnc_session_state state;
    struct ambnc_irc_framer upstream_framer;
    unsigned int backoff_index;
    unsigned int retry_seconds;
};

static struct network_runtime runtimes[AMBNC_NETWORKS_MAX];

struct line_context {
    struct network_runtime *runtime;
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
                         struct network_runtime *runtime,
                         unsigned int count)
{
    unsigned int i;
    control->upstream_sock = -1;
    control->upstream_connected = 0;
    control->downstream_attached = 0;
    control->channel_count = 0;
    control->ring_count = 0;
    control->buffered_lines = 0;

    for (i = 0; i < count; ++i) {
        if (runtime[i].upstream >= 0) {
            control->upstream_connected = 1;
            if (control->upstream_sock < 0) control->upstream_sock = runtime[i].upstream;
        }
        if (runtime[i].downstream.client >= 0) control->downstream_attached = 1;
        control->channel_count += runtime[i].state.channel_count;
        control->ring_count += runtime[i].state.ring_count;
        control->buffered_lines += count_buffered(&runtime[i].state);
    }
}

static int replay_send_line(void *userdata, const char *line)
{
    return ambnc_downstream_send_line((struct ambnc_downstream *)userdata, line);
}

static void hook_lifecycle(const struct network_runtime *runtime,
                           const char *event_name)
{
    char detail[192];
    int written = snprintf(detail, sizeof(detail), "%s %s:%u",
                           runtime->config->name,
                           runtime->config->host,
                           (unsigned int)runtime->config->port);
    if (written > 0 && written < (int)sizeof(detail))
        ambnc_rexx_emit_lifecycle(event_name, detail);
}

static void schedule_reconnect(struct network_runtime *runtime)
{
    runtime->retry_seconds = reconnect_backoff[runtime->backoff_index];
    if (runtime->backoff_index + 1U <
        sizeof(reconnect_backoff) / sizeof(reconnect_backoff[0]))
        ++runtime->backoff_index;
}

static void disconnect_runtime(struct network_runtime *runtime, int schedule)
{
    if (runtime->upstream >= 0) {
        hook_lifecycle(runtime, "ON_DISCONNECT");
        ambnc_net_close_socket(runtime->upstream);
        runtime->upstream = -1;
    }
    if (runtime->downstream.client >= 0) {
        (void)ambnc_downstream_send_line(&runtime->downstream,
                                          ":AmBNC NOTICE * :Upstream disconnected");
        ambnc_downstream_close_client(&runtime->downstream);
    }
    if (schedule) schedule_reconnect(runtime);
}

static int connect_runtime(struct network_runtime *runtime)
{
    int sock;
    printf("AmBNC[%s]: connecting %s:%u\n",
           runtime->config->name,
           runtime->config->host,
           (unsigned int)runtime->config->port);

    sock = ambnc_net_connect_ipv4(runtime->config->host, runtime->config->port);
    if (sock < 0) {
        printf("AmBNC[%s]: connect failed\n", runtime->config->name);
        schedule_reconnect(runtime);
        return -1;
    }

    runtime->upstream = sock;
    runtime->backoff_index = 0;
    runtime->retry_seconds = 0;
    ambnc_irc_framer_init(&runtime->upstream_framer);
    ambnc_state_init(&runtime->state,
                     runtime->config->nick,
                     runtime->config->backlog_lines);

    if (ambnc_irc_send_registration(sock,
                                    runtime->config->nick,
                                    runtime->config->user,
                                    runtime->config->pass[0] != '\0' ? runtime->config->pass : 0) != 0) {
        ambnc_net_close_socket(sock);
        runtime->upstream = -1;
        schedule_reconnect(runtime);
        return -1;
    }

    printf("AmBNC[%s]: upstream connected\n", runtime->config->name);
    hook_lifecycle(runtime, "ON_CONNECT");
    return 0;
}

static void handle_upstream_line(const char *line, void *userdata)
{
    struct line_context *context = (struct line_context *)userdata;
    struct network_runtime *runtime = context->runtime;
    int attached = runtime->downstream.client >= 0;

    printf("U[%s]< %s\n", runtime->config->name, line);
    ambnc_state_observe_line(&runtime->state, line, attached);

    if (strncmp(line, "PING ", 5) == 0) {
        char response[AMBNC_IRC_LINE_MAX + 1];
        int written = snprintf(response, sizeof(response), "PONG %s", line + 5);
        if (written > 0 && written < (int)sizeof(response))
            (void)ambnc_irc_send_line(runtime->upstream, response);
        return;
    }

    ambnc_rexx_emit_irc_line(line);
    if (attached && ambnc_downstream_send_line(&runtime->downstream, line) != 0)
        ambnc_downstream_close_client(&runtime->downstream);
}

static void handle_downstream_line(const char *line, void *userdata)
{
    struct line_context *context = (struct line_context *)userdata;
    struct network_runtime *runtime = context->runtime;

    printf("D[%s]< %s\n", runtime->config->name, line);
    if (command_is(line, "PASS") || command_is(line, "NICK") || command_is(line, "USER"))
        return;
    if (command_is(line, "QUIT")) {
        ambnc_downstream_close_client(&runtime->downstream);
        return;
    }
    if (runtime->upstream < 0) {
        (void)ambnc_downstream_send_line(&runtime->downstream,
                                          ":AmBNC NOTICE * :Upstream is offline");
        return;
    }
    if (ambnc_irc_send_line(runtime->upstream, line) != 0)
        disconnect_runtime(runtime, 1);
}

static int init_runtime(struct network_runtime *runtime,
                        const struct ambnc_network_config *config)
{
    runtime->config = config;
    runtime->upstream = -1;
    runtime->backoff_index = 0;
    runtime->retry_seconds = 0;
    ambnc_downstream_init(&runtime->downstream);
    ambnc_state_init(&runtime->state, config->nick, config->backlog_lines);
    ambnc_irc_framer_init(&runtime->upstream_framer);

    if (ambnc_downstream_listen(&runtime->downstream, config->listen_port) != 0)
        return -1;

    printf("AmBNC[%s]: downstream port %u, backlog %u\n",
           config->name,
           (unsigned int)config->listen_port,
           config->backlog_lines);
    return 0;
}

static void close_runtime(struct network_runtime *runtime)
{
    disconnect_runtime(runtime, 0);
    ambnc_downstream_close(&runtime->downstream);
}

int ambnc_multinet_run(const struct ambnc_networks_config *config)
{
    struct ambnc_rexx rexx;
    struct ambnc_rexx_control control;
    struct line_context contexts[AMBNC_NETWORKS_MAX];
    unsigned int i;
    int paused = 0;

    if (config == 0 || config->count == 0 || config->count > AMBNC_NETWORKS_MAX) return 10;
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

    for (i = 0; i < config->count; ++i) {
        if (init_runtime(&runtimes[i], &config->networks[i]) != 0) {
            unsigned int j;
            printf("AmBNC[%s]: cannot listen on port %u\n",
                   config->networks[i].name,
                   (unsigned int)config->networks[i].listen_port);
            for (j = 0; j < i; ++j) close_runtime(&runtimes[j]);
            ambnc_rexx_close(&rexx);
            ambnc_net_close();
            return 20;
        }
        contexts[i].runtime = &runtimes[i];
    }

    puts("AmBNC: multi-session ARexx port AMBNC active");
    for (i = 0; i < config->count; ++i) (void)connect_runtime(&runtimes[i]);

    while (!stop_requested() && !control.request_quit) {
        int sockets[AMBNC_MULTI_SOCKET_MAX];
        unsigned char kinds[AMBNC_MULTI_SOCKET_MAX];
        unsigned char indexes[AMBNC_MULTI_SOCKET_MAX];
        unsigned int socket_count = 0;
        unsigned long ready = 0;
        unsigned long signals = 0;
        unsigned long rexx_mask = ambnc_rexx_signal_mask(&rexx);
        int rc;

        for (i = 0; i < config->count; ++i) {
            if (runtimes[i].upstream >= 0) {
                sockets[socket_count] = runtimes[i].upstream;
                kinds[socket_count] = SOCKET_UPSTREAM;
                indexes[socket_count++] = (unsigned char)i;
            }
            sockets[socket_count] = runtimes[i].downstream.listener;
            kinds[socket_count] = SOCKET_LISTENER;
            indexes[socket_count++] = (unsigned char)i;
            if (runtimes[i].downstream.client >= 0) {
                sockets[socket_count] = runtimes[i].downstream.client;
                kinds[socket_count] = SOCKET_DOWNSTREAM;
                indexes[socket_count++] = (unsigned char)i;
            }
        }

        sync_control(&control, runtimes, config->count);
        rc = ambnc_net_wait_many_timeout(sockets, socket_count,
                                         SIGBREAKF_CTRL_C | rexx_mask,
                                         &signals, &ready, 1);
        if ((signals & SIGBREAKF_CTRL_C) != 0) break;
        if ((signals & rexx_mask) != 0) {
            ambnc_rexx_process(&rexx, &control);
            if (control.request_quit) break;
            if (control.request_disconnect) {
                for (i = 0; i < config->count; ++i) disconnect_runtime(&runtimes[i], 0);
                paused = 1;
                control.request_disconnect = 0;
            }
            if (control.request_reconnect) {
                for (i = 0; i < config->count; ++i) {
                    disconnect_runtime(&runtimes[i], 0);
                    runtimes[i].retry_seconds = 0;
                    runtimes[i].backoff_index = 0;
                }
                paused = 0;
                control.request_reconnect = 0;
            }
            if (control.request_reload) control.request_reload = 0;
        }
        if (rc < 0) break;

        for (i = 0; i < socket_count; ++i) {
            struct network_runtime *runtime;
            char buffer[AMBNC_MULTI_RECV_BUFFER];
            int received;
            if ((ready & (1UL << i)) == 0) continue;
            runtime = &runtimes[indexes[i]];

            if (kinds[i] == SOCKET_UPSTREAM) {
                received = ambnc_net_recv(runtime->upstream, buffer, sizeof(buffer));
                if (received <= 0) {
                    disconnect_runtime(runtime, !paused);
                } else {
                    ambnc_irc_framer_feed(&runtime->upstream_framer,
                                          buffer,
                                          (unsigned int)received,
                                          handle_upstream_line,
                                          &contexts[indexes[i]]);
                }
            } else if (kinds[i] == SOCKET_LISTENER) {
                int accepted = ambnc_downstream_accept(&runtime->downstream,
                                                       runtime->state.nick);
                if (accepted == 0 &&
                    ambnc_state_replay(&runtime->state,
                                       replay_send_line,
                                       &runtime->downstream) != 0)
                    ambnc_downstream_close_client(&runtime->downstream);
            } else if (kinds[i] == SOCKET_DOWNSTREAM) {
                received = ambnc_net_recv(runtime->downstream.client,
                                          buffer,
                                          sizeof(buffer));
                if (received <= 0) {
                    ambnc_downstream_close_client(&runtime->downstream);
                } else {
                    ambnc_irc_framer_feed(&runtime->downstream.framer,
                                          buffer,
                                          (unsigned int)received,
                                          handle_downstream_line,
                                          &contexts[indexes[i]]);
                }
            }
        }

        if (!paused) {
            for (i = 0; i < config->count; ++i) {
                if (runtimes[i].upstream >= 0) continue;
                if (runtimes[i].retry_seconds > 0) --runtimes[i].retry_seconds;
                if (runtimes[i].retry_seconds == 0) (void)connect_runtime(&runtimes[i]);
            }
        }
    }

    for (i = 0; i < config->count; ++i) close_runtime(&runtimes[i]);
    ambnc_rexx_close(&rexx);
    ambnc_net_close();
    puts("AmBNC: multi-session stopped");
    return 0;
}
