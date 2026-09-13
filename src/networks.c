#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "networks.h"
#undef ambnc_irc_send_registration
#undef ambnc_state_observe_line

#include "irc.h"
#include "modernirc.h"
#include "modernirc.c"

struct ambnc_m7_slot {
    int sock;
    const struct ambnc_network_config *config;
    struct ambnc_modernirc modern;
};

static const struct ambnc_networks_config *active_config;
static struct ambnc_m7_slot m7_slots[AMBNC_NETWORKS_MAX];

static char *trim(char *s)
{
    char *end;
    while (*s != '\0' && isspace((unsigned char)*s)) ++s;
    end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) --end;
    *end = '\0';
    return s;
}

static void copy_bounded(char *dst, unsigned int size, const char *src)
{
    unsigned int i = 0;
    if (size == 0) return;
    while (src != 0 && src[i] != '\0' && i + 1 < size) {
        dst[i] = src[i];
        ++i;
    }
    dst[i] = '\0';
}

static int same_ci(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        if (toupper((unsigned char)*a) != toupper((unsigned char)*b)) return 0;
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

static int set_number(unsigned long *out, const char *value, unsigned long minv, unsigned long maxv)
{
    char *end = 0;
    unsigned long v = strtoul(value, &end, 10);
    if (value == end || end == 0 || *trim(end) != '\0' || v < minv || v > maxv) return -1;
    *out = v;
    return 0;
}

static int set_bool(int *out, const char *value)
{
    if (same_ci(value, "YES") || same_ci(value, "TRUE") ||
        same_ci(value, "ON") || strcmp(value, "1") == 0) {
        *out = 1;
        return 0;
    }
    if (same_ci(value, "NO") || same_ci(value, "FALSE") ||
        same_ci(value, "OFF") || strcmp(value, "0") == 0) {
        *out = 0;
        return 0;
    }
    return -1;
}

static const struct ambnc_network_config *find_config(const char *nick,
                                                       const char *user)
{
    unsigned int i;
    if (active_config == 0) return 0;
    for (i = 0; i < active_config->count; ++i) {
        const struct ambnc_network_config *n = &active_config->networks[i];
        if (same_ci(n->nick, nick) && same_ci(n->user, user)) return n;
    }
    return 0;
}

static struct ambnc_m7_slot *slot_for_config(const struct ambnc_network_config *config)
{
    unsigned int i;
    for (i = 0; i < AMBNC_NETWORKS_MAX; ++i)
        if (m7_slots[i].config == config) return &m7_slots[i];
    for (i = 0; i < AMBNC_NETWORKS_MAX; ++i) {
        if (m7_slots[i].config == 0) {
            m7_slots[i].config = config;
            m7_slots[i].sock = -1;
            ambnc_modernirc_init(&m7_slots[i].modern);
            return &m7_slots[i];
        }
    }
    return 0;
}

void ambnc_networks_init(struct ambnc_networks_config *config)
{
    memset(config, 0, sizeof(*config));
    memset(m7_slots, 0, sizeof(m7_slots));
    active_config = config;
}

int ambnc_networks_load(struct ambnc_networks_config *config, const char *path,
                        char *error, unsigned int error_size)
{
    FILE *fp;
    char line[512];
    unsigned int lineno = 0;
    struct ambnc_network_config *current = 0;

    ambnc_networks_init(config);
    fp = fopen(path, "r");
    if (fp == 0) {
        snprintf(error, error_size, "cannot open config");
        return -1;
    }

    while (fgets(line, sizeof(line), fp) != 0) {
        char *text;
        char *eq;
        ++lineno;
        text = trim(line);
        if (*text == '\0' || *text == '#' || *text == ';') continue;

        if (*text == '[') {
            char name[AMBNC_NETWORK_NAME_MAX + 1];
            char *close = strchr(text, ']');
            char *p;
            unsigned int n = 0;
            if (close == 0) goto bad_line;
            *close = '\0';
            p = trim(text + 1);
            if (strncmp(p, "NETWORK ", 8) != 0) goto bad_line;
            p = trim(p + 8);
            while (*p != '\0' && n + 1 < sizeof(name)) name[n++] = *p++;
            name[n] = '\0';
            if (name[0] == '\0' || config->count >= AMBNC_NETWORKS_MAX) goto bad_line;
            current = &config->networks[config->count++];
            memset(current, 0, sizeof(*current));
            copy_bounded(current->name, sizeof(current->name), name);
            current->port = 6667;
            current->listen_port = (unsigned short)(16667U + config->count - 1U);
            current->backlog_lines = AMBNC_STATE_RING_LINES_DEFAULT;
            current->tls_mode = AMBNC_TLS_PLAIN;
            continue;
        }

        if (current == 0) goto bad_line;
        eq = strchr(text, '=');
        if (eq == 0) goto bad_line;
        *eq++ = '\0';
        text = trim(text);
        eq = trim(eq);

        if (same_ci(text, "HOST")) copy_bounded(current->host, sizeof(current->host), eq);
        else if (same_ci(text, "NICK")) copy_bounded(current->nick, sizeof(current->nick), eq);
        else if (same_ci(text, "USER")) copy_bounded(current->user, sizeof(current->user), eq);
        else if (same_ci(text, "PASS")) copy_bounded(current->pass, sizeof(current->pass), eq);
        else if (same_ci(text, "CAP")) {
            if (set_bool(&current->cap_enabled, eq) != 0) goto bad_line;
        } else if (same_ci(text, "SASL_PLAIN")) {
            if (set_bool(&current->sasl_plain, eq) != 0) goto bad_line;
        } else if (same_ci(text, "TLS_MODE")) {
            if (same_ci(eq, "PLAIN")) current->tls_mode = AMBNC_TLS_PLAIN;
            else if (same_ci(eq, "PROXY")) current->tls_mode = AMBNC_TLS_PROXY;
            else goto bad_line;
        } else if (same_ci(text, "PORT")) {
            unsigned long v;
            if (set_number(&v, eq, 1, 65535) != 0) goto bad_line;
            current->port = (unsigned short)v;
        } else if (same_ci(text, "LISTEN_PORT")) {
            unsigned long v;
            if (set_number(&v, eq, 1, 65535) != 0) goto bad_line;
            current->listen_port = (unsigned short)v;
        } else if (same_ci(text, "BACKLOG_LINES")) {
            unsigned long v;
            if (set_number(&v, eq, 1, AMBNC_STATE_RING_LINES_MAX) != 0) goto bad_line;
            current->backlog_lines = (unsigned int)v;
        } else goto bad_line;
    }

    fclose(fp);
    if (config->count == 0) {
        snprintf(error, error_size, "no NETWORK sections");
        return -1;
    }
    {
        unsigned int i;
        for (i = 0; i < config->count; ++i) {
            struct ambnc_network_config *n = &config->networks[i];
            if (n->host[0] == '\0' || n->nick[0] == '\0') {
                snprintf(error, error_size, "network %s requires HOST and NICK", n->name);
                return -1;
            }
            if (n->user[0] == '\0') copy_bounded(n->user, sizeof(n->user), n->nick);
            if (n->sasl_plain) {
                n->cap_enabled = 1;
                if (n->pass[0] == '\0') {
                    snprintf(error, error_size, "network %s SASL requires PASS", n->name);
                    return -1;
                }
            }
        }
    }
    return 0;

bad_line:
    fclose(fp);
    snprintf(error, error_size, "invalid config line %u", lineno);
    return -1;
}

int ambnc_m7_send_registration(int sock,
                               const char *nick,
                               const char *user,
                               const char *pass)
{
    const struct ambnc_network_config *config = find_config(nick, user);
    struct ambnc_m7_slot *slot;

    if (config == 0) return ambnc_irc_send_registration(sock, nick, user, pass);
    slot = slot_for_config(config);
    if (slot == 0) return -1;
    slot->sock = sock;

    if (config->tls_mode == AMBNC_TLS_PROXY)
        printf("AmBNC[%s]: TLS expected from external proxy/terminator\n", config->name);

    if (ambnc_modernirc_start(sock, config, &slot->modern) != 0) return -1;
    return ambnc_irc_send_registration(sock, nick, user, pass);
}

void ambnc_m7_state_observe_line(struct ambnc_session_state *state,
                                 const char *line,
                                 int downstream_attached)
{
    unsigned int i;
    if (active_config != 0) {
        for (i = 0; i < active_config->count; ++i) {
            const struct ambnc_network_config *config = &active_config->networks[i];
            struct ambnc_m7_slot *slot;
            if (!same_ci(state->nick, config->nick)) continue;
            slot = slot_for_config(config);
            if (slot != 0 && slot->sock >= 0) {
                int rc = ambnc_modernirc_handle_line(slot->sock, config, &slot->modern, line);
                if (rc < 0)
                    printf("AmBNC[%s]: CAP/SASL negotiation failed\n", config->name);
            }
            break;
        }
    }
    ambnc_state_observe_line(state, line, downstream_attached);
}
