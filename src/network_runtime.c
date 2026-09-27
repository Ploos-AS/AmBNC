#include "networks.h"

static int runtime_states[AMBNC_NETWORKS_MAX];
static unsigned int runtime_retry[AMBNC_NETWORKS_MAX];
static unsigned long runtime_reconnect_attempts[AMBNC_NETWORKS_MAX];
static int runtime_paused;
static unsigned long runtime_uptime;

void ambnc_network_runtime_reset(void)
{
    runtime_paused = 0;
    runtime_uptime = 0;
    unsigned int i;
    for (i = 0; i < AMBNC_NETWORKS_MAX; ++i) {
        runtime_states[i] = AMBNC_NETWORK_RUNTIME_CONFIGURED;
        runtime_retry[i] = 0;
        runtime_reconnect_attempts[i] = 0;
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

void ambnc_network_runtime_increment_reconnect_attempts(unsigned int index)
{
    if (index < AMBNC_NETWORKS_MAX)
        ++runtime_reconnect_attempts[index];
}

unsigned long ambnc_network_runtime_get_reconnect_attempts(unsigned int index)
{
    if (index >= AMBNC_NETWORKS_MAX)
        return 0;
    return runtime_reconnect_attempts[index];
}

void ambnc_network_runtime_set_paused(int paused)
{
    runtime_paused = paused ? 1 : 0;
}

int ambnc_network_runtime_get_paused(void)
{
    return runtime_paused;
}

void ambnc_runtime_set_uptime(unsigned long seconds)
{
    runtime_uptime = seconds;
}

unsigned long ambnc_runtime_get_uptime(void)
{
    return runtime_uptime;
}
