--[[
  debug_breakout.lua - Example: Debug the Breakout Game

  This script demonstrates snesdbg with a real OpenSNES example.
  It watches ball position and score variables in the breakout game.

  Usage in Mesen2:
    1. Load examples/games/breakout/breakout.sfc
    2. Run this script from Debug > Script Window

  Adjust SYM_FILE path if running from a different directory.
]]

-- Add snesdbg to path
package.path = package.path .. ";../?.lua"
local dbg = require("snesdbg")

-- Configuration - adjust path as needed
local SYM_FILE = "breakout.sym"

print("=== Breakout Debug Script ===")
print("")

-- Load symbols
print("Loading symbols...")
local ok, err = pcall(function()
    dbg.loadSymbols(SYM_FILE)
end)

if not ok then
    print("ERROR: Could not load symbols: " .. tostring(err))
    print("Make sure breakout.sym is accessible.")
    print("Try: copy examples/games/breakout/breakout.sym here")
    return
end

-- Check key symbols
local symbols = {"ball_x", "ball_y", "ball_dx", "ball_dy", "paddle_x", "score"}
print("")
print("Checking game symbols...")
for _, name in ipairs(symbols) do
    if dbg.symbolExists(name) then
        local addr = dbg.getAddress(name)
        print(string.format("  %s: $%06X = %d", name, addr, dbg.read(name)))
    else
        print(string.format("  %s: NOT FOUND (name may differ)", name))
    end
end

-- Set up OAM buffer
if dbg.symbolExists("oamMemory") then
    dbg.setOAMBuffer("oamMemory")
end

-- Watch ball position for rapid changes (collision detection debug)
if dbg.symbolExists("ball_x") then
    dbg.watch("ball_x", function(old, new)
        -- Detect teleportation (large jump = possible bug)
        local delta = math.abs(new - old)
        if delta > 16 then
            print(string.format("!! ball_x jumped: %d -> %d (delta=%d)", old, new, delta))
        end
    end)
end

if dbg.symbolExists("ball_y") then
    dbg.watch("ball_y", function(old, new)
        local delta = math.abs(new - old)
        if delta > 16 then
            print(string.format("!! ball_y jumped: %d -> %d (delta=%d)", old, new, delta))
        end
    end)
end

-- Watch score
if dbg.symbolExists("score") then
    dbg.watch("score", function(old, new)
        print(string.format("Score: %d -> %d", old, new))
    end)
end

-- Print state after initialization
dbg.afterFrames(30, function()
    print("")
    print("=== State after 30 frames ===")
    for _, name in ipairs(symbols) do
        if dbg.symbolExists(name) then
            print(string.format("  %s = %d", name, dbg.read(name)))
        end
    end

    -- Show visible sprites
    print("")
    dbg.printVisibleSprites()
end)

print("")
print("Watches active. Play the game to see debug output.")
