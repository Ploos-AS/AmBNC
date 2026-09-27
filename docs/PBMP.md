# PBMP management adapter

AmBNC implements the PBMP/1 Endpoint Profile M0 as an optional management adapter.

## Separation from the BNC core

PBMP is not required to start, connect, reconnect, proxy IRC traffic, preserve backlog, or use ARexx. The adapter is a bounded request/response function in `src/pbmp.c`; transports call it but the IRC/BNC core does not depend on a PBMP transport.

## M0 methods

- `pbmp.info`
- `capabilities.list`
- `endpoint.info`

AmBNC reports endpoint kind `bouncer`.

### Endpoint runtime telemetry

`endpoint.info` also exposes the optional PBMP `uptime_seconds` field. It is
the number of elapsed whole seconds since the current AmBNC service instance
started. The value resets to `0` when AmBNC restarts and is runtime telemetry,
not wall-clock time or a persistent lifetime counter. Clients MUST remain
compatible with PBMP endpoints that omit this optional field.

## Transport

The PBMP reference qualification tool uses a local Unix-domain JSONL transport. Classic AmigaOS does not provide Unix-domain sockets as a portable baseline, so AmBNC does not embed that transport in its core.

For native Amiga deployment, a transport adapter MAY expose the same one-request/one-response PBMP messages over an Amiga-appropriate local IPC mechanism. Transport selection MUST NOT alter PBMP message semantics and MUST remain optional.

A host-side bridge MAY expose the reference Unix-domain JSONL transport and forward complete requests to the AmBNC adapter during automated conformance testing. Such a bridge is test/management infrastructure, not part of normal BNC operation.

## Security

Management transports should default to local-only access. A network-reachable bridge requires authentication, confidentiality, integrity protection, and explicit operator configuration.

## Qualification status

AmBNC is qualified against PBMP/1 Endpoint Profile M0.

The CI qualification pins the PBMP suite to revision
`ccf17aa46fa3801fe035513f025b835ca688235d` and exercises the real
`src/pbmp.c` adapter through a host-only Unix JSONL bridge. The required
Endpoint M0 methods are:

- `pbmp.info`
- `capabilities.list`
- `endpoint.info`

The qualification report is generated as
`build/pbmp-conformance-report.json` and uploaded with the workflow
qualification evidence.

The host bridge is test infrastructure only. AmBNC remains independently
functional when PBMP is disabled or unavailable, and the native Amiga build
does not depend on Unix-domain sockets, BotWeb, or BotAI.

## `networks.list`

AmBNC advertises `networks.list` as an optional PBMP capability in addition to
the Endpoint M0 requirements. It is a read-only view of the active
`ambnc_networks_config`.

Each configured network currently exposes:

- `id`: `network-N`, derived from the configuration order.
- `name`: the configured network section name.
- `state`: the current AmBNC runtime state: `configured`, `connecting`,
  `connected`, or `disconnected`.
- `retry_seconds`: remaining reconnect countdown in seconds. It is `0` when
  no reconnect countdown is active.
- `paused`: boolean operator-control state. `true` means automatic upstream
  connection/reconnection is intentionally paused; `false` means normal
  connection management is active.

The response intentionally does not expose upstream hosts, ports, nick/user
credentials, passwords, SASL secrets, or other connection secrets. The state is published by AmBNC's multinet runtime rather than inferred by
the PBMP adapter. `configured` means the network exists in configuration but
has not yet entered a connection attempt; `connecting` marks an active
attempt, `connected` means an upstream socket and IRC registration path have
been established, and `disconnected` means the upstream is down, including manual disconnects
and reconnect backoff.

`state` and `paused` describe different dimensions. In particular,
`disconnected` with `paused:false` means the network is eligible for normal
automatic connection/reconnection; a non-zero `retry_seconds` gives the live
backoff countdown. `disconnected` with `paused:true` means the operator has
intentionally stopped automatic connection attempts, so `retry_seconds` is
`0`. Resuming clears the pause condition and allows the normal connection
loop to proceed. A successful connection also has `retry_seconds:0`.

The pause flag is runtime control telemetry only. It does not reveal why an
operator paused a network and does not expose any connection configuration or
credentials.

When no multinet configuration has been bound, `networks.list` returns an
empty list rather than making PBMP a prerequisite for AmBNC startup.

