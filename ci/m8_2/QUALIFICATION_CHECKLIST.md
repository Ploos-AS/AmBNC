# M8.2 local AmigaOS qualification record

OVERALL: PENDING

Do not change the overall result to PASS until every required row is PASS and
the referenced evidence is retained. Use `N/A` only for the explicitly optional
public-server cross-check. Never paste real passwords, SASL payloads, ROM names,
private server addresses, or private keys into this file.

## Environment

- Date (UTC):
- Source commit:
- AmBNC SHA-256:
- FS-UAE version:
- Emulated model/CPU:
- AmigaOS/Workbench version (must be 2.04+):
- `bsdsocket.library` provider/version:
- Host OS:
- Operator:

## Required matrix

| ID | Check | Result | Evidence file/screenshot and observation |
|---|---|---|---|
| Q01 | Visible FS-UAE and real AmigaOS 2.04+ boot | PENDING | |
| Q02 | 68000 Bebbo loadseg binary starts | PENDING | |
| Q03 | `bsdsocket.library` opens; alpha and beta register | PENDING | |
| Q04 | Fixture PING receives PONG on both networks | PENDING | |
| Q05 | Downstream listeners on 16667 and 16668 | PENDING | |
| Q06 | Downstream client attaches and detaches | PENDING | |
| Q07 | Upstream remains alive while client is detached | PENDING | |
| Q08 | Detached PRIVMSG is buffered and replayed on reattach | PENDING | |
| Q09 | Public `AMBNC` ARexx port responds to STATUS | PENDING | |
| Q10 | CONNECT and DISCONNECT | PENDING | |
| Q11 | JOIN, PART, MSG, NOTICE, and RAW | PENDING | |
| Q12 | BUFFER and documented no-op RELOAD | PENDING | |
| Q13 | QUIT causes clean shutdown and removes the ARexx port | PENDING | |
| Q14 | ON_CONNECT/ON_DISCONNECT and IRC event hooks run | PENDING | |
| Q15 | Both networks operate concurrently | PENDING | |
| Q16 | Forced alpha loss reconnects 1/2/4/.../30s; beta stays up | PENDING | |
| Q17 | CAP LS 302 / CAP END occurs | PENDING | |
| Q18 | Alpha SASL PLAIN succeeds without credential logging | PENDING | |
| Q19 | Beta `TLS_MODE=PROXY` crosses the host TLS relay | PENDING | |
| Q20 | Ctrl-C shutdown is clean (repeat after QUIT test if needed) | PENDING | |

## Optional cross-check

| ID | Check | Result | Evidence file/screenshot and observation |
|---|---|---|---|
| O01 | Operator-approved public IRC server/account | PENDING | |

## Evidence inventory

- `AmBNC.sha256` and `AmBNC.file.txt`
- `AmigaOS-Version.txt`, `bsdsocket-library.txt`, `rexx-hooks.txt`
- `host-fixture.jsonl` and `tls-proxy.jsonl`
- `rexx-commands.txt`, `rexx-reconnect.txt`, `rexx-quit.txt`, `hooks.log`
- downstream attach/backlog transcript with no credentials
- screenshots showing the visible FS-UAE window, startup, reconnect, and shutdown
- completed copy of this checklist

## Final decision

- Overall: PENDING
- Blocking failures or pending rows:
- Notes:
