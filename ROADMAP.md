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

## M5 — ARexx API

- live `AMBNC` message port
- STATUS, CONNECT, DISCONNECT, JOIN, PART, MSG, NOTICE, RAW, BUFFER, RELOAD, QUIT
- event hooks for connect/disconnect/message/channel events
- example scripts

## M6 — Multiple sessions

- multiple IRC networks
- multiple downstream clients where practical
- per-network configuration/state

## M7 — Modern IRC extensions

- CAP negotiation
- SASL where feasible
- TLS strategy appropriate for classic Amiga hardware

## M8 — Qualification and release

- automated host/static qualification
- FS-UAE/AROS CI where practical
- local AmigaOS 2.04+ runtime qualification
- release packaging and documentation
