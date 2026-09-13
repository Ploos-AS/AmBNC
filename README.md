# AmBNC

AmBNC is a lightweight IRC bouncer for classic Amiga systems, designed around AmigaOS conventions and first-class ARexx automation.

## M0 baseline

- Target: AmigaOS 2.04+
- CPU baseline: Motorola 68000
- Networking: `bsdsocket.library`
- ARexx port: `AMBNC`
- License: MIT
- Toolchain: Bebbo `m68k-amigaos-gcc`
- Initial scope: one upstream IRC connection, one downstream client, reconnect/state foundations

M0 establishes the repository structure, architecture, build baseline and roadmap. Networking and ARexx runtime behavior are introduced incrementally in later milestones.

## Build

```sh
make
```

Override the cross compiler if needed:

```sh
make CC=/opt/amiga/bin/m68k-amigaos-gcc
```

## Project layout

- `src/` — Amiga-native implementation
- `include/` — public/internal headers
- `docs/` — architecture and milestone documentation
- `examples/` — ARexx examples as the interface grows

## Design principles

1. Keep the 68000/AmigaOS 2.04 baseline viable.
2. Treat ARexx as a first-class API, not an optional afterthought.
3. Keep IRC protocol/state handling separate from Amiga-specific adapters.
4. Prefer bounded memory structures suitable for classic hardware.
5. Make reconnect and session persistence core behavior of the bouncer.

See `ROADMAP.md` and `docs/ARCHITECTURE.md` for the planned progression.
