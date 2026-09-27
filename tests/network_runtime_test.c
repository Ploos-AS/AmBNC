#include <stdio.h>

#include "networks.h"

static int expect_int(const char *name, int actual, int expected)
{
    if (actual == expected) return 0;
    fprintf(stderr, "%s: got %d, expected %d\n", name, actual, expected);
    return 1;
}

static int expect_uint(const char *name, unsigned int actual, unsigned int expected)
{
    if (actual == expected) return 0;
    fprintf(stderr, "%s: got %u, expected %u\n", name, actual, expected);
    return 1;
}

int main(void)
{
    unsigned int i;
    int failed = 0;

    ambnc_network_runtime_reset();
    failed |= expect_int("reset paused", ambnc_network_runtime_get_paused(), 0);
    failed |= expect_uint("reset uptime", (unsigned int)ambnc_runtime_get_uptime(), 0);
    for (i = 0; i < AMBNC_NETWORKS_MAX; ++i) {
        failed |= expect_uint("reset reconnect attempts",
                              (unsigned int)ambnc_network_runtime_get_reconnect_attempts(i), 0);
        failed |= expect_uint("reset connected seconds",
                              (unsigned int)ambnc_network_runtime_get_connected_seconds(i), 0);
    }
    for (i = 0; i < AMBNC_NETWORKS_MAX; ++i) {
        failed |= expect_int("reset state", ambnc_network_runtime_get(i),
                             AMBNC_NETWORK_RUNTIME_CONFIGURED);
        failed |= expect_uint("reset retry", ambnc_network_runtime_get_retry(i), 0);
    }

    ambnc_runtime_set_uptime(42);
    failed |= expect_uint("uptime 42", (unsigned int)ambnc_runtime_get_uptime(), 42);

    ambnc_network_runtime_increment_reconnect_attempts(0);
    ambnc_network_runtime_increment_reconnect_attempts(0);
    ambnc_network_runtime_increment_reconnect_attempts(1);
    failed |= expect_uint("network 0 reconnect attempts",
                          (unsigned int)ambnc_network_runtime_get_reconnect_attempts(0), 2);
    failed |= expect_uint("network 1 reconnect attempts",
                          (unsigned int)ambnc_network_runtime_get_reconnect_attempts(1), 1);
    ambnc_network_runtime_increment_reconnect_attempts(AMBNC_NETWORKS_MAX);
    failed |= expect_uint("out of range reconnect attempts",
                          (unsigned int)ambnc_network_runtime_get_reconnect_attempts(AMBNC_NETWORKS_MAX), 0);

    ambnc_network_runtime_set(0, AMBNC_NETWORK_RUNTIME_CONNECTING);
    ambnc_network_runtime_set_retry(0, 8);
    failed |= expect_int("connecting", ambnc_network_runtime_get(0),
                         AMBNC_NETWORK_RUNTIME_CONNECTING);
    failed |= expect_uint("retry 8", ambnc_network_runtime_get_retry(0), 8);

    ambnc_network_runtime_set(0, AMBNC_NETWORK_RUNTIME_CONNECTED);
    ambnc_network_runtime_set_connected_seconds(0, 17);
    failed |= expect_uint("connected seconds",
                          (unsigned int)ambnc_network_runtime_get_connected_seconds(0), 17);
    ambnc_network_runtime_set_retry(0, 0);
    failed |= expect_int("connected", ambnc_network_runtime_get(0),
                         AMBNC_NETWORK_RUNTIME_CONNECTED);
    failed |= expect_uint("connected retry", ambnc_network_runtime_get_retry(0), 0);

    ambnc_network_runtime_set_paused(1);
    failed |= expect_int("paused", ambnc_network_runtime_get_paused(), 1);
    ambnc_network_runtime_set_paused(0);
    failed |= expect_int("resumed", ambnc_network_runtime_get_paused(), 0);

    ambnc_network_runtime_set(0, AMBNC_NETWORK_RUNTIME_DISCONNECTED);
    ambnc_network_runtime_set_connected_seconds(0, 0);
    failed |= expect_uint("disconnected connected seconds",
                          (unsigned int)ambnc_network_runtime_get_connected_seconds(0), 0);
    ambnc_network_runtime_set_retry(0, 30);
    failed |= expect_int("disconnected", ambnc_network_runtime_get(0),
                         AMBNC_NETWORK_RUNTIME_DISCONNECTED);
    failed |= expect_uint("backoff retry", ambnc_network_runtime_get_retry(0), 30);

    ambnc_network_runtime_set(AMBNC_NETWORKS_MAX, AMBNC_NETWORK_RUNTIME_CONNECTED);
    ambnc_network_runtime_set_retry(AMBNC_NETWORKS_MAX, 99);
    failed |= expect_int("out of range state",
                         ambnc_network_runtime_get(AMBNC_NETWORKS_MAX),
                         AMBNC_NETWORK_RUNTIME_CONFIGURED);
    failed |= expect_uint("out of range retry",
                          ambnc_network_runtime_get_retry(AMBNC_NETWORKS_MAX), 0);
    failed |= expect_uint("out of range connected seconds",
                          (unsigned int)ambnc_network_runtime_get_connected_seconds(AMBNC_NETWORKS_MAX), 0);

    if (failed) return 1;
    puts("network runtime snapshot: PASS");
    return 0;
}
