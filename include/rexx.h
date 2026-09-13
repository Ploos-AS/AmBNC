#ifndef AMBNC_REXX_H
#define AMBNC_REXX_H

#include <exec/ports.h>

#define AMBNC_REXX_RESULT_MAX 256

struct ambnc_rexx_control {
    int upstream_sock;
    int upstream_connected;
    int downstream_attached;
    unsigned int channel_count;
    unsigned int ring_count;
    unsigned long buffered_lines;
    int request_disconnect;
    int request_reconnect;
    int request_reload;
    int request_quit;
};

struct ambnc_rexx {
    struct MsgPort *port;
};

int ambnc_rexx_open(struct ambnc_rexx *rexx);
void ambnc_rexx_close(struct ambnc_rexx *rexx);
unsigned long ambnc_rexx_signal_mask(const struct ambnc_rexx *rexx);
void ambnc_rexx_process(struct ambnc_rexx *rexx, struct ambnc_rexx_control *control);
int ambnc_rexx_run_event(struct ambnc_rexx *rexx,
                         struct ambnc_rexx_control *control,
                         const char *hook_dir,
                         const char *event_name,
                         const char *nick,
                         const char *target,
                         const char *text);

#endif
