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

int main(void)
{
    int failed = 0;

    failed |= check("pbmp.info", "\"version\":1");
    failed |= check("capabilities.list", "\"endpoint.info\"");
    failed |= check("endpoint.info", "\"kind\":\"bouncer\"");
    failed |= check("unsupported.method", "\"code\":\"not_supported\"");

    if (failed)
        return 1;
    puts("PASS: AmBNC PBMP Endpoint Profile M0 adapter");
    return 0;
}
