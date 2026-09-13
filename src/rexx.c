#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <exec/libraries.h>
#include <exec/ports.h>
#include <proto/exec.h>
#include <proto/rexxsyslib.h>
#include <rexx/rxslib.h>
#include <rexx/storage.h>

#include "ambnc.h"
#include "irc.h"
#include "rexx.h"

#define AMBNC_REXX_SCRIPT_COMMAND_MAX 1024

struct RxsLib *RexxSysBase = 0;
static char ambnc_rexx_port_name[] = AMBNC_REXX_PORT;

static const char *skip_space(const char *p)
{
    while (p != 0 && *p != '\0' && isspace((unsigned char)*p)) ++p;
    return p;
}

static const char *next_word(const char *p, char *word, unsigned int size)
{
    unsigned int n = 0;
    p = skip_space(p);
    if (p == 0) return 0;
    while (*p != '\0' && !isspace((unsigned char)*p)) {
        if (n + 1 < size) word[n++] = *p;
        ++p;
    }
    if (size != 0) word[n] = '\0';
    return skip_space(p);
}

static void uppercase(char *text)
{
    while (*text != '\0') {
        *text = (char)toupper((unsigned char)*text);
        ++text;
    }
}

static int send_raw(struct ambnc_rexx_control *control,
                    const char *line,
                    char *result,
                    unsigned int size)
{
    if (!control->upstream_connected || control->upstream_sock < 0) {
        snprintf(result, size, "NOT CONNECTED");
        return 10;
    }
    line = skip_space(line);
    if (line == 0 || line[0] == '\0') {
        snprintf(result, size, "MISSING ARGUMENTS");
        return 10;
    }
    if (ambnc_irc_send_line(control->upstream_sock, line) != 0) {
        snprintf(result, size, "SEND FAILED");
        return 20;
    }
    snprintf(result, size, "OK");
    return 0;
}

static int send_command(struct ambnc_rexx_control *control,
                        const char *command,
                        const char *args,
                        char *result,
                        unsigned int size)
{
    char line[AMBNC_IRC_LINE_MAX + 1];
    int written;
    args = skip_space(args);
    if (args == 0 || args[0] == '\0') {
        snprintf(result, size, "MISSING ARGUMENTS");
        return 10;
    }
    written = snprintf(line, sizeof(line), "%s %s", command, args);
    if (written <= 0 || written >= (int)sizeof(line)) {
        snprintf(result, size, "LINE TOO LONG");
        return 10;
    }
    return send_raw(control, line, result, size);
}

static int send_target(struct ambnc_rexx_control *control,
                       const char *command,
                       const char *args,
                       char *result,
                       unsigned int size)
{
    char target[64];
    char line[AMBNC_IRC_LINE_MAX + 1];
    const char *text = next_word(args, target, sizeof(target));
    int written;
    if (target[0] == '\0') {
        snprintf(result, size, "MISSING TARGET");
        return 10;
    }
    text = skip_space(text);
    written = snprintf(line, sizeof(line), "%s %s :%s", command, target,
                       text != 0 ? text : "");
    if (written <= 0 || written >= (int)sizeof(line)) {
        snprintf(result, size, "LINE TOO LONG");
        return 10;
    }
    return send_raw(control, line, result, size);
}

static int dispatch(struct ambnc_rexx_control *control,
                    const char *input,
                    char *result,
                    unsigned int size)
{
    char command[16];
    const char *args = next_word(input, command, sizeof(command));
    uppercase(command);

    if (command[0] == '\0') {
        snprintf(result, size, "EMPTY COMMAND");
        return 10;
    }
    if (strcmp(command, "STATUS") == 0) {
        snprintf(result, size,
                 "upstream=%s downstream=%s channels=%u buffers=%u lines=%lu",
                 control->upstream_connected ? "CONNECTED" : "DISCONNECTED",
                 control->downstream_attached ? "ATTACHED" : "DETACHED",
                 control->channel_count, control->ring_count, control->buffered_lines);
        return 0;
    }
    if (strcmp(command, "CONNECT") == 0) {
        control->request_reconnect = 1;
        snprintf(result, size, "CONNECT REQUESTED");
        return 0;
    }
    if (strcmp(command, "DISCONNECT") == 0) {
        control->request_disconnect = 1;
        snprintf(result, size, "DISCONNECT REQUESTED");
        return 0;
    }
    if (strcmp(command, "RELOAD") == 0) {
        control->request_reload = 1;
        snprintf(result, size, "RELOAD REQUESTED");
        return 0;
    }
    if (strcmp(command, "QUIT") == 0) {
        control->request_quit = 1;
        snprintf(result, size, "QUIT REQUESTED");
        return 0;
    }
    if (strcmp(command, "RAW") == 0)
        return send_raw(control, args, result, size);
    if (strcmp(command, "JOIN") == 0)
        return send_command(control, "JOIN", args, result, size);
    if (strcmp(command, "PART") == 0)
        return send_target(control, "PART", args, result, size);
    if (strcmp(command, "MSG") == 0)
        return send_target(control, "PRIVMSG", args, result, size);
    if (strcmp(command, "NOTICE") == 0)
        return send_target(control, "NOTICE", args, result, size);
    if (strcmp(command, "BUFFER") == 0) {
        snprintf(result, size, "targets=%u lines=%lu", control->ring_count,
                 control->buffered_lines);
        return 0;
    }

    snprintf(result, size, "UNKNOWN COMMAND");
    return 10;
}

