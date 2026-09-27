#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "ambnc.h"
#include "networks.h"
#include "pbmp.h"

static const struct ambnc_networks_config *pbmp_networks;

void ambnc_pbmp_set_networks(const struct ambnc_networks_config *networks)
{
    pbmp_networks = networks;
}

static const char *skip_ws(const char *p)
{
    while (p && *p && isspace((unsigned char)*p))
        ++p;
    return p;
}

static const char *find_value(const char *json, const char *key)
{
    const char *p;
    size_t key_len;

    if (!json || !key)
        return 0;
    key_len = strlen(key);
    p = skip_ws(json);
    if (*p++ != '{')
        return 0;

    for (;;) {
        const char *name;
        size_t name_len = 0;
        int escaped = 0;

        p = skip_ws(p);
        if (*p == '}')
            return 0;
        if (*p++ != '"')
            return 0;
        name = p;
        while (*p) {
            if (!escaped && *p == '"')
                break;
            if (!escaped && *p == '\\')
                escaped = 1;
            else
                escaped = 0;
            ++p;
            ++name_len;
        }
        if (*p++ != '"')
            return 0;
        p = skip_ws(p);
        if (*p++ != ':')
            return 0;
        p = skip_ws(p);
        if (name_len == key_len && strncmp(name, key, key_len) == 0)
            return p;

        if (*p == '"') {
            ++p;
            escaped = 0;
            while (*p) {
                if (!escaped && *p == '"') { ++p; break; }
                if (!escaped && *p == '\\') escaped = 1;
                else escaped = 0;
                ++p;
            }
        } else {
            int depth = 0;
            while (*p) {
                if ((*p == ',' || *p == '}') && depth == 0)
                    break;
                if (*p == '{' || *p == '[') ++depth;
                else if (*p == '}' || *p == ']') --depth;
                ++p;
            }
        }
        p = skip_ws(p);
        if (*p == ',') { ++p; continue; }
        if (*p == '}') return 0;
        return 0;
    }
}

static int unique_top_level_key(const char *json, const char *key)
{
    const char *p = skip_ws(json);
    size_t key_len = strlen(key);
    int matches = 0;

    if (!p || *p++ != '{') return 0;
    for (;;) {
        const char *name;
        size_t name_len = 0;
        int escaped = 0;
        int depth = 0;

        p = skip_ws(p);
        if (*p == '}') break;
        if (*p++ != '"') return 0;
        name = p;
        while (*p) {
            if (!escaped && *p == '"') break;
            if (!escaped && *p == '\\') escaped = 1; else escaped = 0;
            ++p; ++name_len;
        }
        if (*p++ != '"') return 0;
        p = skip_ws(p);
        if (*p++ != ':') return 0;
        p = skip_ws(p);
        if (name_len == key_len && strncmp(name, key, key_len) == 0)
            ++matches;

        if (*p == '"') {
            ++p; escaped = 0;
            while (*p) {
                if (!escaped && *p == '"') { ++p; break; }
                if (!escaped && *p == '\\') escaped = 1; else escaped = 0;
                ++p;
            }
        } else {
            while (*p) {
                if ((*p == ',' || *p == '}') && depth == 0) break;
                if (*p == '{' || *p == '[') ++depth;
                else if (*p == '}' || *p == ']') --depth;
                ++p;
            }
        }
        p = skip_ws(p);
        if (*p == ',') { ++p; continue; }
        if (*p == '}') break;
        return 0;
    }
    return matches == 1;
}

static int hex4(const char *p, unsigned long *value)
{
    unsigned int i;
    unsigned long v = 0;
    for (i = 0; i < 4; ++i) {
        unsigned char ch = (unsigned char)p[i];
        v <<= 4;
        if (ch >= '0' && ch <= '9') v |= ch - '0';
        else if (ch >= 'a' && ch <= 'f') v |= ch - 'a' + 10;
        else if (ch >= 'A' && ch <= 'F') v |= ch - 'A' + 10;
        else return -1;
    }
    *value = v;
    return 0;
}

