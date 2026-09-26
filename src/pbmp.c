#include <stdio.h>
#include <string.h>

#include "ambnc.h"
#include "pbmp.h"

static int has_method(const char *request, const char *method)
{
    char needle[96];

    if (!request || !method)
        return 0;
    if (snprintf(needle, sizeof(needle), "\"method\":\"%s\"", method) >= (int)sizeof(needle))
        return 0;
    return strstr(request, needle) != 0;
}

static int get_id(const char *request, char *id, size_t id_size)
{
    const char *p;
    const char *end;
    size_t len;

    p = strstr(request, "\"id\":\"");
    if (!p)
        return -1;
    p += 6;
    end = strchr(p, '"');
    if (!end)
        return -1;
    len = (size_t)(end - p);
    if (len == 0 || len >= id_size)
        return -1;
    memcpy(id, p, len);
    id[len] = '\0';
    return 0;
}

int ambnc_pbmp_handle(const char *request, char *response, size_t response_size)
{
    char id[64];
    int written;

    if (!request || !response || response_size == 0 || get_id(request, id, sizeof(id)) != 0)
        return -1;

    if (has_method(request, "pbmp.info")) {
        written = snprintf(response, response_size,
            "{\"pbmp\":1,\"type\":\"response\",\"id\":\"%s\",\"ok\":true,"
            "\"result\":{\"version\":1,\"implementation\":{\"name\":\"%s\",\"version\":\"%s\"}}}\n",
            id, AMBNC_NAME, AMBNC_VERSION);
    } else if (has_method(request, "capabilities.list")) {
        written = snprintf(response, response_size,
            "{\"pbmp\":1,\"type\":\"response\",\"id\":\"%s\",\"ok\":true,"
            "\"result\":{\"capabilities\":[\"endpoint.info\"]}}\n", id);
    } else if (has_method(request, "endpoint.info")) {
        written = snprintf(response, response_size,
            "{\"pbmp\":1,\"type\":\"response\",\"id\":\"%s\",\"ok\":true,"
            "\"result\":{\"endpoint\":{\"id\":\"%s\",\"kind\":\"%s\","
            "\"implementation\":{\"name\":\"%s\",\"version\":\"%s\"},\"state\":\"running\"}}}\n",
            id, AMBNC_PBMP_ENDPOINT_ID, AMBNC_PBMP_ENDPOINT_KIND, AMBNC_NAME, AMBNC_VERSION);
    } else {
        written = snprintf(response, response_size,
            "{\"pbmp\":1,\"type\":\"response\",\"id\":\"%s\",\"ok\":false,"
            "\"error\":{\"code\":\"not_supported\",\"message\":\"method not supported\"}}\n", id);
    }

    if (written < 0 || (size_t)written >= response_size)
        return -1;
    return written;
}
