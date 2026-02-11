#!/usr/bin/env python3
"""
test_mssp.py - Test MSSP (MUD Server Status Protocol) responses from the MUD server.

Connects to the server, negotiates MSSP via telnet, parses the response,
and displays all MSSP variables received.

Usage:
    python test_mssp.py [host] [port]
    python test_mssp.py                  # defaults to localhost:8888
    python test_mssp.py localhost 9999
"""

import socket
import sys
import time

# Telnet protocol bytes
IAC  = 255  # Interpret As Command
WILL = 251
WONT = 252
DO   = 253
DONT = 254
SB   = 250  # Subnegotiation Begin
SE   = 240  # Subnegotiation End

# MSSP
TELOPT_MSSP = 70
MSSP_VAR = 1
MSSP_VAL = 2


def parse_mssp_subneg(data):
    """Parse MSSP subnegotiation data into a dict of variable -> [values]."""
    variables = {}
    current_var = None
    current_str = bytearray()
    state = "INIT"

    for byte in data:
        if byte == MSSP_VAR:
            # Save previous value if we were building one
            if state == "VAL" and current_var is not None:
                variables.setdefault(current_var, []).append(current_str.decode("utf-8", errors="replace"))
            current_str = bytearray()
            state = "VAR"
        elif byte == MSSP_VAL:
            if state == "VAR":
                current_var = current_str.decode("utf-8", errors="replace")
            elif state == "VAL" and current_var is not None:
                variables.setdefault(current_var, []).append(current_str.decode("utf-8", errors="replace"))
            current_str = bytearray()
            state = "VAL"
        else:
            current_str.append(byte)

    # Capture the last value
    if state == "VAL" and current_var is not None:
        variables.setdefault(current_var, []).append(current_str.decode("utf-8", errors="replace"))

    return variables


def test_mssp(host="localhost", port=8888):
    """Connect to the MUD, negotiate MSSP, and display the response."""
    print(f"Connecting to {host}:{port}...")

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(10)

    try:
        sock.connect((host, port))
    except (ConnectionRefusedError, socket.timeout) as e:
        print(f"Connection failed: {e}")
        return False

    print("Connected. Waiting for telnet negotiation...\n")

    # Collect data in chunks
    raw = bytearray()
    mssp_offered = False
    mssp_data = None
    deadline = time.time() + 5

    while time.time() < deadline:
        try:
            chunk = sock.recv(4096)
            if not chunk:
                break
            raw.extend(chunk)
        except socket.timeout:
            pass

        # Scan for IAC WILL MSSP and respond with IAC DO MSSP
        if not mssp_offered:
            for i in range(len(raw) - 2):
                if raw[i] == IAC and raw[i + 1] == WILL and raw[i + 2] == TELOPT_MSSP:
                    print("Server offered MSSP (IAC WILL MSSP)")
                    print("Sending IAC DO MSSP...\n")
                    sock.sendall(bytes([IAC, DO, TELOPT_MSSP]))
                    mssp_offered = True
                    deadline = time.time() + 5  # Reset timeout for response
                    break

        # Scan for MSSP subnegotiation: IAC SB MSSP ... IAC SE
        if mssp_offered and mssp_data is None:
            i = 0
            while i < len(raw) - 3:
                if raw[i] == IAC and raw[i + 1] == SB and raw[i + 2] == TELOPT_MSSP:
                    # Find the end: IAC SE
                    for j in range(i + 3, len(raw) - 1):
                        if raw[j] == IAC and raw[j + 1] == SE:
                            mssp_data = raw[i + 3:j]
                            break
                    if mssp_data is not None:
                        break
                i += 1

        if mssp_data is not None:
            break

    sock.close()

    if not mssp_offered:
        print("FAIL: Server did not offer MSSP (no IAC WILL MSSP received)")
        return False

    if mssp_data is None:
        print("FAIL: Server did not send MSSP subnegotiation data")
        return False

    # Parse and display
    variables = parse_mssp_subneg(mssp_data)

    if not variables:
        print("FAIL: MSSP subnegotiation was empty")
        return False

    print(f"MSSP Response ({len(variables)} variables):")
    print("=" * 50)

    # Check required variables
    required = ["NAME", "PLAYERS", "UPTIME"]
    missing_required = [r for r in required if r not in variables]

    max_key_len = max(len(k) for k in variables)
    for var, vals in variables.items():
        tag = ""
        if var in required:
            tag = " [REQUIRED]"
        if len(vals) == 1:
            print(f"  {var:<{max_key_len}}  {vals[0]}{tag}")
        else:
            print(f"  {var:<{max_key_len}}  {vals[0]}{tag}")
            for v in vals[1:]:
                print(f"  {'':<{max_key_len}}  {v}")

    print("=" * 50)

    # Validation
    print("\nValidation:")
    ok = True

    if missing_required:
        print(f"  FAIL: Missing required variables: {', '.join(missing_required)}")
        ok = False
    else:
        print(f"  OK: All required variables present ({', '.join(required)})")

    if "UPTIME" in variables:
        try:
            uptime_val = int(variables["UPTIME"][0])
            if uptime_val > 1000000000:
                print(f"  OK: UPTIME looks like a Unix timestamp ({uptime_val})")
            else:
                print(f"  WARN: UPTIME={uptime_val} looks like elapsed seconds, not a Unix timestamp")
        except ValueError:
            print(f"  FAIL: UPTIME is not numeric: {variables['UPTIME'][0]}")
            ok = False

    # Check for multi-value variables
    multi = {k: v for k, v in variables.items() if len(v) > 1}
    if multi:
        print(f"  INFO: Multi-value variables: {', '.join(f'{k} ({len(v)} values)' for k, v in multi.items())}")

    print(f"\n{'PASS' if ok else 'FAIL'}: MSSP test {'passed' if ok else 'failed'}")
    return ok


if __name__ == "__main__":
    host = sys.argv[1] if len(sys.argv) > 1 else "localhost"
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 8888
    success = test_mssp(host, port)
    sys.exit(0 if success else 1)
