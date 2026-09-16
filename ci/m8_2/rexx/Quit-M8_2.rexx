/* Run last. A successful reply requests clean AmBNC shutdown. */
OPTIONS RESULTS
ADDRESS 'AMBNC' 'QUIT'
SAY 'QUIT: RC=' || RC 'RESULT=' || RESULT
IF RC ~= 0 THEN EXIT 10
SAY 'M8_2_REXX_QUIT_REQUEST=PASS'
EXIT 0
