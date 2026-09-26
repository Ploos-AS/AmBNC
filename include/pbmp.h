#ifndef AMBNC_PBMP_H
#define AMBNC_PBMP_H

#include <stddef.h>

struct ambnc_networks_config;

/*
 * Optional PBMP management adapter.
 *
 * This layer contains no socket or BotWeb dependency. A platform transport
 * may pass one complete PBMP JSON request to ambnc_pbmp_handle() and return
 * the generated response. AmBNC's IRC/BNC operation never depends on it.
 */
#define AMBNC_PBMP_VERSION 1
#define AMBNC_PBMP_ENDPOINT_ID "ambnc"
#define AMBNC_PBMP_ENDPOINT_KIND "bouncer"

void ambnc_pbmp_set_networks(const struct ambnc_networks_config *networks);

int ambnc_pbmp_handle(const char *request, char *response, size_t response_size);

#endif
