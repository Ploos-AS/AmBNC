#include <stdio.h>
#include <string.h>

#include "pbmp.h"
#include "networks.h"

static int check(const char *method, const char *needle)
{
    char request[256];
    char response[1024];

    snprintf(request, sizeof(request),
        "{\"pbmp\":1,\"type\":\"request\",\"id\":\"test-1\",\"method\":\"%s\",\"params\":{}}",
        method);
    if (ambnc_pbmp_handle(request, response, sizeof(response)) < 0) {
        fprintf(stderr, "%s: handler failed\n", method);
        return 1;
    }
    if (!strstr(response, "\"id\":\"test-1\"") || !strstr(response, needle)) {
        fprintf(stderr, "%s: unexpected response: %s\n", method, response);
        return 1;
    }
    return 0;
}

static int check_raw(const char *request, const char *needle, int expect_success)
{
    char response[1024];
    int rc = ambnc_pbmp_handle(request, response, sizeof(response));

    if ((rc >= 0) != expect_success) {
        fprintf(stderr, "raw request: unexpected return %d\n", rc);
        return 1;
    }
    if (expect_success && !strstr(response, needle)) {
        fprintf(stderr, "raw request: unexpected response: %s\n", response);
        return 1;
    }
    return 0;
}

int main(void)
{
    int failed = 0;
    struct ambnc_networks_config networks;

    memset(&networks, 0, sizeof(networks));

    failed |= check("pbmp.info", "\"version\":1");
    failed |= check("capabilities.list", "\"endpoint.info\"");
    failed |= check("capabilities.list", "\"networks.list\"");
    failed |= check("endpoint.info", "\"kind\":\"bouncer\"");
    failed |= check("endpoint.info", "\"uptime_seconds\":0");
    ambnc_runtime_set_uptime(42);
    failed |= check("endpoint.info", "\"uptime_seconds\":42");
    failed |= check("networks.list", "\"networks\":[]");
    failed |= check("unsupported.method", "\"code\":\"not_supported\"");
    failed |= check_raw(
        "{ \"pbmp\" : 1, \"type\" : \"request\", \"id\" : \"spaced-1\", "
        "\"method\" : \"endpoint.info\", \"params\" : {} }",
        "\"id\":\"spaced-1\"", 1);
    failed |= check_raw(
        "{\"pbmp\":1,\"type\":\"request\",\"id\":\"quote\\\\\\\"id\","
        "\"method\":\"pbmp.info\",\"params\":{}}",
        "\"id\":\"quote\\\\\\\"id\"", 1);
    failed |= check_raw(
        "{\"note\":\"\\\"method\\\":\\\"unsupported.method\\\"\",\"pbmp\":1,"
        "\"type\":\"request\",\"id\":\"top-level-method\","
        "\"method\":\"pbmp.info\",\"params\":{}}",
        "\"version\":1", 1);
    failed |= check_raw(
        "{\"pbmp\":1,\"type\":\"request\",\"id\":\"dup-method\","
        "\"method\":\"pbmp.info\",\"method\":\"endpoint.info\",\"params\":{}}",
        "", 0);
    failed |= check_raw(
        "{\"pbmp\":1,\"pbmp\":1,\"type\":\"request\",\"id\":\"dup-version\","
        "\"method\":\"pbmp.info\",\"params\":{}}",
        "", 0);
    failed |= check_raw(
        "{\"pbmp\":1,\"type\":\"request\",\"id\":\"unicode-\\u00f8\","
        "\"method\":\"pbmp.info\",\"params\":{}}",
        "\"id\":\"unicode-ø\"", 1);
    failed |= check_raw(
        "{\"pbmp\":1,\"type\":\"request\",\"id\":\"emoji-\\ud83d\\ude80\","
        "\"method\":\"pbmp.info\",\"params\":{}}",
        "\"id\":\"emoji-🚀\"", 1);
    failed |= check_raw(
        "{\"pbmp\":1,\"type\":\"request\",\"id\":\"bad-\\ud83d\","
        "\"method\":\"pbmp.info\",\"params\":{}}",
        "", 0);
    failed |= check_raw(
        "{\"extra\":{\"items\":[{\"text\":\"},],fake method,comma\"},"
        "{\"nested\":\"[still,string]\"}]},\"pbmp\":1,\"type\":\"request\","
        "\"id\":\"nested-extra\",\"method\":\"pbmp.info\","
        "\"params\":{\"note\":\"} ], comma , inside string\"}}",
        "\"id\":\"nested-extra\"", 1);
    failed |= check_raw(
        "{\"pbmp\":1,\"type\":\"request\",\"id\":\"nested-before-method\","
        "\"params\":{\"array\":[1,{\"value\":\"}, method fake\"},3]},"
        "\"method\":\"endpoint.info\"}",
        "\"kind\":\"bouncer\"", 1);
    failed |= check_raw(
        "{\"pbmp\":1,\"type\":\"request\",\"id\":\"trailing-ok\","
        "\"method\":\"pbmp.info\",\"params\":{}}   \t\n",
        "\"id\":\"trailing-ok\"", 1);
    failed |= check_raw(
        "{\"pbmp\":1,\"type\":\"request\",\"id\":\"trailing-bad\","
        "\"method\":\"pbmp.info\",\"params\":{}}garbage",
        "", 0);
    failed |= check_raw(
        "{\"pbmp\":1,\"type\":\"request\",\"id\":\"double-object\","
        "\"method\":\"pbmp.info\",\"params\":{}}{}",
        "", 0);
    failed |= check_raw(
        "{\"pbmp\":2,\"type\":\"request\",\"id\":\"bad-version\","
        "\"method\":\"pbmp.info\",\"params\":{}}",
        "", 0);
    failed |= check_raw(
        "{\"pbmp\":1,\"type\":\"response\",\"id\":\"bad-type\","
        "\"method\":\"pbmp.info\",\"params\":{}}",
        "", 0);

    networks.count = 2;
    strcpy(networks.networks[0].name, "Libera");
    strcpy(networks.networks[0].pass, "secret-one");
    strcpy(networks.networks[1].name, "OFTC");
    strcpy(networks.networks[1].pass, "secret-two");
    ambnc_pbmp_set_networks(&networks);
    failed |= check("networks.list", "\"id\":\"network-1\",\"name\":\"Libera\",\"state\":\"configured\",\"retry_seconds\":0,\"reconnect_attempts\":0,\"paused\":false}");
    failed |= check("networks.list", "\"id\":\"network-2\",\"name\":\"OFTC\",\"state\":\"configured\"");
    ambnc_network_runtime_increment_reconnect_attempts(0);
    ambnc_network_runtime_increment_reconnect_attempts(0);
    failed |= check("networks.list", "\"name\":\"Libera\",\"state\":\"configured\",\"retry_seconds\":0,\"reconnect_attempts\":2");
    failed |= check("networks.list", "\"name\":\"OFTC\",\"state\":\"configured\",\"retry_seconds\":0,\"reconnect_attempts\":0");
    ambnc_network_runtime_set_paused(1);
    failed |= check("networks.list", "\"name\":\"Libera\",\"state\":\"configured\",\"retry_seconds\":0,\"paused\":true");
    ambnc_network_runtime_set_paused(0);
    failed |= check("networks.list", "\"name\":\"Libera\",\"state\":\"configured\",\"retry_seconds\":0,\"paused\":false");
    ambnc_network_runtime_set(0, AMBNC_NETWORK_RUNTIME_CONNECTING);
    failed |= check("networks.list", "\"name\":\"Libera\",\"state\":\"connecting\"");
    ambnc_network_runtime_set(0, AMBNC_NETWORK_RUNTIME_CONNECTED);
    failed |= check("networks.list", "\"name\":\"Libera\",\"state\":\"connected\"");
    ambnc_network_runtime_set(0, AMBNC_NETWORK_RUNTIME_DISCONNECTED);
    ambnc_network_runtime_set_retry(0, 8);
    failed |= check("networks.list", "\"name\":\"Libera\",\"state\":\"disconnected\",\"retry_seconds\":8");
    ambnc_network_runtime_set(0, AMBNC_NETWORK_RUNTIME_CONFIGURED);
    failed |= check("networks.list", "\"name\":\"Libera\",\"state\":\"configured\"");
    {
        char response[1024];
        const char *request =
            "{\"pbmp\":1,\"type\":\"request\",\"id\":\"secret-check\","
            "\"method\":\"networks.list\",\"params\":{}}";
        if (ambnc_pbmp_handle(request, response, sizeof(response)) < 0 ||
            strstr(response, "secret-one") || strstr(response, "secret-two")) {
            fprintf(stderr, "networks.list exposed a secret or failed\n");
            failed = 1;
        }
    }

    if (failed)
        return 1;
    puts("PASS: AmBNC PBMP Endpoint Profile M0 adapter");
    return 0;
}
