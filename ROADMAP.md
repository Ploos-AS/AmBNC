# AmBNC Roadmap

## M0 — Foundation

- Repository and licensing baseline
- AmigaOS 2.04+ / 68000 target documented
- Bebbo GCC build baseline
- Module boundaries for IRC core, networking, downstream, state and ARexx
- ARexx port name fixed as `AMBNC`
- Minimal native executable skeleton

## M1 — Upstream IRC session

- `bsdsocket.library` adapter
- TCP connect/disconnect
- IRC line framing/parser baseline
- PASS/NICK/USER registration
- PING/PONG
- reconnect with bounded backoff

## M2 — Downstream client

- Local listener
- one downstream IRC client
- relay between downstream and upstream
- clean disconnect/reconnect semantics

## M3 — Session/state

- channel and nickname state
- persistent upstream session while downstream is absent
- bounded per-target ring buffers

## M4 — Backlog playback

- replay buffered messages after downstream reconnect
- timestamps/metadata where feasible
- configurable buffer limits

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
