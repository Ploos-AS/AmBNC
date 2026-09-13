#include <stdio.h>
#include <string.h>

#include "rexx_events.h"

#define AMBNC_HOOK_DIR "REXX:AmBNC"

static struct ambnc_rexx *bound_rexx;
static struct ambnc_rexx_control *bound_control;

void ambnc_rexx_events_bind(struct ambnc_rexx *rexx,
                            struct ambnc_rexx_control *control)
{
    bound_rexx = rexx;
    bound_control = control;
}

void ambnc_rexx_emit_lifecycle(const char *event_name, const char *detail)
{
    if (bound_rexx == 0 || bound_control == 0 || event_name == 0) return;
    (void)ambnc_rexx_run_event(bound_rexx, bound_control, AMBNC_HOOK_DIR,
                               event_name, "", "", detail != 0 ? detail : "");
}

static const char *prefix_nick(const char *line, char *nick, unsigned int size)
{
    const char *p = line;
    unsigned int n = 0;
    if (size != 0) nick[0] = '\0';
    if (p == 0 || *p != ':') return p;
    ++p;
    while (*p != '\0' && *p != '!' && *p != ' ') {
        if (n + 1 < size) nick[n++] = *p;
        ++p;
    }
    if (size != 0) nick[n] = '\0';
    p = strchr(line, ' ');
    return p != 0 ? p + 1 : "";
}

void ambnc_rexx_emit_irc_line(const char *line)
{
    char nick[32];
    char command[16];
    char target[64];
    char event_name[24];
    const char *p;
    const char *text;
    unsigned int n = 0;
    unsigned int t = 0;

    if (bound_rexx == 0 || bound_control == 0 || line == 0) return;
    event_name[0] = '\0';
    p = prefix_nick(line, nick, sizeof(nick));
    while (*p == ' ') ++p;
    while (*p != '\0' && *p != ' ' && n + 1 < sizeof(command)) command[n++] = *p++;
    command[n] = '\0';
    while (*p == ' ') ++p;
    while (*p != '\0' && *p != ' ' && t + 1 < sizeof(target)) target[t++] = *p++;
    target[t] = '\0';
    while (*p == ' ') ++p;
    if (*p == ':') ++p;
    text = p;

    if (strcmp(command, "PRIVMSG") == 0) snprintf(event_name, sizeof(event_name), "ON_PRIVMSG");
    else if (strcmp(command, "NOTICE") == 0) snprintf(event_name, sizeof(event_name), "ON_NOTICE");
    else if (strcmp(command, "JOIN") == 0) snprintf(event_name, sizeof(event_name), "ON_JOIN");
    else if (strcmp(command, "PART") == 0) snprintf(event_name, sizeof(event_name), "ON_PART");
    else if (strcmp(command, "QUIT") == 0) snprintf(event_name, sizeof(event_name), "ON_QUIT");
    else if (strcmp(command, "KICK") == 0) snprintf(event_name, sizeof(event_name), "ON_KICK");
    else if (strcmp(command, "TOPIC") == 0) snprintf(event_name, sizeof(event_name), "ON_TOPIC");
    else if (strcmp(command, "NICK") == 0) snprintf(event_name, sizeof(event_name), "ON_NICK");

    if (event_name[0] != '\0')
        (void)ambnc_rexx_run_event(bound_rexx, bound_control, AMBNC_HOOK_DIR,
                                   event_name, nick, target, text);
}
