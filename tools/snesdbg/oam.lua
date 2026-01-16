--[[
  oam.lua - OAM (Object Attribute Memory) Helpers for snesdbg

  Provides functions to read, compare, and verify OAM state.

  OAM Structure (544 bytes total):
    - Main table: 512 bytes (128 sprites × 4 bytes each)
    - High table: 32 bytes (2 bits per sprite)

  Main table entry (4 bytes per sprite):
    Byte 0: X position (low 8 bits)
    Byte 1: Y position
    Byte 2: Tile number
    Byte 3: Attributes (vhoopppc)
      v = vertical flip
      h = horizontal flip
      oo = priority (0-3)
      ppp = palette (0-7)
      c = tile name bit 8

  High table (2 bits per sprite):
    Bit 0: X position bit 8 (sign extend)
    Bit 1: Size select (0=small, 1=large)

  Usage:
    local oam = require("oam")
    oam.setBufferSymbol("oamMemory")  -- Set shadow buffer symbol
    local sprite = oam.readSprite(0)
    oam.compareToHardware()
]]

local M = {}

-- Shadow buffer symbol
local bufferSymbol = nil
local bufferAddr = nil
local symbols = nil
local mem = nil

--- Initialize with memory module and symbol table
function M.init(memModule, symbolTable)
    mem = memModule
    symbols = symbolTable
end

--- Set the OAM shadow buffer symbol name
-- @param symbolName Name of the shadow buffer (e.g., "oamMemory")
function M.setBufferSymbol(symbolName)
    bufferSymbol = symbolName
    if symbols then
        bufferAddr = symbols:resolve(symbolName)
    end
end

--- Set the OAM shadow buffer address directly
-- @param addr 24-bit address of the shadow buffer
function M.setBufferAddress(addr)
    bufferAddr = addr
    bufferSymbol = nil
end

--- Read a sprite from the shadow buffer
-- @param index Sprite index (0-127)
-- @return table with sprite data
function M.readSprite(index)
    if not bufferAddr then
        error("OAM buffer not set. Call setBufferSymbol() or setBufferAddress() first.")
    end

    if index < 0 or index > 127 then
        error("Sprite index out of range: " .. index)
    end

    -- Main table offset
    local mainOffset = index * 4

    -- Read main table bytes
    local x_low = mem.readByte(bufferAddr + mainOffset)
    local y = mem.readByte(bufferAddr + mainOffset + 1)
    local tile = mem.readByte(bufferAddr + mainOffset + 2)
    local attr = mem.readByte(bufferAddr + mainOffset + 3)

    -- Read high table
    local highOffset = 512 + math.floor(index / 4)
    local highByte = mem.readByte(bufferAddr + highOffset)
    local highBit = (index % 4) * 2
    local highBits = bit.band(bit.rshift(highByte, highBit), 0x03)

    local x_bit8 = bit.band(highBits, 0x01)
    local size = bit.rshift(bit.band(highBits, 0x02), 1)

    -- Reconstruct full X position (signed 9-bit)
    local x = x_low
    if x_bit8 == 1 then
        x = x - 256  -- Sign extend for off-screen left
    end

    -- Parse attributes
    local vflip = bit.band(attr, 0x80) ~= 0
    local hflip = bit.band(attr, 0x40) ~= 0
    local priority = bit.rshift(bit.band(attr, 0x30), 4)
    local palette = bit.rshift(bit.band(attr, 0x0E), 1)
    local tileHi = bit.band(attr, 0x01)

    -- Full tile number (9-bit)
    local fullTile = tile + (tileHi * 256)

    return {
        index = index,
        x = x,
        y = y,
        tile = fullTile,
        attr = attr,
        vflip = vflip,
        hflip = hflip,
        priority = priority,
        palette = palette,
        size = size,
        hidden = (y >= 240)  -- Y >= 240 hides sprite on NTSC
    }
end

