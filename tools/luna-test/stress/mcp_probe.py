#!/usr/bin/env python3
"""Drive luna's MCP server (`luna mcp`) over stdio — a transitory prototype to
validate the MCP debug surface end-to-end (luna v1.14.0, the third #168-#181
release ask). MCP stdio transport = newline-delimited JSON-RPC 2.0.

Usage: mcp_probe.py <rom.sfc> [--list]
  --list : handshake + tools/list + capabilities only (learn the catalogue).
"""
from __future__ import annotations

import json
import queue
import subprocess
import sys
import threading
from pathlib import Path

REPO = Path(__file__).resolve().parents[3]
LUNA = REPO / "tools" / "luna-test" / "bin" / "luna"


class MCP:
    def __init__(self, rom: str):
        self.p = subprocess.Popen(
            [str(LUNA), "mcp", "--rom", rom],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            text=True, bufsize=1)
        self.q: "queue.Queue[str]" = queue.Queue()
        threading.Thread(target=self._reader, daemon=True).start()
        self._id = 0

    def _reader(self):
        for line in self.p.stdout:
            self.q.put(line)

    def _send(self, obj: dict):
        self.p.stdin.write(json.dumps(obj) + "\n")
        self.p.stdin.flush()

    def request(self, method: str, params: dict | None = None, timeout=20):
        self._id += 1
        rid = self._id
        self._send({"jsonrpc": "2.0", "id": rid, "method": method,
                    "params": params or {}})
        # read until we see our id (skip notifications / other ids)
        import time
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            try:
                line = self.q.get(timeout=deadline - time.monotonic())
            except queue.Empty:
                break
            line = line.strip()
            if not line:
                continue
            try:
                msg = json.loads(line)
            except json.JSONDecodeError:
                continue
            if msg.get("id") == rid:
                return msg
        raise TimeoutError(f"no response to {method}")

    def notify(self, method: str, params: dict | None = None):
        self._send({"jsonrpc": "2.0", "method": method, "params": params or {}})

    def close(self):
        try:
            self.p.stdin.close()
            self.p.wait(timeout=5)
        except Exception:
            self.p.kill()


def main() -> int:
    rom = sys.argv[1]
    m = MCP(rom)
    init = m.request("initialize", {
        "protocolVersion": "2024-11-05",
        "capabilities": {},
        "clientInfo": {"name": "opensnes-mcp-probe", "version": "0.1"},
    })
    info = init.get("result", {}).get("serverInfo", {})
    print(f"server: {info.get('name')} v{info.get('version')}")
    m.notify("notifications/initialized")

    tl = m.request("tools/list")
    tools = tl.get("result", {}).get("tools", [])
    print(f"tools/list: {len(tools)} tools")
    for t in tools:
        print(f"  - {t['name']}")

    m.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
