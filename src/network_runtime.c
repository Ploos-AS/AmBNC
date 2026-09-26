#include "networks.h"

static int runtime_states[AMBNC_NETWORKS_MAX];
static unsigned int runtime_retry[AMBNC_NETWORKS_MAX];

void ambnc_network_runtime_reset(void)
{
    unsigned int i;
    for (i = 0; i < AMBNC_NETWORKS_MAX; ++i) {
        runtime_states[i] = AMBNC_NETWORK_RUNTIME_CONFIGURED;
        runtime_retry[i] = 0;
    }
}

void ambnc_network_runtime_set(unsigned int index, int state)
{
    if (index < AMBNC_NETWORKS_MAX)
        runtime_states[index] = state;
}

int ambnc_network_runtime_get(unsigned int index)
{
    if (index >= AMBNC_NETWORKS_MAX)
        return AMBNC_NETWORK_RUNTIME_CONFIGURED;
    return runtime_states[index];
}

void ambnc_network_runtime_set_retry(unsigned int index, unsigned int seconds)
{
    if (index < AMBNC_NETWORKS_MAX)
        runtime_retry[index] = seconds;
}

unsigned int ambnc_network_runtime_get_retry(unsigned int index)
{
    if (index >= AMBNC_NETWORKS_MAX)
        return 0;
    return runtime_retry[index];
}