static int append_utf8(unsigned long cp, char *out, size_t out_size, size_t *n)
{
    unsigned int need;
    if (cp <= 0x7fUL) need = 1;
    else if (cp <= 0x7ffUL) need = 2;
    else if (cp <= 0xffffUL) need = 3;
    else if (cp <= 0x10ffffUL) need = 4;
    else return -1;
    if (*n + need >= out_size) return -1;
    if (need == 1) out[(*n)++] = (char)cp;
    else if (need == 2) {
        out[(*n)++] = (char)(0xc0UL | (cp >> 6));
        out[(*n)++] = (char)(0x80UL | (cp & 0x3fUL));
    } else if (need == 3) {
        out[(*n)++] = (char)(0xe0UL | (cp >> 12));
        out[(*n)++] = (char)(0x80UL | ((cp >> 6) & 0x3fUL));
        out[(*n)++] = (char)(0x80UL | (cp & 0x3fUL));
    } else {
        out[(*n)++] = (char)(0xf0UL | (cp >> 18));
        out[(*n)++] = (char)(0x80UL | ((cp >> 12) & 0x3fUL));
        out[(*n)++] = (char)(0x80UL | ((cp >> 6) & 0x3fUL));
        out[(*n)++] = (char)(0x80UL | (cp & 0x3fUL));
    }
    return 0;
}

static int string_value(const char *json, const char *key, char *out, size_t out_size)
{
    const char *p = find_value(json, key);
    size_t n = 0;

    if (!p || *p++ != '"' || out_size == 0) return -1;
    while (*p && *p != '"') {
        unsigned char ch = (unsigned char)*p++;
        if (ch == '\\') {
            unsigned long cp;
            ch = (unsigned char)*p++;
            if (ch == '"' || ch == '\\' || ch == '/') ;
            else if (ch == 'b') ch = '\b';
            else if (ch == 'f') ch = '\f';
            else if (ch == 'n') ch = '\n';
            else if (ch == 'r') ch = '\r';
            else if (ch == 't') ch = '\t';
            else if (ch == 'u') {
                unsigned long low;
                if (hex4(p, &cp) != 0) return -1;
                p += 4;
                if (cp >= 0xd800UL && cp <= 0xdbffUL) {
                    if (p[0] != '\\' || p[1] != 'u' || hex4(p + 2, &low) != 0 ||
                        low < 0xdc00UL || low > 0xdfffUL) return -1;
                    p += 6;
                    cp = 0x10000UL + ((cp - 0xd800UL) << 10) + (low - 0xdc00UL);
                } else if (cp >= 0xdc00UL && cp <= 0xdfffUL) return -1;
                if (append_utf8(cp, out, out_size, &n) != 0) return -1;
                continue;
            } else return -1;
        }
        if (ch < 0x20 || n + 1 >= out_size) return -1;
        out[n++] = (char)ch;
    }
    if (*p != '"' || n == 0) return -1;
    out[n] = '\0';
    return 0;
}

static int literal_value(const char *json, const char *key, const char *value)
{
    const char *p = find_value(json, key);
    size_t n = strlen(value);
    if (!p || strncmp(p, value, n) != 0)
        return 0;
    p += n;
    return !isalnum((unsigned char)*p) && *p != '_' && *p != '.';
}

static int json_escape(const char *src, char *dst, size_t dst_size)
{
    size_t n = 0;
    unsigned char ch;

    while ((ch = (unsigned char)*src++) != 0) {
        const char *esc = 0;
        if (ch == '"') esc = "\\\"";
        else if (ch == '\\') esc = "\\\\";
        else if (ch == '\b') esc = "\\b";
        else if (ch == '\f') esc = "\\f";
        else if (ch == '\n') esc = "\\n";
        else if (ch == '\r') esc = "\\r";
        else if (ch == '\t') esc = "\\t";
        if (esc) {
            if (n + 2 >= dst_size) return -1;
            dst[n++] = esc[0];
            dst[n++] = esc[1];
        } else {
            if (ch < 0x20 || n + 1 >= dst_size) return -1;
            dst[n++] = (char)ch;
        }
    }
    if (n >= dst_size) return -1;
    dst[n] = '\0';
    return 0;
}

static const char *runtime_state_name(int state)
{
    if (state == AMBNC_NETWORK_RUNTIME_CONNECTING) return "connecting";
    if (state == AMBNC_NETWORK_RUNTIME_CONNECTED) return "connected";
    if (state == AMBNC_NETWORK_RUNTIME_DISCONNECTED) return "disconnected";
    return "configured";
}

