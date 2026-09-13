#ifndef AMBNC_DOWNSTREAM_H
#define AMBNC_DOWNSTREAM_H

#include "irc.h"

struct ambnc_downstream {
    int listener;
    int client;
    struct ambnc_irc_framer framer;
};

void ambnc_downstream_init(struct ambnc_downstream *downstream);
int ambnc_downstream_listen(struct ambnc_downstream *downstream, unsigned short port);
int ambnc_downstream_accept(struct ambnc_downstream *downstream, const char *nick);
void ambnc_downstream_close_client(struct ambnc_downstream *downstream);
void ambnc_downstream_close(struct ambnc_downstream *downstream);
int ambnc_downstream_send_line(struct ambnc_downstream *downstream, const char *line);

#endif
