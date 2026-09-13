#ifndef AMBNC_UPSTREAM_H
#define AMBNC_UPSTREAM_H

struct ambnc_upstream_config {
    const char *host;
    unsigned short port;
    const char *nick;
    const char *user;
    const char *pass;
};

int ambnc_upstream_run(const struct ambnc_upstream_config *config);

#endif
