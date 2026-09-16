#!/usr/bin/env python3
"""Small deterministic IRC fixture for a visible AmigaOS M8.2 run.

This is deliberately not a full IRC daemon. It implements only the protocol
surface needed to qualify AmBNC and never logs PASS or AUTHENTICATE payloads.
"""

import argparse
import base64
import json
import signal
import socketserver
import ssl
import threading
import time


LOG_LOCK = threading.Lock()
CONNECTIONS = {}
OUTAGE_UNTIL = {}


def emit(event, network, **fields):
    record = {"time": int(time.time()), "event": event, "network": network}
    record.update(fields)
    with LOG_LOCK:
        print(json.dumps(record, sort_keys=True), flush=True)


class ThreadingServer(socketserver.ThreadingMixIn, socketserver.TCPServer):
    allow_reuse_address = True
    daemon_threads = True

    def __init__(self, address, handler, network, sasl_user, sasl_pass,
                 sasl_required=False, tls_context=None):
        self.network = network
        self.sasl_user = sasl_user
        self.sasl_pass = sasl_pass
        self.sasl_required = sasl_required
        self.tls_context = tls_context
        super().__init__(address, handler)

    def get_request(self):
        sock, address = super().get_request()
        if self.tls_context is not None:
            sock = self.tls_context.wrap_socket(sock, server_side=True)
        return sock, address


