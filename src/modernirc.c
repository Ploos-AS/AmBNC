#include <stdio.h>
#include <string.h>

#include "irc.h"
#include "modernirc.h"

static int send_line(int sock, const char *line)
{
    return ambnc_irc_send_line(sock, line);
}

static int contains_token(const char *line, const char *token)
{
    return line != 0 && token != 0 && strstr(line, token) != 0;
}

static int b64_encode(const unsigned char *src, unsigned int length,
                      char *dst, unsigned int size)
{
    static const char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    unsigned int i = 0;
    unsigned int o = 0;

    while (i < length) {
        unsigned long v = (unsigned long)src[i++] << 16;
        unsigned int remain = length - (i - 1U);
        if (remain > 1U) v |= (unsigned long)src[i++] << 8;
        if (remain > 2U) v |= (unsigned long)src[i++];
        if (o + 4U >= size) return -1;
        dst[o++] = table[(v >> 18) & 63U];
        dst[o++] = table[(v >> 12) & 63U];
        dst[o++] = remain > 1U ? table[(v >> 6) & 63U] : '=';
        dst[o++] = remain > 2U ? table[v & 63U] : '=';
    }
    if (o >= size) return -1;
    dst[o] = '\0';
    return 0;
}

static int send_sasl_payload(int sock, const struct ambnc_network_config *config)
{
    unsigned char plain[192];
    char encoded[260];
    char line[300];
    unsigned int ulen = (unsigned int)strlen(config->sasl_user);
    unsigned int plen = (unsigned int)strlen(config->sasl_pass);
    unsigned int length;
    int written;

    if (ulen == 0U || plen == 0U) return -1;
    if (ulen * 2U + plen + 2U > sizeof(plain)) return -1;

    memcpy(plain, config->sasl_user, ulen);
    plain[ulen] = 0;
    memcpy(plain + ulen + 1U, config->sasl_user, ulen);
    plain[ulen + 1U + ulen] = 0;
    memcpy(plain + ulen + 2U + ulen, config->sasl_pass, plen);
    length = ulen + 1U + ulen + 1U + plen;

    if (b64_encode(plain, length, encoded, sizeof(encoded)) != 0) return -1;
    written = snprintf(line, sizeof(line), "AUTHENTICATE %s", encoded);
    if (written <= 0 || written >= (int)sizeof(line)) return -1;
    return send_line(sock, line);
}

void ambnc_modernirc_init(struct ambnc_modernirc *state)
{
    memset(state, 0, sizeof(*state));
}

int ambnc_modernirc_start(int sock,
                          const struct ambnc_network_config *config,
                          struct ambnc_modernirc *state)
{
    ambnc_modernirc_init(state);
    if (!config->cap_enabled) return 0;
    state->cap_active = 1;
    return send_line(sock, "CAP LS 302");
}

int ambnc_modernirc_handle_line(int sock,
                                const struct ambnc_network_config *config,
                                struct ambnc_modernirc *state,
                                const char *line)
{
    if (!state->cap_active || state->cap_ended) return 0;

    if (contains_token(line, " CAP ") && contains_token(line, " LS ")) {
        if (config->sasl_plain && contains_token(line, "sasl")) {
            state->sasl_requested = 1;
            return send_line(sock, "CAP REQ :sasl");
        }
        state->cap_ended = 1;
        return send_line(sock, "CAP END");
    }

    if (state->sasl_requested && contains_token(line, " CAP ") &&
        contains_token(line, " ACK ") && contains_token(line, "sasl"))
        return send_line(sock, "AUTHENTICATE PLAIN");

    if (state->sasl_requested && strncmp(line, "AUTHENTICATE +", 14) == 0)
        return send_sasl_payload(sock, config);

    if (contains_token(line, " 903 ")) {
        state->sasl_complete = 1;
        state->cap_ended = 1;
        return send_line(sock, "CAP END");
    }

    if (contains_token(line, " 904 ") || contains_token(line, " 905 ") ||
        contains_token(line, " 906 ") || contains_token(line, " 907 ") ||
        (contains_token(line, " CAP ") && contains_token(line, " NAK ") &&
         contains_token(line, "sasl"))) {
        state->cap_ended = 1;
        (void)send_line(sock, "CAP END");
        return -1;
    }

    return 0;
}
