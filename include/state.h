#ifndef AMBNC_STATE_H
#define AMBNC_STATE_H

#include "irc.h"

#define AMBNC_STATE_CHANNELS_MAX 16
#define AMBNC_STATE_NICK_MAX 31
#define AMBNC_STATE_TARGET_MAX 63
#define AMBNC_STATE_RING_TARGETS_MAX 16
#define AMBNC_STATE_RING_LINES 32

struct ambnc_ring_line {
    char line[AMBNC_IRC_LINE_MAX + 1];
};

struct ambnc_target_ring {
    char target[AMBNC_STATE_TARGET_MAX + 1];
    struct ambnc_ring_line lines[AMBNC_STATE_RING_LINES];
    unsigned int start;
    unsigned int count;
    unsigned long dropped;
};

struct ambnc_session_state {
    char nick[AMBNC_STATE_NICK_MAX + 1];
    char channels[AMBNC_STATE_CHANNELS_MAX][AMBNC_STATE_TARGET_MAX + 1];
    unsigned int channel_count;
    struct ambnc_target_ring rings[AMBNC_STATE_RING_TARGETS_MAX];
    unsigned int ring_count;
};

void ambnc_state_init(struct ambnc_session_state *state, const char *nick);
void ambnc_state_observe_line(struct ambnc_session_state *state,
                              const char *line,
                              int downstream_attached);

#endif
