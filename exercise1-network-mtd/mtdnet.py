#!/usr/bin/env python3
"""
Network-level MTD (URL / port shuffling) demo web application.

A tiny web server whose listening port can be changed by the client at runtime:

    GET /                       -> normal response
    GET /restart?port=<NEW>     -> respond, then move to port <NEW> for the next access
    GET /shutdown               -> respond, then stop the server

This is the "moving target": the network identifier (port number) keeps changing,
so an attacker probing a fixed port quickly loses the target, while a legitimate
client that knows the shuffling algorithm can always follow it.

Updated 2026-07-15 from the original gist (mtdnet2.py) to run cleanly on
Python 3.8 - 3.13 (Content-Length is now sent as bytes, threads are daemonic,
and shutdown is handled without leaking threads).

Usage:
    python3 mtdnet.py [initial_port]      # default initial port: 8123
"""

import sys
from urllib.parse import urlparse, parse_qs
from http.server import BaseHTTPRequestHandler, HTTPServer

DEFAULT_PORT = 8123


class MyHandler(BaseHTTPRequestHandler):
    # Shared counter across all handler instances / server generations.
    num = 1000

    # Set by MainServer so the handler can ask the server to move / stop.
    controller = None

    def _write_body(self):
        mesg = (
            "<html><body>\n"
            "<h3>MTD demo response</h3>\n"
            f"This is a response. cnt={MyHandler.num}\n"
            "</body></html>\n"
        )
        MyHandler.num += 1
        body = mesg.encode("utf-8")
        self.send_response(200)
        self.send_header("Content-type", "text/html; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        parsed = urlparse(self.path)
        queries = parse_qs(parsed.query)
        self._write_body()

        if parsed.path == "/restart":
            try:
                next_port = int(queries.get("port", [None])[0])
            except (TypeError, ValueError):
                return
            # Move the target: ask the controller to rebind on the new port.
            MyHandler.controller.request_move(next_port)
        elif parsed.path == "/shutdown":
            MyHandler.controller.request_stop()

    # Silence default noisy logging; comment out to see requests.
    def log_message(self, fmt, *args):
        sys.stderr.write("[mtd] %s - %s\n" % (self.address_string(), fmt % args))


class MainServer:
    """Single-threaded controller: serves on one port, then rebinds on the
    next port when a client asks to move. Deterministic, no race conditions."""

    def __init__(self, port=DEFAULT_PORT):
        self.port = port
        self._next_port = None
        self._running = True
        self._active = True

    def request_move(self, next_port):
        """Called from a request handler: rebind on next_port after this request."""
        self._next_port = next_port
        self._active = False

    def request_stop(self):
        self._running = False
        self._active = False

    def run_forever(self, port):
        self.port = port
        while self._running:
            self._active = True
            self._next_port = None
            httpd = HTTPServer(("0.0.0.0", self.port), MyHandler)
            httpd.timeout = 0.5  # handle_request() returns after this if idle
            MyHandler.controller = self
            sys.stderr.write(f"[mtd] listening on port {self.port}\n")
            try:
                # Serve requests until a move/stop is requested. handle_request()
                # blocks up to httpd.timeout so the flags are checked regularly.
                while self._active and self._running:
                    httpd.handle_request()
            finally:
                httpd.server_close()
            if self._next_port is not None:
                self.port = self._next_port


def main():
    port = DEFAULT_PORT
    if len(sys.argv) > 1:
        port = int(sys.argv[1])
    try:
        MainServer().run_forever(port)
    except KeyboardInterrupt:
        sys.stderr.write("\n[mtd] bye\n")


if __name__ == "__main__":
    main()
