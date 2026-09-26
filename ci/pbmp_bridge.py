#!/usr/bin/env python3
"""Unix JSONL bridge to the host-built AmBNC PBMP adapter test driver."""

import argparse
import os
import socket
import subprocess


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--socket", required=True)
    parser.add_argument("--driver", required=True)
    parser.add_argument("--requests", type=int, default=3)
    args = parser.parse_args()

    try:
        os.unlink(args.socket)
    except FileNotFoundError:
        pass

    server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    server.bind(args.socket)
    server.listen(1)
    try:
        for _ in range(args.requests):
            conn, _ = server.accept()
            with conn:
                data = b""
                while b"\n" not in data:
                    chunk = conn.recv(4096 - len(data))
                    if not chunk:
                        break
                    data += chunk
                    if len(data) >= 4096:
                        break
                proc = subprocess.run(
                    [args.driver],
                    input=data,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    check=False,
                )
                if proc.returncode != 0:
                    raise SystemExit(proc.stderr.decode("utf-8", "replace"))
                conn.sendall(proc.stdout)
    finally:
        server.close()
        try:
            os.unlink(args.socket)
        except FileNotFoundError:
            pass


if __name__ == "__main__":
    main()
