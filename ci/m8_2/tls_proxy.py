#!/usr/bin/env python3
"""Plain-listener to TLS-upstream relay used for TLS_MODE=PROXY qualification."""

import argparse
import json
import select
import socket
import socketserver
import ssl
import time


def emit(event, **fields):
    record = {"time": int(time.time()), "event": event, "network": "beta"}
    record.update(fields)
    print(json.dumps(record, sort_keys=True), flush=True)


class ProxyServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    allow_reuse_address = True
    daemon_threads = True


class ProxyHandler(socketserver.BaseRequestHandler):
    def handle(self):
        context = ssl.create_default_context()
        context.check_hostname = False
        context.verify_mode = ssl.CERT_NONE
        raw = socket.create_connection((self.server.upstream_host,
                                        self.server.upstream_port), timeout=10)
        upstream = context.wrap_socket(raw, server_hostname="ambnc-m8-2-fixture")
        emit("tls_proxy_connected", peer=self.client_address[0],
             tls_version=upstream.version())
        sockets = [self.request, upstream]
        try:
            while True:
                readable, _, _ = select.select(sockets, [], [], 30)
                if not readable:
                    continue
                for source in readable:
                    data = source.recv(4096)
                    if not data:
                        return
                    target = upstream if source is self.request else self.request
                    target.sendall(data)
        finally:
            upstream.close()
            emit("tls_proxy_disconnected", peer=self.client_address[0])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--bind", default="0.0.0.0")
    parser.add_argument("--listen-port", type=int, default=17669)
    parser.add_argument("--upstream-host", default="127.0.0.1")
    parser.add_argument("--upstream-port", type=int, default=17668)
    args = parser.parse_args()
    server = ProxyServer((args.bind, args.listen_port), ProxyHandler)
    server.upstream_host = args.upstream_host
    server.upstream_port = args.upstream_port
    emit("tls_proxy_ready", listen_port=args.listen_port,
         upstream_port=args.upstream_port)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
        emit("tls_proxy_stopped")


if __name__ == "__main__":
    main()
