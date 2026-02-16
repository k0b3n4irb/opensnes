--[[
  debug_animation.lua - Example: Debug the animation Example

  This script is designed to debug the OpenSNES animation example
  that was fixed for WRAM mirroring issues.

  It verifies:
  1. oamMemory is placed correctly (not overlapping Bank $00 vars)
  2. Sprite position variables maintain their values
  3. OAM buffer is transferred correctly to hardware

  Usage in Mesen2:
    1. Load examples/graphics/animation/animation.sfc
    2. Run this script from Debug > Script Window
]]

-- Add snesdbg to path
package.path = package.path .. ";../?.lua"
local dbg = require("snesdbg")

-- Configuration - adjust paths as needed
local SYM_FILE = "animation.sym"

print("=== animation Debug Script ===")
print("")

-- Load symbols
print("Loading symbols...")
local ok, err = pcall(function()
    dbg.loadSymbols(SYM_FILE)
end)

if not ok then
    print("ERROR: Could not load symbols: " .. tostring(err))
    print("Make sure you're running from the animation directory")
    print("or adjust the SYM_FILE path in this script.")
    return
end

-- Check critical symbols exist
local criticalSymbols = {"oamMemory", "sprite_x", "sprite_y"}
print("")
print("Checking critical symbols...")
for _, name in ipairs(criticalSymbols) do
    local addr = dbg.getAddress(name)
    if addr then
        local bank = math.floor(addr / 0x10000)
        local localAddr = addr % 0x10000
        print(string.format("  %s: $%02X:%04X", name, bank, localAddr))

        -- Warning for WRAM mirror overlap
        if bank == 0x7E and localAddr < 0x2000 then
            print(string.format("    WARNING: In WRAM mirror range! May overlap Bank $00 vars!"))
        end
    else
        print(string.format("  %s: NOT FOUND", name))
    end
end

-- Set up OAM buffer
if dbg.symbolExists("oamMemory") then
    dbg.setOAMBuffer("oamMemory")
end

-- Watch for the WRAM corruption bug
-- If sprite_x gets zeroed unexpectedly, we'll catch it
if dbg.symbolExists("sprite_x") then
    local lastValidX = nil
    dbg.watch("sprite_x", function(old, new)
        if new == 0 and old ~= 0 and old < 240 then
            print("")
            print("!!! POSSIBLE CORRUPTION DETECTED !!!")
            print(string.format("sprite_x zeroed: %d -> 0", old))
            print("This may indicate WRAM mirroring overlap!")
            print("")
            -- Dump oamMemory area
            if dbg.symbolExists("oamMemory") then
                print("oamMemory dump:")
                dbg.hexdump("oamMemory", 32)
            end
        end
        lastValidX = new
    end)
end

-- Wait for initialization
print("")
print("Waiting for initialization (20 frames)...")
dbg.waitFrames(20)

-- Print current state
print("")
print("=== Current State ===")
if dbg.symbolExists("sprite_x") then
    print(string.format("sprite_x: %d", dbg.read("sprite_x")))
end
if dbg.symbolExists("sprite_y") then
    print(string.format("sprite_y: %d", dbg.read("sprite_y")))
end

-- Print visible sprites
print("")
dbg.printVisibleSprites()

-- Compare OAM
print("")
local mismatches = dbg.compareOAM(8)

-- Summary
print("")
print("=== Summary ===")
if mismatches == 0 then
    print("OAM: OK (shadow buffer matches hardware)")
else
    print(string.format("OAM: %d mismatches detected", mismatches))
end

local spriteX = dbg.symbolExists("sprite_x") and dbg.read("sprite_x") or nil
if spriteX and spriteX > 0 then
    print(string.format("sprite_x: OK (value = %d)", spriteX))
else
    print("sprite_x: WARNING (value is 0 or missing)")
end

print("")
print("Continuing to monitor... (watches active)")
print("If WRAM corruption occurs, you'll see a warning.")
