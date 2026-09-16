/* M8.2 ARexx command surface. Run while both fixture networks are connected. */
OPTIONS RESULTS
failures = 0

CALL TestCommand 'STATUS'
CALL TestCommand 'BUFFER'
CALL TestCommand 'JOIN #ambnc'
ADDRESS COMMAND 'Wait 1 SECS'
CALL TestCommand 'MSG #ambnc M8_2_MSG'
CALL TestCommand 'NOTICE #ambnc M8_2_NOTICE'
CALL TestCommand 'RAW PING :M8_2_RAW'
CALL TestCommand 'PART #ambnc M8_2_PART'
CALL TestCommand 'RELOAD'
CALL TestCommand 'DISCONNECT'
ADDRESS COMMAND 'Wait 2 SECS'
CALL TestContains 'STATUS', 'upstream=DISCONNECTED'
CALL TestCommand 'CONNECT'
ADDRESS COMMAND 'Wait 4 SECS'
CALL TestContains 'STATUS', 'upstream=CONNECTED'
CALL TestCommand 'BUFFER'

IF failures = 0 THEN DO
  SAY 'M8_2_REXX_COMMANDS=PASS'
  EXIT 0
END
SAY 'M8_2_REXX_COMMANDS=FAIL failures=' || failures
EXIT 10

TestCommand: PROCEDURE EXPOSE failures
  PARSE ARG command
  ADDRESS 'AMBNC' command
  code = RC
  response = RESULT
  SAY command ': RC=' || code 'RESULT=' || response
  IF code ~= 0 THEN failures = failures + 1
  RETURN

TestContains: PROCEDURE EXPOSE failures
  PARSE ARG command, expected
  ADDRESS 'AMBNC' command
  code = RC
  response = RESULT
  SAY command ': RC=' || code 'RESULT=' || response
  IF code ~= 0 | POS(expected, response) = 0 THEN failures = failures + 1
  RETURN
