-- Debug script to inspect PPU state after hello_world runs

-- Run for 120 frames to let initialization complete
for i = 1, 120 do
    emu.frameAdvance()
end

emu.log("=== PPU Debug Info ===")

-- Get PPU state
local state = emu.getState()
local ppu = state.ppu

emu.log("Screen On: " .. tostring(not ppu.forcedVblank))
emu.log("Brightness: " .. ppu.brightness)
emu.log("BG Mode: " .. ppu.bgMode)

-- Check main screen layers
emu.log("Main Screen Layers: " .. string.format("0x%02X", ppu.mainScreenLayers))

-- Check VRAM - read first few words where font should be
emu.log("")
emu.log("=== VRAM at $0000 (Font Tile 0 - D) ===")
for i = 0, 7 do
    local addr = i * 2  -- word address i = byte address i*2
    local lo = emu.read(addr, emu.memType.snesVram)
    local hi = emu.read(addr + 1, emu.memType.snesVram)
    emu.log(string.format("  Word %d: $%02X%02X", i, hi, lo))
end

-- Check tilemap at $0800 - position (10, 14) = $0800 + 14*32 + 10 = $09CA
emu.log("")
emu.log("=== Tilemap at $0800 ===")
local tilemap_base = 0x0800 * 2  -- word $0800 = byte $1000
for row = 13, 15 do
    local row_str = "Row " .. row .. ":"
    for col = 9, 22 do
        local addr = tilemap_base + (row * 32 + col) * 2
        local lo = emu.read(addr, emu.memType.snesVram)
        local hi = emu.read(addr + 1, emu.memType.snesVram)
        row_str = row_str .. string.format(" %02X", lo)
    end
    emu.log(row_str)
end

-- Check palette
emu.log("")
emu.log("=== CGRAM (Palette) ===")
for i = 0, 3 do
    local lo = emu.read(i * 2, emu.memType.snesCgRam)
    local hi = emu.read(i * 2 + 1, emu.memType.snesCgRam)
    local color = lo | (hi << 8)
    local r = color & 0x1F
    local g = (color >> 5) & 0x1F
    local b = (color >> 10) & 0x1F
    emu.log(string.format("  Color %d: $%04X (R=%d G=%d B=%d)", i, color, r, g, b))
end

emu.log("")
emu.log("=== CPU State ===")
emu.log("PC: $" .. string.format("%04X", state.cpu.pc))
emu.log("SP: $" .. string.format("%04X", state.cpu.sp))

emu.log("")
emu.log("=== Debug Complete ===")
emu.stop(0)
