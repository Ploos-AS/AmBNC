#!/usr/bin/env bash
set -euo pipefail

out_dir="${1:-build/m8_2}"
binary="${2:-build/fs-uae/native/AmBNC}"
host="${M8_2_HOST:-HOST_IP_HERE}"
system_dir="${M8_2_SYSTEM_DIR:-SYSTEM_DIR_HERE}"
kickstart_file="${M8_2_KICKSTART_FILE:-KICKSTART_FILE_HERE}"
sasl_user="${M8_2_SASL_USER:-ambnc_m8_2}"
sasl_pass="${M8_2_SASL_PASS:-m8_2_fixture_${RANDOM}_${RANDOM}}"

case "$out_dir" in
  ''|/|.) echo "ERROR: unsafe M8.2 output directory: $out_dir" >&2; exit 2 ;;
esac
if [[ ! -f "$binary" ]]; then
  echo "ERROR: Amiga binary missing: $binary" >&2
  exit 2
fi
if [[ ! "$sasl_user" =~ ^[A-Za-z0-9_.-]+$ || ! "$sasl_pass" =~ ^[A-Za-z0-9_.-]+$ ]]; then
  echo 'ERROR: M8.2 fixture SASL values must use only A-Z, a-z, 0-9, _, ., or -' >&2
  exit 2
fi

shared="$out_dir/shared"
evidence="$shared/evidence"
host_dir="$out_dir/host"
mkdir -p "$shared/rexx/hooks" "$evidence" "$host_dir/tls"

cp "$binary" "$shared/AmBNC"
cp ci/m8_2/amiga/Setup-M8_2 "$shared/Setup-M8_2"
cp ci/m8_2/amiga/Start-M8_2 "$shared/Start-M8_2"
cp ci/m8_2/amiga/Snapshot-M8_2 "$shared/Snapshot-M8_2"
cp ci/m8_2/rexx/Commands-M8_2.rexx "$shared/rexx/Commands-M8_2.rexx"
cp ci/m8_2/rexx/Backlog-M8_2.rexx "$shared/rexx/Backlog-M8_2.rexx"
cp ci/m8_2/rexx/Drop-Alpha-M8_2.rexx "$shared/rexx/Drop-Alpha-M8_2.rexx"
cp ci/m8_2/rexx/Quit-M8_2.rexx "$shared/rexx/Quit-M8_2.rexx"
cp ci/m8_2/rexx/hooks/Hook-M8_2.rexx "$shared/rexx/hooks/Hook-M8_2.rexx"
cp ci/m8_2/QUALIFICATION_CHECKLIST.md "$evidence/QUALIFICATION_CHECKLIST.md"

escape_sed() { printf '%s' "$1" | sed 's/[&|\\]/\\&/g'; }
host_escaped="$(escape_sed "$host")"
user_escaped="$(escape_sed "$sasl_user")"
pass_escaped="$(escape_sed "$sasl_pass")"
sed -e "s|@HOST@|$host_escaped|g" \
    -e "s|@SASL_USER@|$user_escaped|g" \
    -e "s|@SASL_PASS@|$pass_escaped|g" \
    ci/m8_2/AmBNC-M8_2.cfg.in > "$shared/AmBNC-M8_2.cfg"

system_escaped="$(escape_sed "$system_dir")"
kickstart_escaped="$(escape_sed "$kickstart_file")"
shared_abs="$(cd "$shared" && pwd)"
shared_escaped="$(escape_sed "$shared_abs")"
sed -e "s|@SYSTEM_DIR@|$system_escaped|g" \
    -e "s|@KICKSTART_FILE@|$kickstart_escaped|g" \
    -e "s|@QUALIFICATION_DIR@|$shared_escaped|g" \
    ci/m8_2/amigaos-template.fs-uae > "$out_dir/AmBNC-M8_2.fs-uae"

cat > "$host_dir/fixture.env" <<EOF
M8_2_BIND=0.0.0.0
M8_2_ALPHA_PORT=17667
M8_2_BETA_TLS_PORT=17668
M8_2_BETA_PROXY_PORT=17669
M8_2_SASL_USER=$sasl_user
M8_2_SASL_PASS=$sasl_pass
EOF
chmod 600 "$host_dir/fixture.env"

sha256sum "$shared/AmBNC" > "$evidence/AmBNC.sha256"
file "$shared/AmBNC" > "$evidence/AmBNC.file.txt"
{
  echo 'OVERALL=PENDING'
  echo 'REASON=visible AmigaOS runtime qualification has not been executed'
  echo "BINARY=$shared_abs/AmBNC"
  echo "HOST=$host"
} > "$evidence/STATUS.txt"

echo
echo 'M8.2 local qualification bundle prepared.'
echo "  FS-UAE config: $out_dir/AmBNC-M8_2.fs-uae"
echo "  Amiga volume:  $shared_abs (mounted as AMBNCQ:)"
echo "  Evidence:      $evidence"
echo '  Overall state: PENDING (no AmigaOS runtime result was inferred)'
echo
if [[ "$host" == HOST_IP_HERE || "$system_dir" == SYSTEM_DIR_HERE ||
      "$kickstart_file" == KICKSTART_FILE_HERE ]]; then
  echo 'Set M8_2_HOST, M8_2_SYSTEM_DIR, and M8_2_KICKSTART_FILE, then rerun'
  echo 'make qualify-m8_2 before launching FS-UAE.'
else
  echo "Terminal 1: ci/m8_2/run-host-fixture.sh '$out_dir'"
  echo "Terminal 2: fs-uae '$out_dir/AmBNC-M8_2.fs-uae'"
fi
echo 'Follow docs/M8_2_QUALIFICATION.md and the copied checklist.'
