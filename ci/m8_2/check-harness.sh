#!/usr/bin/env bash
set -euo pipefail

required=(
  ci/m8_2/amigaos-template.fs-uae
  ci/m8_2/AmBNC-M8_2.cfg.in
  ci/m8_2/prepare.sh
  ci/m8_2/run-host-fixture.sh
  ci/m8_2/summarize-evidence.sh
  ci/m8_2/fixture_server.py
  ci/m8_2/tls_proxy.py
  ci/m8_2/amiga/Setup-M8_2
  ci/m8_2/amiga/Start-M8_2
  ci/m8_2/amiga/Snapshot-M8_2
  ci/m8_2/rexx/Commands-M8_2.rexx
  ci/m8_2/rexx/Backlog-M8_2.rexx
  ci/m8_2/rexx/Drop-Alpha-M8_2.rexx
  ci/m8_2/rexx/Quit-M8_2.rexx
  ci/m8_2/rexx/hooks/Hook-M8_2.rexx
  ci/m8_2/QUALIFICATION_CHECKLIST.md
  docs/M8_2_QUALIFICATION.md
)

for path in "${required[@]}"; do
  if [[ ! -f "$path" ]]; then
    echo "ERROR: M8.2 harness file missing: $path" >&2
    exit 1
  fi
done

grep -q '^bsdsocket_library = 1$' ci/m8_2/amigaos-template.fs-uae
grep -q 'TLS_MODE=PROXY' ci/m8_2/AmBNC-M8_2.cfg.in
grep -q 'SASL_PLAIN=YES' ci/m8_2/AmBNC-M8_2.cfg.in
grep -q 'OVERALL: PENDING' ci/m8_2/QUALIFICATION_CHECKLIST.md
grep -q 'Status: \*\*PENDING\*\*' docs/M8_2_QUALIFICATION.md
grep -q '1, 2, 4, 8, 16, 30' src/multinet.c

if grep -ERn --exclude=check-harness.sh \
  '(BEGIN (RSA |OPENSSH |EC )?PRIVATE KEY|oauth_token|sasl_password[[:space:]]*=[[:space:]]*[^@])' \
  ci/m8_2 docs/M8_2_QUALIFICATION.md >/dev/null; then
  echo 'ERROR: possible credential or private key in M8.2 tracked inputs' >&2
  exit 1
fi

python3 -m py_compile ci/m8_2/fixture_server.py ci/m8_2/tls_proxy.py
rm -rf ci/m8_2/__pycache__

echo 'M8.2 qualification harness checks: PASS'
