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
