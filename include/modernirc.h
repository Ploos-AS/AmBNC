#ifndef AMBNC_MODERNIRC_H
#define AMBNC_MODERNIRC_H

#include "networks.h"

struct ambnc_modernirc {
    int cap_active;
    int cap_ended;
    int sasl_requested;
    int sasl_complete;
};

void ambnc_modernirc_init(struct ambnc_modernirc *state);
int ambnc_modernirc_start(int sock,
                          const struct ambnc_network_config *config,
                          struct ambnc_modernirc *state);
int ambnc_modernirc_handle_line(int sock,
                                const struct ambnc_network_config *config,
                                struct ambnc_modernirc *state,
                                const char *line);

#endif
