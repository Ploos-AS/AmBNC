/* Generic M8.2 hook copied to each supported ON_* filename. */
PARSE ARG event nick target text
IF OPEN('HOOKLOG', 'AMBNCQ:evidence/hooks.log', 'APPEND') THEN DO
  CALL WRITELN 'HOOKLOG', event || ' nick=' || nick || ' target=' || target || ' text=' || text
  CALL CLOSE 'HOOKLOG'
END
EXIT 0