int ambnc_rexx_open(struct ambnc_rexx *rexx)
{
    rexx->port = 0;
    RexxSysBase = (struct RxsLib *)OpenLibrary((STRPTR)"rexxsyslib.library", 0);
    if (RexxSysBase == 0) return -1;
    rexx->port = CreateMsgPort();
    if (rexx->port == 0) {
        CloseLibrary((struct Library *)RexxSysBase);
        RexxSysBase = 0;
        return -1;
    }
    rexx->port->mp_Node.ln_Name = ambnc_rexx_port_name;
    rexx->port->mp_Node.ln_Pri = 0;
    Forbid();
    if (FindPort((STRPTR)ambnc_rexx_port_name) != 0) {
        Permit();
        DeleteMsgPort(rexx->port);
        rexx->port = 0;
        CloseLibrary((struct Library *)RexxSysBase);
        RexxSysBase = 0;
        return -1;
    }
    AddPort(rexx->port);
    Permit();
    return 0;
}

void ambnc_rexx_close(struct ambnc_rexx *rexx)
{
    if (rexx->port != 0) {
        struct Message *message;
        Forbid();
        RemPort(rexx->port);
        Permit();
        while ((message = GetMsg(rexx->port)) != 0) ReplyMsg(message);
        DeleteMsgPort(rexx->port);
        rexx->port = 0;
    }
    if (RexxSysBase != 0) {
        CloseLibrary((struct Library *)RexxSysBase);
        RexxSysBase = 0;
    }
}

unsigned long ambnc_rexx_signal_mask(const struct ambnc_rexx *rexx)
{
    return rexx->port != 0 ? (1UL << rexx->port->mp_SigBit) : 0;
}

void ambnc_rexx_process(struct ambnc_rexx *rexx, struct ambnc_rexx_control *control)
{
    struct RexxMsg *message;
    while (rexx->port != 0 && (message = (struct RexxMsg *)GetMsg(rexx->port)) != 0) {
        char result[AMBNC_REXX_RESULT_MAX];
        const char *input = message->rm_Args[0] != 0 ? (const char *)message->rm_Args[0] : "";
        int rc = dispatch(control, input, result, sizeof(result));
        message->rm_Result1 = rc;
        message->rm_Result2 = 0;
        if ((message->rm_Action & RXFF_RESULT) != 0)
            message->rm_Result2 = (LONG)CreateArgstring((UBYTE *)result, (LONG)strlen(result));
        ReplyMsg((struct Message *)message);
    }
}

static int append_text(char *dst, unsigned int size, unsigned int *length, const char *src)
{
    while (*src != '\0') {
        if (*length + 1 >= size) return -1;
        dst[(*length)++] = *src++;
    }
    dst[*length] = '\0';
    return 0;
}

static int append_quoted(char *dst, unsigned int size, unsigned int *length, const char *src)
{
    if (append_text(dst, size, length, " \"") != 0) return -1;
    while (src != 0 && *src != '\0') {
        char ch = *src++;
        if (ch == '\r' || ch == '\n') ch = ' ';
        if (ch == '"') ch = '\'';
        if (*length + 1 >= size) return -1;
        dst[(*length)++] = ch;
        dst[*length] = '\0';
    }
    return append_text(dst, size, length, "\"");
}

int ambnc_rexx_run_event(struct ambnc_rexx *rexx,
                         struct ambnc_rexx_control *control,
                         const char *hook_dir,
                         const char *event_name,
                         const char *nick,
                         const char *target,
                         const char *text)
{
    struct MsgPort *master;
    struct MsgPort *reply;
    struct RexxMsg *message;
    char path[256];
    char command[AMBNC_REXX_SCRIPT_COMMAND_MAX];
    unsigned int length = 0;
    unsigned long reply_mask;
    unsigned long ambnc_mask;
    int done = 0;
    LONG rc;

    if (RexxSysBase == 0 || hook_dir == 0 || event_name == 0) return 20;
    if (snprintf(path, sizeof(path), "%s/%s.rexx", hook_dir, event_name) >= (int)sizeof(path))
        return 10;
    master = FindPort((STRPTR)"REXX");
    if (master == 0) return 20;
    reply = CreateMsgPort();
    if (reply == 0) return 20;
    message = CreateRexxMsg(reply, (UBYTE *)".ambnc", (UBYTE *)AMBNC_REXX_PORT);
    if (message == 0) {
        DeleteMsgPort(reply);
        return 20;
    }
    command[0] = '\0';
    if (append_text(command, sizeof(command), &length, path) != 0 ||
        append_quoted(command, sizeof(command), &length, event_name) != 0 ||
        append_quoted(command, sizeof(command), &length, nick != 0 ? nick : "") != 0 ||
        append_quoted(command, sizeof(command), &length, target != 0 ? target : "") != 0 ||
        append_quoted(command, sizeof(command), &length, text != 0 ? text : "") != 0) {
        DeleteRexxMsg(message);
        DeleteMsgPort(reply);
        return 10;
    }
    message->rm_Args[0] = CreateArgstring((UBYTE *)command, (LONG)strlen(command));
    if (message->rm_Args[0] == 0) {
        DeleteRexxMsg(message);
        DeleteMsgPort(reply);
        return 20;
    }
    message->rm_Action = RXCOMM | RXFF_RESULT;
    PutMsg(master, (struct Message *)message);
    reply_mask = 1UL << reply->mp_SigBit;
    ambnc_mask = ambnc_rexx_signal_mask(rexx);
    while (!done) {
        unsigned long signals = Wait(reply_mask | ambnc_mask);
        if ((signals & ambnc_mask) != 0) ambnc_rexx_process(rexx, control);
        if ((signals & reply_mask) != 0 && GetMsg(reply) != 0) done = 1;
    }
    rc = message->rm_Result1;
    DeleteArgstring(message->rm_Args[0]);
    if (message->rm_Result2 != 0) DeleteArgstring((UBYTE *)message->rm_Result2);
    DeleteRexxMsg(message);
    DeleteMsgPort(reply);
    return (int)rc;
}
