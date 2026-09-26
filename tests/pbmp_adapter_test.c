#include <stdio.h>
#include <string.h>

#include "pbmp.h"

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

    failed |= check("pbmp.info", "\"version\":1");
    failed |= check("capabilities.list", "\"endpoint.info\"");
    failed |= check("endpoint.info", "\"kind\":\"bouncer\"");
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
        "{\"pbmp\":2,\"type\":\"request\",\"id\":\"bad-version\","
        "\"method\":\"pbmp.info\",\"params\":{}}",
        "", 0);
    failed |= check_raw(
        "{\"pbmp\":1,\"type\":\"response\",\"id\":\"bad-type\","
        "\"method\":\"pbmp.info\",\"params\":{}}",
        "", 0);

    if (failed)
        return 1;
    puts("PASS: AmBNC PBMP Endpoint Profile M0 adapter");
    return 0;
}
