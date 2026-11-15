"""The test server of a Web frontend's dist/ (LIB-15 design §2; spec AC-13).

    python3 serve.py <dir> <port> [<prefix>]

Serves <dir> at the server's root, or, with a prefix such as
/anything/calc, only below it (everything else a 404): the sub-path that
shows that dist/ works wherever it is hosted. From both products'
run-tests.sh, which were identical but for the .wasm type.
"""
import functools
import http.server
import sys

DIRECTORY = sys.argv[1]
PORT = int(sys.argv[2])
PREFIX = sys.argv[3] if len(sys.argv) > 3 else ""


class Handler(http.server.SimpleHTTPRequestHandler):
    extensions_map = {
        **http.server.SimpleHTTPRequestHandler.extensions_map,
        ".wasm": "application/wasm",
        ".js": "text/javascript",
        ".json": "application/json",
    }

    def translate_path(self, path):
        if PREFIX:
            if not path.startswith(PREFIX + "/"):
                return "/nonexistent"
            path = path[len(PREFIX):]
        return super().translate_path(path)

    def end_headers(self):
        self.send_header("Cache-Control", "no-store")
        super().end_headers()

    def log_message(self, *args):
        pass


class Server(http.server.ThreadingHTTPServer):
    request_queue_size = 128
    daemon_threads = True

    def handle_error(self, request, client_address):
        pass   # a page that goes away mid-response (a closed tab) is no error here


Server(("", PORT), functools.partial(Handler, directory=DIRECTORY)).serve_forever()
