#include "irc.h"
#include "modernirc.h"
#include "state.h"

static int ambnc_m7_send_registration(int sock,
                                      const char *nick,
                                      const char *user,
                                      const char *pass);
static void ambnc_m7_observe_line(struct ambnc_session_state *state,
                                  const char *line,
                                  int downstream_attached);

#define ambnc_irc_send_registration ambnc_m7_send_registration
#define ambnc_state_observe_line ambnc_m7_observe_line
#include "multinet.c"
#undef ambnc_state_observe_line
#undef ambnc_irc_send_registration

static struct ambnc_modernirc modern_states[AMBNC_NETWORKS_MAX];

static int runtime_index_by_sock(int sock)
{
    unsigned int i;
    for (i = 0; i < AMBNC_NETWORKS_MAX; ++i)
        if (runtimes[i].upstream == sock) return (int)i;
    return -1;
}

static int runtime_index_by_state(const struct ambnc_session_state *state)
{
    unsigned int i;
    for (i = 0; i < AMBNC_NETWORKS_MAX; ++i)
        if (&runtimes[i].state == state) return (int)i;
    return -1;
}

static int ambnc_m7_send_registration(int sock,
                                      const char *nick,
                                      const char *user,
                                      const char *pass)
{
    int index = runtime_index_by_sock(sock);
    if (index >= 0) {
        if (ambnc_modernirc_start(sock,
                                  runtimes[index].config,
                                  &modern_states[index]) != 0)
            return -1;
    }
    return ambnc_irc_send_registration(sock, nick, user, pass);
}

static void ambnc_m7_observe_line(struct ambnc_session_state *state,
                                  const char *line,
                                  int downstream_attached)
{
    int index = runtime_index_by_state(state);
    if (index >= 0 && runtimes[index].upstream >= 0) {
        int rc = ambnc_modernirc_handle_line(runtimes[index].upstream,
                                             runtimes[index].config,
                                             &modern_states[index],
                                             line);
        if (rc < 0)
            puts("AmBNC: CAP/SASL negotiation failed; CAP ended");
    }
    ambnc_state_observe_line(state, line, downstream_attached);
}
