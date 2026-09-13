#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <dos/dos.h>
#include <proto/dos.h>

#include "state.h"

static int same_ci(const char *a, const char *b)
{
    while (*a != '\0' && *b != '\0') {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
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

static int prefix_nick(const char *line, char *out, unsigned int size)
{
    unsigned int n = 0;
    const char *p;
    if (line == 0 || line[0] != ':') {
        if (size != 0) out[0] = '\0';
        return 0;
    }
    p = line + 1;
    while (*p != '\0' && *p != '!' && *p != ' ') {
        if (n + 1 < size) out[n++] = *p;
        ++p;
    }
    if (size != 0) out[n] = '\0';
    return n != 0;
}

static const char *skip_prefix(const char *line)
{
    const char *space;
    if (line == 0) return "";
    if (line[0] != ':') return line;
    space = strchr(line, ' ');
    return space != 0 ? space + 1 : "";
}

static int parse_command(const char *line, char *command, unsigned int size, const char **params)
{
    const char *p = skip_prefix(line);
    unsigned int n = 0;
    while (*p == ' ') ++p;
    while (*p != '\0' && *p != ' ') {
        if (n + 1 < size) command[n++] = *p;
        ++p;
    }
    if (size != 0) command[n] = '\0';
    while (*p == ' ') ++p;
    *params = p;
    return n != 0;
}

static const char *take_param(const char *params, char *out, unsigned int size)
{
    unsigned int n = 0;
    if (params == 0) {
        if (size != 0) out[0] = '\0';
        return 0;
    }
    while (*params == ' ') ++params;
    if (*params == ':') ++params;
    while (*params != '\0' && *params != ' ') {
        if (n + 1 < size) out[n++] = *params;
        ++params;
    }
    if (size != 0) out[n] = '\0';
    while (*params == ' ') ++params;
    return params;
}

static void add_channel(struct ambnc_session_state *state, const char *channel)
{
    unsigned int i;
    for (i = 0; i < state->channel_count; ++i)
        if (same_ci(state->channels[i], channel)) return;
    if (state->channel_count >= AMBNC_STATE_CHANNELS_MAX) return;
    copy_bounded(state->channels[state->channel_count], AMBNC_STATE_TARGET_MAX + 1, channel);
    ++state->channel_count;
}

static void remove_channel(struct ambnc_session_state *state, const char *channel)
{
    unsigned int i;
    for (i = 0; i < state->channel_count; ++i) {
        if (same_ci(state->channels[i], channel)) {
            unsigned int j;
            for (j = i + 1; j < state->channel_count; ++j)
                memcpy(state->channels[j - 1], state->channels[j], AMBNC_STATE_TARGET_MAX + 1);
            --state->channel_count;
            return;
        }
    }
}

static struct ambnc_target_ring *ring_for(struct ambnc_session_state *state, const char *target)
{
    unsigned int i;
    for (i = 0; i < state->ring_count; ++i)
        if (same_ci(state->rings[i].target, target)) return &state->rings[i];
    if (state->ring_count >= AMBNC_STATE_RING_TARGETS_MAX) return 0;
    memset(&state->rings[state->ring_count], 0, sizeof(state->rings[state->ring_count]));
    copy_bounded(state->rings[state->ring_count].target, AMBNC_STATE_TARGET_MAX + 1, target);
    return &state->rings[state->ring_count++];
}

static void stamp_line(struct ambnc_ring_line *entry)
{
    struct DateStamp stamp;
    DateStamp(&stamp);
    entry->days = (unsigned long)stamp.ds_Days;
    entry->minutes = (unsigned long)stamp.ds_Minute;
    entry->ticks = (unsigned long)stamp.ds_Tick;
}

static void ring_push(struct ambnc_session_state *state,
                      struct ambnc_target_ring *ring,
                      const char *line)
{
    unsigned int slot;
    unsigned int limit = state->ring_lines_limit;

    if (limit == 0 || limit > AMBNC_STATE_RING_LINES_MAX)
        limit = AMBNC_STATE_RING_LINES_DEFAULT;

    if (ring->count < limit) {
        slot = (ring->start + ring->count) % limit;
        ++ring->count;
    } else {
        slot = ring->start;
        ring->start = (ring->start + 1U) % limit;
        ++ring->dropped;
    }
    copy_bounded(ring->lines[slot].line, AMBNC_IRC_LINE_MAX + 1, line);
    stamp_line(&ring->lines[slot]);
}

void ambnc_state_init(struct ambnc_session_state *state,
                      const char *nick,
                      unsigned int ring_lines_limit)
{
    memset(state, 0, sizeof(*state));
    copy_bounded(state->nick, sizeof(state->nick), nick != 0 ? nick : "");
    if (ring_lines_limit == 0 || ring_lines_limit > AMBNC_STATE_RING_LINES_MAX)
        ring_lines_limit = AMBNC_STATE_RING_LINES_DEFAULT;
    state->ring_lines_limit = ring_lines_limit;
}

void ambnc_state_observe_line(struct ambnc_session_state *state,
                              const char *line,
                              int downstream_attached)
{
    char command[16];
    char sender[AMBNC_STATE_NICK_MAX + 1];
    char target[AMBNC_STATE_TARGET_MAX + 1];
    const char *params;

    if (state == 0 || line == 0) return;
    sender[0] = '\0';
    (void)prefix_nick(line, sender, sizeof(sender));
    if (!parse_command(line, command, sizeof(command), &params)) return;

    if (strcmp(command, "NICK") == 0) {
        char nick[AMBNC_STATE_NICK_MAX + 1];
        if (same_ci(sender, state->nick)) {
            (void)take_param(params, nick, sizeof(nick));
            if (nick[0] != '\0') copy_bounded(state->nick, sizeof(state->nick), nick);
        }
        return;
    }

    if (strcmp(command, "JOIN") == 0) {
        if (same_ci(sender, state->nick)) {
            (void)take_param(params, target, sizeof(target));
            if (target[0] != '\0') add_channel(state, target);
        }
    } else if (strcmp(command, "PART") == 0) {
        if (same_ci(sender, state->nick)) {
            (void)take_param(params, target, sizeof(target));
            if (target[0] != '\0') remove_channel(state, target);
        }
    } else if (strcmp(command, "KICK") == 0) {
        char victim[AMBNC_STATE_NICK_MAX + 1];
        params = take_param(params, target, sizeof(target));
        (void)take_param(params, victim, sizeof(victim));
        if (target[0] != '\0' && same_ci(victim, state->nick)) remove_channel(state, target);
    }

    if (!downstream_attached &&
        (strcmp(command, "PRIVMSG") == 0 || strcmp(command, "NOTICE") == 0)) {
        struct ambnc_target_ring *ring;
        (void)take_param(params, target, sizeof(target));
        if (target[0] == '\0') return;
        if (same_ci(target, state->nick) && sender[0] != '\0')
            copy_bounded(target, sizeof(target), sender);
        ring = ring_for(state, target);
        if (ring != 0) ring_push(state, ring, line);
    }
}

int ambnc_state_replay(struct ambnc_session_state *state,
                       ambnc_state_send_line_fn send_line,
                       void *userdata)
{
    unsigned int i;

    if (state == 0 || send_line == 0) return -1;

    for (i = 0; i < state->ring_count; ++i) {
        struct ambnc_target_ring *ring = &state->rings[i];
        unsigned int j;
        unsigned int limit = state->ring_lines_limit;
        char notice[AMBNC_IRC_LINE_MAX + 1];
        int written;

        if (limit == 0 || limit > AMBNC_STATE_RING_LINES_MAX)
            limit = AMBNC_STATE_RING_LINES_DEFAULT;
        if (ring->count == 0) continue;

        written = snprintf(notice, sizeof(notice),
                           ":AmBNC NOTICE * :Backlog target=%s lines=%u dropped=%lu",
                           ring->target,
                           ring->count,
                           ring->dropped);
        if (written <= 0 || written >= (int)sizeof(notice) ||
            send_line(userdata, notice) != 0)
            return -1;

        for (j = 0; j < ring->count; ++j) {
            unsigned int slot = (ring->start + j) % limit;
            const struct ambnc_ring_line *entry = &ring->lines[slot];

            written = snprintf(notice, sizeof(notice),
                               ":AmBNC NOTICE * :Backlog stamp=%lu:%lu:%lu target=%s",
                               entry->days,
                               entry->minutes,
                               entry->ticks,
                               ring->target);
            if (written <= 0 || written >= (int)sizeof(notice) ||
                send_line(userdata, notice) != 0 ||
                send_line(userdata, entry->line) != 0)
                return -1;
        }

        ring->start = 0;
        ring->count = 0;
        ring->dropped = 0;
    }
    return 0;
}