static int complete_json_object(const char *json)
{
    const char *p = skip_ws(json);
    int depth = 0;
    int in_string = 0;
    int escaped = 0;

    if (!p || *p != '{') return 0;
    for (; *p; ++p) {
        if (in_string) {
            if (!escaped && *p == '"') in_string = 0;
            if (!escaped && *p == '\\') escaped = 1; else escaped = 0;
            continue;
        }
        if (*p == '"') { in_string = 1; escaped = 0; continue; }
        if (*p == '{' || *p == '[') ++depth;
        else if (*p == '}' || *p == ']') {
            --depth;
            if (depth < 0) return 0;
            if (depth == 0) {
                p = skip_ws(p + 1);
                return *p == '\0';
            }
        }
    }
    return 0;
}

int ambnc_pbmp_handle(const char *request, char *response, size_t response_size)
{
    char id[64], escaped_id[128], method[96], type[32];
    int written;

    if (!request || !response || response_size == 0)
        return -1;
    if (!complete_json_object(request))
        return -1;
    if (!unique_top_level_key(request, "pbmp") ||
        !unique_top_level_key(request, "type") ||
        !unique_top_level_key(request, "id") ||
        !unique_top_level_key(request, "method"))
        return -1;
    if (!literal_value(request, "pbmp", "1"))
        return -1;
    if (string_value(request, "type", type, sizeof(type)) != 0 || strcmp(type, "request") != 0)
        return -1;
    if (string_value(request, "id", id, sizeof(id)) != 0 ||
        json_escape(id, escaped_id, sizeof(escaped_id)) != 0 ||
        string_value(request, "method", method, sizeof(method)) != 0)
        return -1;

    if (strcmp(method, "pbmp.info") == 0) {
        written = snprintf(response, response_size,
            "{\"pbmp\":1,\"type\":\"response\",\"id\":\"%s\",\"ok\":true,"
            "\"result\":{\"version\":1,\"implementation\":{\"name\":\"%s\",\"version\":\"%s\"}}}\n",
            escaped_id, AMBNC_NAME, AMBNC_VERSION);
    } else if (strcmp(method, "capabilities.list") == 0) {
        written = snprintf(response, response_size,
            "{\"pbmp\":1,\"type\":\"response\",\"id\":\"%s\",\"ok\":true,"
            "\"result\":{\"capabilities\":[\"endpoint.info\",\"networks.list\"]}}\n", escaped_id);
    } else if (strcmp(method, "endpoint.info") == 0) {
        written = snprintf(response, response_size,
            "{\"pbmp\":1,\"type\":\"response\",\"id\":\"%s\",\"ok\":true,"
            "\"result\":{\"endpoint\":{\"id\":\"%s\",\"kind\":\"%s\","
            "\"implementation\":{\"name\":\"%s\",\"version\":\"%s\"},\"state\":\"running\"}}}\n",
            escaped_id, AMBNC_PBMP_ENDPOINT_ID, AMBNC_PBMP_ENDPOINT_KIND, AMBNC_NAME, AMBNC_VERSION);
    } else if (strcmp(method, "networks.list") == 0) {
        size_t used;
        unsigned int i;
        written = snprintf(response, response_size,
            "{\"pbmp\":1,\"type\":\"response\",\"id\":\"%s\",\"ok\":true,"
            "\"result\":{\"networks\":[", escaped_id);
        if (written < 0 || (size_t)written >= response_size)
            return -1;
        used = (size_t)written;
        if (pbmp_networks != 0) {
            for (i = 0; i < pbmp_networks->count; ++i) {
                char name[AMBNC_NETWORK_NAME_MAX * 2 + 1];
                const struct ambnc_network_config *n = &pbmp_networks->networks[i];
                if (json_escape(n->name, name, sizeof(name)) != 0)
                    return -1;
                written = snprintf(response + used, response_size - used,
                    "%s{\"id\":\"network-%u\",\"name\":\"%s\",\"state\":\"%s\",\"retry_seconds\":%u}",
                    i ? "," : "", i + 1U, name,
                    runtime_state_name(ambnc_network_runtime_get(i)),
                    ambnc_network_runtime_get_retry(i));
                if (written < 0 || (size_t)written >= response_size - used)
                    return -1;
                used += (size_t)written;
            }
        }
        written = snprintf(response + used, response_size - used, "]}}\n");
        if (written < 0 || (size_t)written >= response_size - used)
            return -1;
        return (int)(used + (size_t)written);
    } else {
        written = snprintf(response, response_size,
            "{\"pbmp\":1,\"type\":\"response\",\"id\":\"%s\",\"ok\":false,"
            "\"error\":{\"code\":\"not_supported\",\"message\":\"method not supported\"}}\n", escaped_id);
    }

    if (written < 0 || (size_t)written >= response_size)
        return -1;
    return written;
}
