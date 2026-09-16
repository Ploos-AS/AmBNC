#!/usr/bin/env bash
set -euo pipefail

out_dir="${1:-build/m8_2}"
evidence="$out_dir/shared/evidence"
fixture_log="$evidence/host-fixture.jsonl"
proxy_log="$evidence/tls-proxy.jsonl"
env_file="$out_dir/host/fixture.env"
passes=0
pending=0
failures=0

result() {
  local status="$1" label="$2"
  printf '%-8s %s\n' "$status" "$label"
  case "$status" in
    PASS) passes=$((passes + 1)) ;;
    FAIL) failures=$((failures + 1)) ;;
    *) pending=$((pending + 1)) ;;
  esac
}

event() {
  local log="$1" event_name="$2" network="$3" label="$4"
  if [[ -f "$log" ]] && grep -Fq "\"event\": \"$event_name\"" "$log" &&
     grep -F "\"event\": \"$event_name\"" "$log" | grep -Fq "\"network\": \"$network\""; then
    result PASS "$label"
  else
    result PENDING "$label"
  fi
}

echo 'M8.2 evidence summary (advisory; the signed checklist is authoritative)'
event "$fixture_log" fixture_ready all 'host fixture started'
event "$fixture_log" registered alpha 'alpha IRC registration'
event "$fixture_log" registered beta 'beta IRC registration through proxy'
event "$fixture_log" pong alpha 'alpha PING/PONG'
event "$fixture_log" pong beta 'beta PING/PONG'
event "$fixture_log" sasl_pass alpha 'alpha SASL PLAIN'
event "$fixture_log" cap_end alpha 'alpha CAP completion'
event "$fixture_log" cap_end beta 'beta CAP completion'
event "$fixture_log" forced_drop alpha 'alpha forced disconnect'
event "$fixture_log" outage_reject alpha 'alpha bounded reconnect retries'
event "$proxy_log" tls_proxy_connected beta 'beta TLS proxy transport'

for pair in \
  'rexx-commands.txt:M8_2_REXX_COMMANDS=PASS:ARexx command suite' \
  'rexx-backlog.txt:M8_2_BACKLOG_COLLECTION=PASS:backlog collection request' \
  'rexx-reconnect.txt:M8_2_DROP_ALPHA_SENT=PASS:independent reconnect request' \
  'rexx-quit.txt:M8_2_REXX_QUIT_REQUEST=PASS:ARexx QUIT request'; do
  IFS=: read -r file marker label <<< "$pair"
  if [[ -f "$evidence/$file" ]] && grep -Fq "$marker" "$evidence/$file"; then
    result PASS "$label"
  else
    result PENDING "$label"
  fi
done

if [[ -s "$evidence/hooks.log" ]]; then
  result PASS 'ARexx hook log present'
else
  result PENDING 'ARexx hook log present'
fi

if [[ -f "$env_file" && -f "$fixture_log" ]]; then
  # shellcheck disable=SC1090
  source "$env_file"
  if grep -Fq "$M8_2_SASL_PASS" "$fixture_log" "$proxy_log" 2>/dev/null; then
    result FAIL 'fixture credential absent from logs'
  else
    result PASS 'fixture credential absent from logs'
  fi
else
  result PENDING 'fixture credential absent from logs'
fi

echo
echo "PASS=$passes PENDING=$pending FAIL=$failures"
echo 'OVERALL=PENDING'
echo 'Reason: visible execution, downstream transcript, screenshots, environment,'
echo 'and operator-reviewed checklist cannot be inferred by this helper.'
[[ "$failures" -eq 0 ]]