--- Read a sprite from hardware OAM
-- @param index Sprite index (0-127)
-- @return table with sprite data
function M.readHardwareSprite(index)
    if index < 0 or index > 127 then
        error("Sprite index out of range: " .. index)
    end

    -- Main table offset
    local mainOffset = index * 4

    -- Read from snesSpriteRam (NOT snesOam!)
    local x_low = emu.read(mainOffset, emu.memType.snesSpriteRam)
    local y = emu.read(mainOffset + 1, emu.memType.snesSpriteRam)
    local tile = emu.read(mainOffset + 2, emu.memType.snesSpriteRam)
    local attr = emu.read(mainOffset + 3, emu.memType.snesSpriteRam)

    -- Read high table
    local highOffset = 512 + math.floor(index / 4)
    local highByte = emu.read(highOffset, emu.memType.snesSpriteRam)
    local highBit = (index % 4) * 2
    local highBits = bit.band(bit.rshift(highByte, highBit), 0x03)

    local x_bit8 = bit.band(highBits, 0x01)
    local size = bit.rshift(bit.band(highBits, 0x02), 1)

    -- Reconstruct full X position
    local x = x_low
    if x_bit8 == 1 then
        x = x - 256
    end

    -- Parse attributes
    local vflip = bit.band(attr, 0x80) ~= 0
    local hflip = bit.band(attr, 0x40) ~= 0
    local priority = bit.rshift(bit.band(attr, 0x30), 4)
    local palette = bit.rshift(bit.band(attr, 0x0E), 1)
    local tileHi = bit.band(attr, 0x01)

    local fullTile = tile + (tileHi * 256)

    return {
        index = index,
        x = x,
        y = y,
        tile = fullTile,
        attr = attr,
        vflip = vflip,
        hflip = hflip,
        priority = priority,
        palette = palette,
        size = size,
        hidden = (y >= 240)
    }
end

--- Compare a sprite's shadow buffer state to hardware
-- @param index Sprite index (0-127)
-- @return true if match, false if mismatch
-- @return mismatch description if different
function M.compareSprite(index)
    local buffer = M.readSprite(index)
    local hw = M.readHardwareSprite(index)

    local mismatches = {}

    if buffer.x ~= hw.x then
        table.insert(mismatches, string.format("X: buffer=%d hw=%d", buffer.x, hw.x))
    end
    if buffer.y ~= hw.y then
        table.insert(mismatches, string.format("Y: buffer=%d hw=%d", buffer.y, hw.y))
    end
    if buffer.tile ~= hw.tile then
        table.insert(mismatches, string.format("tile: buffer=%d hw=%d", buffer.tile, hw.tile))
    end
    if buffer.attr ~= hw.attr then
        table.insert(mismatches, string.format("attr: buffer=$%02X hw=$%02X", buffer.attr, hw.attr))
    end

    if #mismatches == 0 then
        return true, nil
    else
        return false, table.concat(mismatches, ", ")
    end
end

--- Compare all sprites and print differences
-- @param maxSprites Maximum sprites to check (default 128)
-- @return number of mismatches
function M.compareToHardware(maxSprites)
    maxSprites = maxSprites or 128
    local mismatches = 0

    print("=== OAM Comparison ===")

    for i = 0, maxSprites - 1 do
        local match, desc = M.compareSprite(i)
        if not match then
            mismatches = mismatches + 1
            print(string.format("Sprite %d: MISMATCH - %s", i, desc))
        end
    end

    if mismatches == 0 then
        print("All sprites match!")
    else
        print(string.format("%d sprite(s) mismatched", mismatches))
    end

    return mismatches
end

