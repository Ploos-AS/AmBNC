#!/usr/bin/env bash
set -euo pipefail
IMAGE="${AMBNC_BEBBO_IMAGE:-amigadev/m68k-amigaos-gcc@sha256:b18080e6ffca8f793e0f539536a9138e9d2a548ca1a301c7483f43ee15fedfed}"
OUT_DIR="${1:-build/fs-uae/native}"
mkdir -p "$OUT_DIR"
docker pull "$IMAGE"
docker image inspect "$IMAGE" --format '{{join .RepoDigests "\n"}}' | tee "$OUT_DIR/toolchain-image.txt"
docker run --rm -v "$PWD:/work" -w /work "$IMAGE" m68k-amigaos-gcc \
  -Iinclude -Os -Wall -Wextra -Werror -m68000 -mcrt=nix20 \
  -o AmBNC \
  src/main.c src/net.c src/irc.c src/downstream.c src/state.c \
  src/rexx.c src/rexx_events.c src/networks.c src/multinet.c src/upstream.c
cp AmBNC "$OUT_DIR/AmBNC"
file "$OUT_DIR/AmBNC" | tee "$OUT_DIR/file.txt"
sha256sum "$OUT_DIR/AmBNC" | tee "$OUT_DIR/AmBNC.sha256"
if ! grep -Eiq 'AmigaOS|Amiga.*executable|loadseg' "$OUT_DIR/file.txt"; then
  echo 'ERROR: native output is not recognized as an Amiga executable' >&2
  exit 1
fi
printf 'STATUS=PASS\nGATE=M8_1_NATIVE_BEBBO_BUILD\nIMAGE=%s\nBINARY=%s\n' "$IMAGE" "$OUT_DIR/AmBNC" | tee "$OUT_DIR/result.txt"
