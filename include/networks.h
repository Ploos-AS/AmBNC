#ifndef AMBNC_NETWORKS_H
#define AMBNC_NETWORKS_H

#include "state.h"

#define AMBNC_NETWORKS_MAX 4
#define AMBNC_NETWORK_NAME_MAX 31
#define AMBNC_HOST_MAX 127
#define AMBNC_USER_MAX 31
#define AMBNC_PASS_MAX 63

#define AMBNC_TLS_PLAIN 0
#define AMBNC_TLS_PROXY 1

struct ambnc_network_config {
    char name[AMBNC_NETWORK_NAME_MAX + 1];
    char host[AMBNC_HOST_MAX + 1];
    unsigned short port;
    char nick[AMBNC_STATE_NICK_MAX + 1];
    char user[AMBNC_USER_MAX + 1];
    char pass[AMBNC_PASS_MAX + 1];
    unsigned short listen_port;
    unsigned int backlog_lines;
    int cap_enabled;
    int sasl_plain;
    int tls_mode;
};

struct ambnc_networks_config {
    struct ambnc_network_config networks[AMBNC_NETWORKS_MAX];
    unsigned int count;
};

void ambnc_networks_init(struct ambnc_networks_config *config);
int ambnc_networks_load(struct ambnc_networks_config *config, const char *path,
                        char *error, unsigned int error_size);

int ambnc_m7_send_registration(int sock,
                               const char *nick,
                               const char *user,
                               const char *pass);
void ambnc_m7_state_observe_line(struct ambnc_session_state *state,
                                 const char *line,
                                 int downstream_attached);

#define ambnc_irc_send_registration ambnc_m7_send_registration
#define ambnc_state_observe_line ambnc_m7_state_observe_line

#endif
