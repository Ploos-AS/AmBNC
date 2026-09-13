#ifndef AMBNC_NETWORKS_H
#define AMBNC_NETWORKS_H

#include "state.h"

#define AMBNC_NETWORKS_MAX 4
#define AMBNC_NETWORK_NAME_MAX 31
#define AMBNC_HOST_MAX 127
#define AMBNC_USER_MAX 31
#define AMBNC_PASS_MAX 63

struct ambnc_network_config {
    char name[AMBNC_NETWORK_NAME_MAX + 1];
    char host[AMBNC_HOST_MAX + 1];
    unsigned short port;
    char nick[AMBNC_STATE_NICK_MAX + 1];
    char user[AMBNC_USER_MAX + 1];
    char pass[AMBNC_PASS_MAX + 1];
    unsigned short listen_port;
    unsigned int backlog_lines;
};

struct ambnc_networks_config {
    struct ambnc_network_config networks[AMBNC_NETWORKS_MAX];
    unsigned int count;
};

void ambnc_networks_init(struct ambnc_networks_config *config);
int ambnc_networks_load(struct ambnc_networks_config *config, const char *path,
                        char *error, unsigned int error_size);

#endif
