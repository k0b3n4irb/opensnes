-- Debug script for calculator
-- Run with Mesen to inspect VRAM

-- Wait a few frames for init to complete
emu.frameCount = 0
local frameWait = 10

function onFrame()
    emu.frameCount = (emu.frameCount or 0) + 1
    if emu.frameCount < frameWait then return end
    if emu.frameCount > frameWait then return end

    print("=== Calculator Debug ===")

    -- Read tilemap at $0400 (word addresses, so byte address $0800)
    -- Row 10 starts at $0400 + 10*32 = $0540
    -- In VRAM word addresses

    print("\n-- Tilemap Row 10 (should have 7 8 9 /) --")
    for col = 0, 31 do
        local addr = 0x0540 + col  -- VRAM word address
        local tile = emu.read(addr, emu.memType.videoRam, false)
        if tile ~= 0 then
            print(string.format("  Col %d: VRAM $%04X = tile %d", col, addr, tile))
        end
    end

    print("\n-- Tilemap Row 12 (should have 4 5 6 *) --")
    for col = 0, 31 do
        local addr = 0x0580 + col
        local tile = emu.read(addr, emu.memType.videoRam, false)
        if tile ~= 0 then
            print(string.format("  Col %d: VRAM $%04X = tile %d", col, addr, tile))
        end
    end

    print("\n-- Check specific expected positions --")
    -- Position for '7' should be at col 10, row 10 = $0540 + 10 = $054A
    local tile_7 = emu.read(0x054A, emu.memType.videoRam, false)
    print(string.format("  $054A (7): tile %d (expected 8)", tile_7))

    local tile_8 = emu.read(0x054E, emu.memType.videoRam, false)
    print(string.format("  $054E (8): tile %d (expected 9)", tile_8))

    local tile_9 = emu.read(0x0552, emu.memType.videoRam, false)
    print(string.format("  $0552 (9): tile %d (expected 10)", tile_9))

    -- Check WRAM variables
    print("\n-- WRAM Variables --")
    local calc_pos = emu.read(0x0000, emu.memType.workRam, false)
    print(string.format("  calc_pos: %d", calc_pos))

    -- Check if tiles are in VRAM (character data)
    print("\n-- Tile data check (first few bytes of tiles) --")
    for t = 0, 18 do
        local byte0 = emu.read(t * 8, emu.memType.videoRam, false)
        local byte1 = emu.read(t * 8 + 1, emu.memType.videoRam, false)
        print(string.format("  Tile %d: $%02X $%02X", t, byte0, byte1))
    end

    print("\n=== End Debug ===")
end

emu.addEventCallback(onFrame, emu.eventType.endFrame)
print("Debug script loaded - waiting for frames...")
