# AmBNC Roadmap

## M0 — Foundation — COMPLETE

- Repository and licensing baseline
- AmigaOS 2.04+ / 68000 target documented
- Bebbo GCC build baseline
- Module boundaries for IRC core, networking, downstream, state and ARexx
- ARexx port name fixed as `AMBNC`
- Minimal native executable skeleton

## M1 — Upstream IRC session — IMPLEMENTED

- `bsdsocket.library` adapter
- TCP connect/disconnect
- IRC line framing/parser baseline
- PASS/NICK/USER registration
- PING/PONG
- reconnect with bounded backoff

Runtime qualification remains separate; see `docs/M1.md`.

## M2 — Downstream client — IMPLEMENTED

- local IPv4 listener with configurable port
- one downstream IRC client
- local consumption of downstream PASS/NICK/USER registration
- minimal synthetic 001 attach welcome
- bidirectional relay between downstream and upstream
- downstream QUIT detaches only the client
- upstream loss closes downstream cleanly before reconnect
- one `WaitSelect()` loop services upstream, listener, downstream and Ctrl-C

Runtime qualification remains separate; see `docs/M2.md`.

## M3 — Session/state — IMPLEMENTED

- current nick state from self NICK events
- up to 16 joined channels tracked from self JOIN/PART/KICK events
- upstream session remains active while downstream is absent
- up to 16 bounded per-target ring buffers
- up to 32 IRC lines retained per target with overwrite counters
- private messages keyed by sender nick and channel messages by channel

Runtime qualification remains separate; see `docs/M3.md`.

## M4 — Backlog playback — IMPLEMENTED

- replay buffered messages after downstream reconnect
- Amiga DateStamp metadata retained per buffered line
- target summary and timestamp metadata NOTICEs during replay
- original IRC lines replayed unchanged
- runtime-configurable per-target buffer limit from 1 to 32 lines
- failed replay preserves rings not yet completed

Runtime qualification remains separate; see `docs/M4.md`.

## M5 — ARexx API — IMPLEMENTED

- live `AMBNC` Exec/ARexx message port
- STATUS, CONNECT, DISCONNECT, JOIN, PART, MSG, NOTICE, RAW, BUFFER, RELOAD and QUIT commands
- ARexx signal multiplexed with IRC sockets through the existing WaitSelect loop
- DISCONNECT holds upstream offline until CONNECT
- optional lifecycle, message and channel event hooks under `REXX:AmBNC/`
- missing hook scripts are ignored cleanly
- example ON_PRIVMSG hook

Runtime qualification remains separate; see `docs/M5.md`. `RELOAD` is reserved/accepted but is a no-op while configuration remains CLI-only.

## M6 — Multiple sessions — IMPLEMENTED

- optional `-c CONFIG` multi-network mode with legacy single-session CLI preserved
- up to four named `[NETWORK name]` sections
- one upstream connection, downstream listener/client, state and backlog set per network
- simultaneous socket servicing in one bounded `WaitSelect()` loop
- independent reconnect state per network
- per-network HOST/PORT/NICK/USER/PASS/LISTEN_PORT/BACKLOG_LINES configuration
- one downstream client per network, allowing multiple downstream clients across different networks
- aggregate ARexx status/control compatibility retained

Runtime qualification remains separate; see `docs/M6.md`. Explicit per-network ARexx send routing remains a hardening item; legacy send commands use the first connected network.

## M7 — Modern IRC extensions — IMPLEMENTED

- per-network CAP negotiation with IRCv3 `CAP LS 302`
- optional SASL PLAIN using configured USER/PASS
- bounded Base64 SASL payload generation
- SASL success/failure handling and clean CAP termination
- per-network CAP/SASL negotiation state layered over the M6 runtime
- `TLS_MODE=PLAIN|PROXY` strategy for classic hardware
- external TLS termination recommended for 68000 systems; native AmiSSL/TLS deferred

Runtime qualification remains separate; see `docs/M7.md`.

## M8 — Qualification and release

### M8.1 — Automated Bebbo + FS-UAE/AROS qualification — PASS

- M7 static checks PASS
- pinned Bebbo 68000 native build PASS
- output recognized as AmigaOS loadseg executable
- FS-UAE A1200 / internal AROS guest execution PASS
- qualification evidence uploaded as GitHub Actions artifact

See `docs/M8_1.md`.

### M8.2 — Local AmigaOS runtime qualification — NEXT

- actual AmigaOS 2.04+ / Workbench runtime
- `bsdsocket.library` enabled
- live IRC connect/reconnect
- downstream attach/detach and backlog replay
- ARexx command and hook qualification
- multi-network qualification
- CAP/SASL and TLS-proxy qualification

### M8.3 — Release packaging

- release documentation
- package archive/checksums
- GitHub release artifacts