class IRCHandler(socketserver.StreamRequestHandler):
    def setup(self):
        super().setup()
        self.nick = None
        self.user = None
        self.cap_seen = False
        self.cap_ended = False
        self.sasl_ok = not self.server.sasl_required
        self.registered = False
        network = self.server.network
        with LOG_LOCK:
            CONNECTIONS[network] = CONNECTIONS.get(network, 0) + 1
            connection = CONNECTIONS[network]
            self.unavailable = time.time() < OUTAGE_UNTIL.get(network, 0)
        emit("connected", network, connection=connection,
             peer=self.client_address[0])
        if self.unavailable:
            emit("outage_reject", network, connection=connection)

    def send_line(self, line):
        self.wfile.write((line + "\r\n").encode("utf-8"))
        self.wfile.flush()

    def maybe_register(self):
        if self.registered or not self.nick or not self.user:
            return
        if self.cap_seen and not self.cap_ended:
            return
        if not self.sasl_ok:
            return
        self.registered = True
        self.send_line(":fixture 001 %s :M8.2 fixture registered" % self.nick)
        self.send_line("PING :M8_2_%s" % self.server.network.upper())
        emit("registered", self.server.network, nick=self.nick)

    def handle_authenticate(self, argument):
        if argument == "PLAIN":
            self.send_line("AUTHENTICATE +")
            emit("sasl_plain_requested", self.server.network)
            return
        try:
            decoded = base64.b64decode(argument.encode("ascii"), validate=True)
            expected = (self.server.sasl_user + "\0" + self.server.sasl_user +
                        "\0" + self.server.sasl_pass).encode("utf-8")
        except (ValueError, UnicodeError):
            decoded = b""
            expected = b"x"
        if decoded == expected:
            self.sasl_ok = True
            self.send_line(":fixture 903 %s :SASL authentication successful" %
                           (self.nick or "*"))
            emit("sasl_pass", self.server.network)
        else:
            self.send_line(":fixture 904 %s :SASL authentication failed" %
                           (self.nick or "*"))
            emit("sasl_fail", self.server.network)

    def handle_line(self, line):
        command, _, arguments = line.partition(" ")
        command = command.upper()
        network = self.server.network
        if command in ("PASS", "AUTHENTICATE"):
            emit("rx_redacted", network, command=command)
        else:
            emit("rx", network, line=line)

        if command == "CAP" and arguments.upper().startswith("LS"):
            self.cap_seen = True
            self.send_line(":fixture CAP * LS :sasl")
            emit("cap_ls", network)
        elif command == "CAP" and arguments.upper().startswith("REQ"):
            self.send_line(":fixture CAP * ACK :sasl")
            emit("cap_ack", network)
        elif command == "CAP" and arguments.upper().startswith("END"):
            self.cap_ended = True
            emit("cap_end", network)
            self.maybe_register()
        elif command == "AUTHENTICATE":
            self.handle_authenticate(arguments)
        elif command == "NICK":
            self.nick = arguments.lstrip(":").split(" ", 1)[0]
            self.maybe_register()
        elif command == "USER":
            self.user = arguments.split(" ", 1)[0]
            self.maybe_register()
        elif command == "PONG":
            emit("pong", network, token=arguments.lstrip(":"))
        elif command == "PING":
            self.send_line("PONG " + arguments)
            emit("raw_ping", network)
        elif command == "JOIN" and self.nick:
            channel = arguments.lstrip(":").split(" ", 1)[0]
            self.send_line(":%s!ambnc@fixture JOIN :%s" % (self.nick, channel))
            self.send_line(":fixture-user!test@fixture JOIN :%s" % channel)
            emit("join", network, channel=channel)
        elif command == "PART" and self.nick:
            channel = arguments.split(" ", 1)[0]
            self.send_line(":%s!ambnc@fixture PART %s :fixture part" %
                           (self.nick, channel))
            self.send_line(":fixture-user!test@fixture PART %s :fixture part" %
                           channel)
            emit("part", network, channel=channel)
        elif command in ("PRIVMSG", "NOTICE"):
            target, _, text = arguments.partition(" ")
            text = text.lstrip(":")
            if text == "M8_2_DROP_%s" % network.upper():
                with LOG_LOCK:
                    OUTAGE_UNTIL[network] = time.time() + 12
                emit("forced_drop", network)
                return False
            self.send_line(":fixture-user!test@fixture %s %s :ECHO %s" %
                           (command, target, text))
            emit(command.lower(), network, target=target, text=text)
        elif command == "QUIT":
            emit("quit", network)
            return False
        return True

    def handle(self):
        if self.unavailable:
            return
        self.send_line(":fixture NOTICE AUTH :AmBNC M8.2 deterministic fixture")
        while True:
            raw = self.rfile.readline(2048)
            if not raw:
                break
            line = raw.decode("utf-8", "replace").rstrip("\r\n")
            if not line:
                continue
            if not self.handle_line(line):
                break

    def finish(self):
        emit("disconnected", self.server.network,
             nick=self.nick or "unknown")
        super().finish()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--bind", default="0.0.0.0")
    parser.add_argument("--alpha-port", type=int, default=17667)
    parser.add_argument("--beta-tls-port", type=int, default=17668)
    parser.add_argument("--sasl-user", required=True)
    parser.add_argument("--sasl-pass", required=True)
    parser.add_argument("--cert", required=True)
    parser.add_argument("--key", required=True)
    args = parser.parse_args()

    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain(args.cert, args.key)
    alpha = ThreadingServer((args.bind, args.alpha_port), IRCHandler, "alpha",
                            args.sasl_user, args.sasl_pass, sasl_required=True)
    beta = ThreadingServer((args.bind, args.beta_tls_port), IRCHandler, "beta",
                           args.sasl_user, args.sasl_pass, tls_context=context)
    threads = [threading.Thread(target=server.serve_forever, daemon=True)
               for server in (alpha, beta)]
    for thread in threads:
        thread.start()
    emit("fixture_ready", "all", alpha_port=args.alpha_port,
         beta_tls_port=args.beta_tls_port)

    stopped = threading.Event()
    signal.signal(signal.SIGTERM, lambda *_: stopped.set())
    signal.signal(signal.SIGINT, lambda *_: stopped.set())
    while not stopped.wait(1):
        pass
    for server in (alpha, beta):
        server.shutdown()
        server.server_close()
    emit("fixture_stopped", "all")


if __name__ == "__main__":
    main()
