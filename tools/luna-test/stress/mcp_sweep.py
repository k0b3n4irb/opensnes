#!/usr/bin/env python3
"""Exhaustive sweep of luna's MCP tool catalogue (luna v1.14.0, #168-#181).

Calls EVERY tool at least once, in a dependency-aware order (setup -> action ->
observe -> mutate -> cleanup), with schema-correct args, and reports OK / ERROR
/ not-attempted per tool. A one-time validation instrument (transitory), not a
standing regression. Usage: mcp_sweep.py <rom.sfc>
"""
from __future__ import annotations

import base64
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from mcp_probe import MCP  # noqa: E402

results: dict[str, tuple[str, str]] = {}   # name -> (status, detail)


def main() -> int:
    rom = sys.argv[1]
    m = MCP(rom)
    m.request("initialize", {"protocolVersion": "2024-11-05", "capabilities": {},
                             "clientInfo": {"name": "sweep", "version": "0.1"}})
    m.notify("notifications/initialized")
    catalogue = [t["name"] for t in m.request("tools/list")["result"]["tools"]]
    sym = str(Path(rom).with_suffix(".sym"))

    def call(name, args=None, timeout=30):
        try:
            r = m.request("tools/call", {"name": name, "arguments": args or {}},
                          timeout=timeout)
            res = r.get("result", {})
            txt = "".join(c.get("text", "") for c in res.get("content", [])
                          if c.get("type") == "text")
            if res.get("isError") or ("error" in r):
                results[name] = ("ERROR", (txt or json.dumps(r.get("error", "")))[:120])
            else:
                results[name] = ("OK", txt[:80].replace("\n", " "))
            return txt
        except Exception as e:
            results[name] = ("ERROR", f"{type(e).__name__}: {str(e)[:100]}")
            return ""

    def jget(txt, key, default=None):
        try:
            return json.loads(txt).get(key, default)
        except Exception:
            return default

    # ---- SETUP ----
    call("capabilities")
    call("load_rom", {"path": rom})
    call("load_symbols", {"path": sym, "space": "cpu"})
    call("resolve_symbol", {"name": "current_song"})
    call("symbol_for_addr", {"addr": 0xE0, "space": "cpu"})
    for t in ["cpu", "dma", "dsp", "spc", "superfx", "sa1"]:
        call(f"enable_{t}_trace", {"max_events": 16})
    call("enable_dsp1_trace", {"max_events": 16, "ports_only": False})
    call("enable_mem_trace", {"max_events": 16})
    for lg in ["mailbox", "nocash", "wdm", "sa1", "sa1_side"]:
        call(f"enable_{lg}_log")
    call("enable_call_stack", {"enabled": True})
    bp_txt = call("bp_add", {"kind": "exec", "addr": 0x8000,
                             "on_read": False, "on_write": False})
    bp_id = jget(bp_txt, "id", 0)
    call("bp_list")
    call("bp_set_enabled", {"id": bp_id, "enabled": False})
    call("freeze_add", {"symbol": "current_song", "value": 1})
    call("freeze_list")
    call("search_begin", {"width": "u8"})
    call("start_input_capture")
    call("set_native_capture", {"enabled": False})

    # ---- ACTION ----
    call("run", {"max_steps": 2_000_000})
    call("step", {"count": 10})
    call("step_until_frame", {"max_steps": 200_000})
    call("run_until_pc", {"pc": 0x8000, "max_steps": 500_000})
    call("run_until_mem_read", {"symbol": "current_song", "max_steps": 500_000})
    call("run_until_mem_write", {"symbol": "current_song", "max_steps": 500_000})
    call("run_until_break", {"max_steps": 300_000})
    call("loop_probe", {"max_steps": 500_000})
    call("drain_audio", {"max": 1000})

    # ---- OBSERVE ----
    call("state")
    call("peek_memory", {"symbol": "current_song", "count": 1})
    call("peek_aram", {"offset": 0, "count": 4})
    call("peek_vram", {"offset": 0, "count": 4})
    call("peek_cgram")
    call("peek_oam")
    call("take_cpu_trace")
    call("take_dma_trace")
    call("take_dsp_trace")
    call("take_dsp1_trace", {"decode_commands": True})
    call("take_mem_trace")
    call("take_spc_trace")
    call("take_superfx_trace")
    call("take_sa1_trace")
    call("take_sa1_log")
    call("take_sa1_side_log")
    call("take_mailbox_log")
    call("take_nocash_log")
    call("take_wdm_log")
    call("call_stack")
    call("search_refine", {"op": "eq", "value": 0})
    call("search_results", {"limit": 8})
    call("search_memory", {"pattern": [0xDE, 0xC0]})   # #167 $7F-hit fix
    call("take_input_capture")
    call("frame_hash", {"force_display": False, "native": False})
    call("wram_page_hashes", {"page_size": 4096})
    call("wram_snapshot", {"include_data": False})
    call("disasm_cpu", {"addr": 0x8000, "lines": 4})
    call("disasm_spc", {"addr": 0x0000, "lines": 4})
    call("render_tilemap", {"bg": 0})
    call("render_vram_tiles", {"bpp": 4, "palette_row": 0})
    call("render_palette", {"cell": 0})
    call("render_sprite_sheet")
    call("decode_sprites")
    call("screenshot", {"force_display": True, "native": False})
    call("export_spc")
    sram_txt = call("sram_get")
    save_txt = call("save_state")

    # ---- MUTATE ----
    call("poke_memory", {"symbol": "current_song", "data": [0x02]})
    call("poke_aram", {"offset": 0x200, "data": [0x00]})
    call("poke_vram", {"offset": 0, "data": [0x00, 0x00]})
    call("poke_cgram", {"offset": 0, "data": [0x00, 0x00]})
    call("poke_oam", {"offset": 0, "data": [0x00]})
    call("set_cpu_register", {"reg": "a", "val": 0x1234})
    call("set_joypad", {"mask": 0, "port": 1})
    call("set_mouse", {"dx": 0, "dy": 0, "buttons": 0})
    call("set_superscope", {"x": 100, "y": 80, "buttons": 0})
    call("set_port_device", {"device": "joypad", "port": 2})
    sb = jget(sram_txt, "sram_base64") or base64.b64encode(b"\x00" * 8).decode()
    call("sram_set", {"sram_base64": sb})
    st = jget(save_txt, "state_base64")
    if st:
        call("load_state", {"state_base64": st})
    rom_b64 = base64.b64encode(Path(rom).read_bytes()).decode()
    call("load_rom_bytes", {"rom_base64": rom_b64}, timeout=60)

    # ---- CLEANUP ----
    call("bp_remove", {"id": bp_id})
    call("bp_clear_all")
    call("freeze_remove", {"symbol": "current_song"})
    # load_symbols_str REPLACES the table, so test it after the symbol-dependent
    # ops, feeding the real .sym text (expect the same count as the file load).
    call("load_symbols_str", {"text": Path(sym).read_text(), "space": "cpu"})
    call("clear_symbols")
    call("pause")
    call("reset")

    m.close()

    # ---- REPORT ----
    attempted = set(results)
    missing = [t for t in catalogue if t not in attempted]
    errs = {k: v for k, (s, v) in results.items() if s == "ERROR"}
    ok = sum(1 for s, _ in results.values() if s == "OK")
    print(f"\nMCP sweep: {ok}/{len(catalogue)} OK, {len(errs)} error, "
          f"{len(missing)} not attempted")
    if errs:
        print("\nERRORS / anomalies:")
        for k in sorted(errs):
            print(f"  {k}: {errs[k]}")
    if missing:
        print("\nNOT ATTEMPTED:", ", ".join(missing))
    return 0 if not errs and not missing else 1


if __name__ == "__main__":
    raise SystemExit(main())
