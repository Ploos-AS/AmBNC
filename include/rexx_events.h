#ifndef AMBNC_REXX_EVENTS_H
#define AMBNC_REXX_EVENTS_H

#include "rexx.h"

void ambnc_rexx_events_bind(struct ambnc_rexx *rexx,
                            struct ambnc_rexx_control *control);
void ambnc_rexx_emit_lifecycle(const char *event_name, const char *detail);
void ambnc_rexx_emit_irc_line(const char *line);

#endif
