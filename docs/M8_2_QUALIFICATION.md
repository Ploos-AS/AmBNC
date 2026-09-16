# M8.2 — Local AmigaOS runtime qualification

Status: **PENDING**

The reproducible harness is present, but no visible AmigaOS runtime result is
recorded in this repository. Host checks, compilation, loadseg recognition, and
the M8.1 AROS run do not make M8.2 pass.

## Qualification target

- real AmigaOS/Workbench 2.04 or newer, not AROS
- classic 68k system or visible FS-UAE; the supplied profile uses A1200
- AmBNC built by Bebbo `m68k-amigaos-gcc` with `-m68000 -mcrt=nix20`
- a live `bsdsocket.library` implementation
- `rexxsyslib.library`, RexxMast, and the public `AMBNC` port
- two simultaneous IRC sessions, downstream clients, CAP, SASL PLAIN, and an
  external plain-to-TLS proxy

The FS-UAE profile enables its UAE `bsdsocket.library` emulation. An installed
Roadshow, AmiTCP, or Miami stack may be used instead, but the provider and
version must be recorded. Do not run both providers at once. FS-UAE documents
the [`bsdsocket_library` option](https://fs-uae.net/docs/options/bsdsocket-library/)
and supports mounting a host folder as an Amiga drive through
[`hard_drive_0`/`hard_drive_1`](https://fs-uae.net/docs/options/hard-drive-0/).

No Kickstart, Workbench, TCP/IP stack, IRC account, or other proprietary
runtime material is included. Supply only material you are licensed to use.

## What the harness provides

`make qualify-m8_2` runs static checks, creates the Bebbo `-m68000` binary, and
prepares an ignored `build/m8_2/` tree containing:

- a generated visible FS-UAE configuration;
- the AmBNC binary and its `file(1)` and SHA-256 evidence;
- a deterministic two-network AmBNC configuration;
- AmigaDOS setup/start/snapshot scripts;
- ARexx command, reconnect, backlog, hook, and shutdown scripts;
- a two-network host IRC fixture;
- a separate plain-listener/TLS-upstream relay for `TLS_MODE=PROXY`;
- a blank qualification checklist that starts as `PENDING`.

Alpha requires CAP and SASL PLAIN. Its generated fixture credential exists only
under ignored `build/m8_2/`. Beta uses CAP without SASL and reaches a TLS IRC
fixture through the external relay. The fixture redacts `PASS` and
`AUTHENTICATE` payloads.

## Host prerequisites

- Docker or another compatible `docker` CLI for the pinned Bebbo image
- `file`, `sha256sum`, Python 3, OpenSSL, FS-UAE, and a graphical desktop
- a licensed bootable AmigaOS 2.04+ system directory or HDF
- a matching licensed Kickstart ROM
- an Amiga IRC client capable of connecting to `127.0.0.1` (for the downstream
  test); do not add that third-party client to this repository

Find a host address reachable from the emulated Amiga. With FS-UAE socket
emulation this is normally a LAN address of the host. Do not use
`HOST_IP_HERE`, and allow host TCP ports 17667 and 17669 locally if a firewall
blocks them.

## Build and prepare

From the repository root:

```sh
make check
make amiga
make qualify-m8_2 \
  M8_2_HOST=192.168.1.10 \
  M8_2_SYSTEM_DIR=/absolute/path/to/your/bootable-amigaos-system \
  M8_2_KICKSTART_FILE=/absolute/path/to/your/kickstart.rom
```

Replace all three sample values. A system HDF may be used in place of a
directory if supported by the local FS-UAE configuration. Paths and secrets are
written only below ignored `build/`. To choose the local, disposable fixture
identity explicitly, set `M8_2_SASL_USER` and `M8_2_SASL_PASS` in the
environment; allowed characters are alphanumeric, `_`, `.`, and `-`.

`make amiga` uses the same pinned Bebbo image as M8.1. `make qualify-m8_1` is
available to rerun the complete M8.1 native and AROS gates separately.

## Start the host endpoints

In terminal 1:

```sh
ci/m8_2/run-host-fixture.sh build/m8_2
```

This generates a two-day self-signed fixture certificate and private key under
ignored `build/m8_2/host/tls/`. It starts:

| Port | Service |
|---:|---|
| 17667 | alpha plain IRC fixture with SASL required |
| 17668 | loopback TLS IRC fixture for beta |
| 17669 | beta plain listener forwarding over TLS to 17668 |

Leave the process running. Its JSON Lines logs are written directly to the
shared evidence directory. Do not publish `build/m8_2/host/`.

## Visible FS-UAE and AmigaOS setup

In terminal 2:

```sh
fs-uae build/m8_2/AmBNC-M8_2.fs-uae
```

Do not wrap this in Xvfb, `timeout`, or a headless runner. Confirm in the visible
window that the real AmigaOS installation boots and that the mounted `AMBNCQ:`
volume is available. Start RexxMast if it is not already running.

Open an AmigaShell and run:

```text
Execute AMBNCQ:Setup-M8_2
Execute AMBNCQ:Start-M8_2
```

Keep the AmBNC Shell visible. Expected startup observations include the
`0.7.0-m7` banner, two downstream ports, the `AMBNC` port, alpha and beta
connection messages, the beta external-proxy notice, CAP traffic, both 001
registrations, and PONG traffic in the host evidence log. A failure to open
`bsdsocket.library` is a qualification failure, not a reason to skip networking.

## Runtime procedure

Use a second AmigaShell for these steps and retain its output.

### 1. ARexx commands and hooks

```text
RX AMBNCQ:rexx/Commands-M8_2.rexx >AMBNCQ:evidence/rexx-commands.txt
```

Expected final marker: `M8_2_REXX_COMMANDS=PASS`. This exercises STATUS,
CONNECT, DISCONNECT, JOIN, PART, MSG, NOTICE, RAW, BUFFER, and RELOAD. RELOAD is
currently an accepted no-op and must be recorded as such, not as dynamic reload
functionality. The host log must show the corresponding protocol lines, and
`AMBNCQ:evidence/hooks.log` must show lifecycle and IRC event hooks.

### 2. Downstream attach, detach, persistence, and replay

Configure an Amiga-side IRC client with no password:

- alpha: server `127.0.0.1`, port `16667`
- beta: server `127.0.0.1`, port `16668`

Attach to alpha and capture the synthetic 001. Send a harmless message, then
detach by sending QUIT or closing the client. In the second Shell, confirm
`STATUS` says `downstream=DETACHED`, then run:

```text
RX AMBNCQ:rexx/Backlog-M8_2.rexx >AMBNCQ:evidence/rexx-backlog.txt
```

Expected final marker: `M8_2_BACKLOG_COLLECTION=PASS`. The fixture echoes the
message while no client is attached, proving that the upstream is still usable.
Reattach to alpha. Capture the `Backlog target=`, `Backlog stamp=`, and original
`ECHO M8_2_BACKLOG_ALPHA` line. Confirm a following BUFFER reports zero replayed
lines. Repeat a basic attach/detach on beta and record both listener results.

### 3. Independent bounded reconnect

```text
RX AMBNCQ:rexx/Drop-Alpha-M8_2.rexx >AMBNCQ:evidence/rexx-reconnect.txt
```

The fixture drops alpha and rejects its reconnects for 12 seconds. Observe
alpha retries after approximately 1, 2, 4, and 8 seconds, followed by a new
registration. During this interval beta must remain registered and must not
reconnect. The fixture timestamps and connection numbers are evidence; the
source backoff is bounded at 30 seconds.

### 4. Clean QUIT and Ctrl-C

Run QUIT last:

```text
RX AMBNCQ:rexx/Quit-M8_2.rexx >AMBNCQ:evidence/rexx-quit.txt
Execute AMBNCQ:Snapshot-M8_2
```

Expected marker: `M8_2_REXX_QUIT_REQUEST=PASS`, followed by `AmBNC:
multi-session stopped` in the visible Shell. STATUS must no longer find the
`AMBNC` port. Repeat startup once and stop with Ctrl-C to qualify that shutdown
path too.

Stop FS-UAE visibly through its UI only after the evidence files are flushed.
Then stop the host fixture with Ctrl-C.

## Expected host evidence

Run:

```sh
make review-m8_2-evidence
```

The helper checks fixture registration, PONG, CAP completion, SASL, reconnect,
TLS relay, ARexx markers, hook output, and credential redaction. Its
`OVERALL=PENDING` is intentional: it cannot inspect the visible window,
Amiga-side client transcript, OS/library identity, screenshots, or operator
judgement.

Useful non-secret fixture events include:

- `registered`, `pong`, `cap_ls`, `cap_ack`, and `cap_end` for both networks;
- `sasl_pass` for alpha;
- `tls_proxy_connected` with a negotiated TLS version for beta;
- `forced_drop` and several `outage_reject` events for alpha;
- no beta reconnect during the alpha-only outage.

## Optional public IRC cross-check

The deterministic fixture is a live IRC server over the Amiga socket API and is
the reproducible qualification path. A public IRC cross-check is optional and
must comply with the network's policy. Copy the generated config outside Git,
change host/port/nick, and use a dedicated test account if SASL is desired. Put
TLS termination on the host or LAN and point AmBNC's `TLS_MODE=PROXY` network
at its plain listener. Never commit or paste server passwords, SASL credentials,
private hostnames, proxy keys, or unredacted logs.

## Qualification matrix

All runtime rows remain PENDING until the visible run is completed.

| Area | Required evidence | Current state |
|---|---|---|
| startup and clean shutdown | visible startup plus QUIT and Ctrl-C screenshots | PENDING |
| AmigaOS 2.04+ / 68000 binary | version capture, build flags, loadseg evidence | PENDING |
| live `bsdsocket.library` / IRC / PING | visible run plus fixture registration and PONG | PENDING |
| disconnect / bounded reconnect | alpha retry timestamps and beta continuity | PENDING |
| downstream listeners / attach / detach | Amiga IRC client transcript | PENDING |
| detached upstream / backlog replay | BUFFER results and replay transcript | PENDING |
| `AMBNC` and all ARexx commands | three ARexx transcripts and removed port after QUIT | PENDING |
| ARexx event hooks | `hooks.log` with lifecycle and IRC events | PENDING |
| multi-network independence | simultaneous registration and alpha-only outage | PENDING |
| IRCv3 CAP / SASL PLAIN | redacted fixture events | PENDING |
| `TLS_MODE=PROXY` | beta proxy TLS version and registration | PENDING |

The editable, itemized record is copied to
`build/m8_2/shared/evidence/QUALIFICATION_CHECKLIST.md`. Its final decision is
authoritative only when every required row is backed by evidence.

## Known limitations

- The harness cannot automate or attest to a human-visible FS-UAE session.
- An Amiga IRC client is not redistributed; downstream UI behavior is manual.
- `RELOAD` remains a documented accepted no-op.
- Multi-network ARexx status is aggregate, and send commands target the first
  connected network; this is why the forced drop targets alpha before it falls.
- The fixture covers the protocol deterministically but is not a production IRC
  daemon. Public-service behavior can be checked separately without changing
  the required deterministic evidence.
- The generated certificate is self-signed and exists only to prove that the
  external proxy's upstream leg uses TLS.

## Final state

M8.2 is **PENDING**. Do not start M8.3 or change this document/roadmap to PASS
until the completed checklist and redacted evidence from a visible real
AmigaOS run have been reviewed.
