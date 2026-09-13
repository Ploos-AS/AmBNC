# Architecture

AmBNC is split into small modules so the IRC protocol engine can remain largely independent of AmigaOS-specific services.

## Modules

- `core` — lifecycle and shared application state
- `irc` — IRC line parsing/formatting and protocol state
- `net` — `bsdsocket.library` integration
- `upstream` — IRC server connection/session
- `downstream` — local IRC client listener/session
- `buffer` — bounded backlog/ring buffers
- `config` — configuration loading and validation
- `rexx` — ARexx message port and command dispatch

## ARexx

The public ARexx port is fixed as `AMBNC`. Internal operations should be exposed through shared core functions so command-line/UI paths and ARexx commands do not duplicate behavior.

The initial command vocabulary reserved for M5 is:

`STATUS`, `CONNECT`, `DISCONNECT`, `JOIN`, `PART`, `MSG`, `NOTICE`, `RAW`, `BUFFER`, `RELOAD`, `QUIT`.

## Constraints

The baseline target is a 68000-class Amiga running AmigaOS 2.04 or newer. Dynamic allocation and buffers should therefore remain bounded and predictable. Networking is implemented against `bsdsocket.library`; newer protocol features must not silently raise the baseline requirements.
