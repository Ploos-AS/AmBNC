# PBMP management adapter

AmBNC implements the PBMP/1 Endpoint Profile M0 as an optional management adapter.

## Separation from the BNC core

PBMP is not required to start, connect, reconnect, proxy IRC traffic, preserve backlog, or use ARexx. The adapter is a bounded request/response function in `src/pbmp.c`; transports call it but the IRC/BNC core does not depend on a PBMP transport.

## M0 methods

- `pbmp.info`
- `capabilities.list`
- `endpoint.info`

AmBNC reports endpoint kind `bouncer`.

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
- `state`: currently `configured`.

The response intentionally does not expose upstream hosts, ports, nick/user
credentials, passwords, SASL secrets, or other connection secrets. PBMP
consumers MUST NOT infer that a `configured` network is currently connected;
runtime connection-state reporting may be added separately in a later
capability revision.

When no multinet configuration has been bound, `networks.list` returns an
empty list rather than making PBMP a prerequisite for AmBNC startup.

