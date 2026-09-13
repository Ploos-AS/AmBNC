#include <stdio.h>
#include <string.h>

#include "irc.h"
#include "net.h"

int ambnc_irc_token_valid(const char *text)
{
    const unsigned char *p = (const unsigned char *)text;
    if (text == 0 || text[0] == '\0') return 0;
    while (*p != '\0') {
        if (*p == '\r' || *p == '\n' || *p == ' ') return 0;
        ++p;
    }
    return 1;
}

void ambnc_irc_framer_init(struct ambnc_irc_framer *framer)
{
    framer->length = 0;
    framer->overflow = 0;
    framer->line[0] = '\0';
}

void ambnc_irc_framer_feed(struct ambnc_irc_framer *framer,
                           const char *data,
                           unsigned int length,
                           ambnc_irc_line_cb callback,
                           void *userdata)
{
    unsigned int i;
    for (i = 0; i < length; ++i) {
        char ch = data[i];
        if (ch == '\n') {
            if (!framer->overflow && framer->length > 0 &&
                framer->line[framer->length - 1] == '\r')
                --framer->length;
            if (!framer->overflow) {
                framer->line[framer->length] = '\0';
                callback(framer->line, userdata);
            }
            framer->length = 0;
            framer->overflow = 0;
            framer->line[0] = '\0';
        } else if (!framer->overflow) {
            if (framer->length < AMBNC_IRC_LINE_MAX) {
                framer->line[framer->length++] = ch;
            } else {
                framer->overflow = 1;
            }
        }
    }
}

int ambnc_irc_send_line(int sock, const char *line)
{
    unsigned int length;
    char wire[AMBNC_IRC_LINE_MAX + 3];

    if (line == 0) return -1;
    length = (unsigned int)strlen(line);
    if (length > AMBNC_IRC_LINE_MAX) return -1;
    memcpy(wire, line, length);
    wire[length++] = '\r';
    wire[length++] = '\n';
    return ambnc_net_send_all(sock, wire, length);
}

int ambnc_irc_send_registration(int sock,
                                const char *nick,
                                const char *user,
                                const char *pass)
{
    char line[AMBNC_IRC_LINE_MAX + 1];
    int written;

    if (!ambnc_irc_token_valid(nick) || !ambnc_irc_token_valid(user)) return -1;
    if (pass != 0 && pass[0] != '\0') {
        if (strchr(pass, '\r') != 0 || strchr(pass, '\n') != 0) return -1;
        written = snprintf(line, sizeof(line), "PASS %s", pass);
        if (written <= 0 || written >= (int)sizeof(line) || ambnc_irc_send_line(sock, line) != 0)
            return -1;
    }

    written = snprintf(line, sizeof(line), "NICK %s", nick);
    if (written <= 0 || written >= (int)sizeof(line) || ambnc_irc_send_line(sock, line) != 0)
        return -1;

    written = snprintf(line, sizeof(line), "USER %s 0 * :%s", user, user);
    if (written <= 0 || written >= (int)sizeof(line) || ambnc_irc_send_line(sock, line) != 0)
        return -1;
    return 0;
}
