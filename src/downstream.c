#include <stdio.h>

#include "downstream.h"
#include "net.h"

void ambnc_downstream_init(struct ambnc_downstream *downstream)
{
    downstream->listener = -1;
    downstream->client = -1;
    ambnc_irc_framer_init(&downstream->framer);
}

int ambnc_downstream_listen(struct ambnc_downstream *downstream, unsigned short port)
{
    downstream->listener = ambnc_net_listen_ipv4(port);
    return downstream->listener >= 0 ? 0 : -1;
}

int ambnc_downstream_accept(struct ambnc_downstream *downstream, const char *nick)
{
    char welcome[AMBNC_IRC_LINE_MAX + 1];
    int written;
    int client = ambnc_net_accept(downstream->listener);

    if (client < 0) return -1;
    if (downstream->client >= 0) {
        static const char busy[] = ":AmBNC NOTICE * :AmBNC already has a downstream client\r\n";
        (void)ambnc_net_send_all(client, busy, (unsigned int)(sizeof(busy) - 1U));
        ambnc_net_close_socket(client);
        return 1;
    }

    downstream->client = client;
    ambnc_irc_framer_init(&downstream->framer);
    written = snprintf(welcome, sizeof(welcome),
                       ":AmBNC 001 %s :Attached to AmBNC upstream session",
                       nick != 0 && nick[0] != '\0' ? nick : "client");
    if (written <= 0 || written >= (int)sizeof(welcome) ||
        ambnc_downstream_send_line(downstream, welcome) != 0) {
        ambnc_downstream_close_client(downstream);
        return -1;
    }
    puts("AmBNC: downstream client attached");
    return 0;
}

void ambnc_downstream_close_client(struct ambnc_downstream *downstream)
{
    if (downstream->client >= 0) {
        ambnc_net_close_socket(downstream->client);
        downstream->client = -1;
        ambnc_irc_framer_init(&downstream->framer);
        puts("AmBNC: downstream client detached");
    }
}

void ambnc_downstream_close(struct ambnc_downstream *downstream)
{
    ambnc_downstream_close_client(downstream);
    if (downstream->listener >= 0) {
        ambnc_net_close_socket(downstream->listener);
        downstream->listener = -1;
    }
}

int ambnc_downstream_send_line(struct ambnc_downstream *downstream, const char *line)
{
    unsigned int length = 0;
    if (downstream->client < 0 || line == 0) return -1;
    while (line[length] != '\0') ++length;
    if (length > AMBNC_IRC_LINE_MAX) return -1;
    if (ambnc_net_send_all(downstream->client, line, length) != 0) return -1;
    return ambnc_net_send_all(downstream->client, "\r\n", 2);
}
