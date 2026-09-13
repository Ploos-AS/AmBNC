#ifndef AMBNC_IRC_H
#define AMBNC_IRC_H

#define AMBNC_IRC_LINE_MAX 510

struct ambnc_irc_framer {
    char line[AMBNC_IRC_LINE_MAX + 1];
    unsigned int length;
    int overflow;
};

typedef void (*ambnc_irc_line_cb)(const char *line, void *userdata);

void ambnc_irc_framer_init(struct ambnc_irc_framer *framer);
void ambnc_irc_framer_feed(struct ambnc_irc_framer *framer,
                           const char *data,
                           unsigned int length,
                           ambnc_irc_line_cb callback,
                           void *userdata);
int ambnc_irc_send_line(int sock, const char *line);
int ambnc_irc_send_registration(int sock,
                                const char *nick,
                                const char *user,
                                const char *pass);
int ambnc_irc_token_valid(const char *text);

#endif
