#!/usr/bin/env python3
"""
Simple HTTP server for the Dystopia MUD Map Viewer.

Usage: python serve.py [port]
Default port: 8080

This avoids CORS issues when loading JSON data files.
"""

import http.server
import socketserver
import sys
import webbrowser
from pathlib import Path

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 8080

# Change to the webmap directory
webmap_dir = Path(__file__).parent
import os
os.chdir(webmap_dir)

Handler = http.server.SimpleHTTPRequestHandler

# Add CORS headers
class CORSHandler(Handler):
    def end_headers(self):
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET')
        self.send_header('Cache-Control', 'no-store, no-cache, must-revalidate')
        super().end_headers()

    def log_message(self, format, *args):
        # Quieter logging
        if '200' not in str(args):
            print(f"{args[0]}")

with socketserver.TCPServer(("", PORT), CORSHandler) as httpd:
    url = f"http://localhost:{PORT}"
    print(f"Serving Dystopia MUD Map at {url}")
    print(f"Press Ctrl+C to stop\n")

    # Try to open browser
    try:
        webbrowser.open(url)
    except Exception:
        pass

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nServer stopped.")
