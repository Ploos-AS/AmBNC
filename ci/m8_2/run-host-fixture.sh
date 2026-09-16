#!/usr/bin/env bash
set -euo pipefail

out_dir="${1:-build/m8_2}"
env_file="$out_dir/host/fixture.env"
tls_dir="$out_dir/host/tls"
log_file="$out_dir/shared/evidence/host-fixture.jsonl"
proxy_log="$out_dir/shared/evidence/tls-proxy.jsonl"

if [[ ! -f "$env_file" ]]; then
  echo "ERROR: missing $env_file; run make qualify-m8_2 first" >&2
  exit 2
fi
# This ignored file contains only the locally generated fixture credential.
# shellcheck disable=SC1090
source "$env_file"
mkdir -p "$tls_dir" "$(dirname "$log_file")"

if [[ ! -f "$tls_dir/cert.pem" || ! -f "$tls_dir/key.pem" ]]; then
  openssl req -x509 -newkey rsa:2048 -nodes -days 2 \
    -subj '/CN=ambnc-m8-2-fixture' \
    -keyout "$tls_dir/key.pem" -out "$tls_dir/cert.pem" >/dev/null 2>&1
  chmod 600 "$tls_dir/key.pem"
fi

: > "$log_file"
: > "$proxy_log"
python3 ci/m8_2/fixture_server.py \
  --bind "$M8_2_BIND" \
  --alpha-port "$M8_2_ALPHA_PORT" \
  --beta-tls-port "$M8_2_BETA_TLS_PORT" \
  --sasl-user "$M8_2_SASL_USER" \
  --sasl-pass "$M8_2_SASL_PASS" \
  --cert "$tls_dir/cert.pem" \
  --key "$tls_dir/key.pem" >> "$log_file" 2>&1 &
fixture_pid=$!
python3 ci/m8_2/tls_proxy.py \
  --bind "$M8_2_BIND" \
  --listen-port "$M8_2_BETA_PROXY_PORT" \
  --upstream-port "$M8_2_BETA_TLS_PORT" >> "$proxy_log" 2>&1 &
proxy_pid=$!

# Invoked indirectly by the EXIT/INT/TERM trap.
# shellcheck disable=SC2329
cleanup() {
  kill "$fixture_pid" "$proxy_pid" 2>/dev/null || true
  wait "$fixture_pid" "$proxy_pid" 2>/dev/null || true
}
trap cleanup EXIT INT TERM

sleep 1
if ! kill -0 "$fixture_pid" 2>/dev/null || ! kill -0 "$proxy_pid" 2>/dev/null; then
  echo 'ERROR: host fixture failed to start' >&2
  tail -20 "$log_file" "$proxy_log" >&2 || true
  exit 1
fi

echo 'M8.2 host fixture is ready.'
echo "  IRC evidence: $log_file"
echo "  TLS evidence: $proxy_log"
echo 'Leave this process running throughout the visible FS-UAE test.'
echo 'Press Ctrl-C after AmBNC has stopped and evidence is copied.'

while kill -0 "$fixture_pid" 2>/dev/null && kill -0 "$proxy_pid" 2>/dev/null; do
  sleep 1
done
echo 'ERROR: a host fixture process stopped unexpectedly' >&2
exit 1