--- Print sprite information
-- @param index Sprite index (0-127)
-- @param source "buffer" (default), "hardware", or "both"
function M.printSprite(index, source)
    source = source or "buffer"

    local function printSpriteData(sprite, label)
        local statusStr = sprite.hidden and " (hidden)" or ""
        print(string.format("%s Sprite %d:%s", label, sprite.index, statusStr))
        print(string.format("  Position: (%d, %d)", sprite.x, sprite.y))
        print(string.format("  Tile: %d ($%03X)", sprite.tile, sprite.tile))
        print(string.format("  Attributes: $%02X", sprite.attr))
        print(string.format("    Flip: H=%s V=%s",
            sprite.hflip and "yes" or "no",
            sprite.vflip and "yes" or "no"))
        print(string.format("    Priority: %d, Palette: %d, Size: %s",
            sprite.priority, sprite.palette,
            sprite.size == 1 and "large" or "small"))
    end

    if source == "buffer" or source == "both" then
        local sprite = M.readSprite(index)
        printSpriteData(sprite, "[Buffer]")
    end

    if source == "hardware" or source == "both" then
        local sprite = M.readHardwareSprite(index)
        printSpriteData(sprite, "[Hardware]")
    end
end

--- Assert OAM sprite matches expected values
-- @param index Sprite index
-- @param expected Table with expected values {x=, y=, tile=, ...}
-- @param msg Optional message
-- @return true if all assertions pass
function M.assertSprite(index, expected, msg)
    local sprite = M.readSprite(index)
    local prefix = msg and (msg .. ": ") or ""
    local allPass = true

    if expected.x ~= nil and sprite.x ~= expected.x then
        print(string.format("[FAIL] %sSprite %d X: expected %d, got %d",
            prefix, index, expected.x, sprite.x))
        allPass = false
    end

    if expected.y ~= nil and sprite.y ~= expected.y then
        print(string.format("[FAIL] %sSprite %d Y: expected %d, got %d",
            prefix, index, expected.y, sprite.y))
        allPass = false
    end

    if expected.tile ~= nil and sprite.tile ~= expected.tile then
        print(string.format("[FAIL] %sSprite %d tile: expected %d, got %d",
            prefix, index, expected.tile, sprite.tile))
        allPass = false
    end

    if expected.palette ~= nil and sprite.palette ~= expected.palette then
        print(string.format("[FAIL] %sSprite %d palette: expected %d, got %d",
            prefix, index, expected.palette, sprite.palette))
        allPass = false
    end

    if expected.priority ~= nil and sprite.priority ~= expected.priority then
        print(string.format("[FAIL] %sSprite %d priority: expected %d, got %d",
            prefix, index, expected.priority, sprite.priority))
        allPass = false
    end

    if expected.hidden ~= nil and sprite.hidden ~= expected.hidden then
        print(string.format("[FAIL] %sSprite %d hidden: expected %s, got %s",
            prefix, index, tostring(expected.hidden), tostring(sprite.hidden)))
        allPass = false
    end

    if allPass then
        print(string.format("[PASS] %sSprite %d matches expected values", prefix, index))
    end

    return allPass
end

--- Assert that OAM was transferred to hardware (all sprites match)
-- @param msg Optional message
-- @return true if all sprites match
function M.assertTransferred(msg)
    local mismatches = 0

    for i = 0, 127 do
        local match = M.compareSprite(i)
        if not match then
            mismatches = mismatches + 1
        end
    end

    local prefix = msg and (msg .. ": ") or ""

    if mismatches == 0 then
        print(string.format("[PASS] %sOAM transferred correctly", prefix))
        return true
    else
        print(string.format("[FAIL] %sOAM transfer failed: %d sprite(s) differ", prefix, mismatches))
        return false
    end
end

--- Find visible (non-hidden) sprites
-- @return array of sprite indices that are visible
function M.findVisibleSprites()
    local visible = {}

    for i = 0, 127 do
        local sprite = M.readSprite(i)
        if not sprite.hidden then
            table.insert(visible, i)
        end
    end

    return visible
end

--- Print all visible sprites
function M.printVisibleSprites()
    local visible = M.findVisibleSprites()

    print(string.format("=== Visible Sprites (%d) ===", #visible))

    for _, i in ipairs(visible) do
        local sprite = M.readSprite(i)
        print(string.format("  [%d] (%d, %d) tile=%d pal=%d pri=%d",
            i, sprite.x, sprite.y, sprite.tile, sprite.palette, sprite.priority))
    end
end

return M
