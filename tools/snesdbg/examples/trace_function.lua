--[[
  trace_function.lua - Example: Trace Function Execution

  This script demonstrates how to set breakpoints at function symbols
  and trace execution with symbol annotations.

  Usage in Mesen2:
    1. Load your ROM
    2. Run this script from Debug > Script Window
]]

-- Add snesdbg to path
package.path = package.path .. ";../?.lua"
local dbg = require("snesdbg")

-- Load symbols from your ROM
-- CHANGE THIS to your actual .sym file path
dbg.loadSymbols("game.sym")

-- Set breakpoint at game initialization
dbg.breakAtSymbol("main", function()
    print("")
    print("=== main() called ===")
    print("Initial state:")
    -- Print some variables if they exist
    if dbg.symbolExists("frame_count") then
        print(string.format("  frame_count: %d", dbg.read("frame_count")))
    end
end)

-- Set breakpoint at game loop (if it exists)
if dbg.symbolExists("game_loop") then
    local callCount = 0
    dbg.breakAtSymbol("game_loop", function()
        callCount = callCount + 1
        -- Only print first few calls to avoid spam
        if callCount <= 5 then
            print(string.format("game_loop called (call #%d)", callCount))
        elseif callCount == 6 then
            print("(suppressing further game_loop messages)")
        end
    end)
end

-- Trace enemy updates
if dbg.symbolExists("update_enemies") then
    dbg.breakAtSymbol("update_enemies", function()
        print("update_enemies() called")
        if dbg.symbolExists("enemy_count") then
            print(string.format("  enemy_count: %d", dbg.read("enemy_count")))
        end
    end)
end

-- Trace NMI handler
if dbg.symbolExists("_nmi") then
    local nmiCount = 0
    dbg.breakAtSymbol("_nmi", function()
        nmiCount = nmiCount + 1
        if nmiCount <= 3 then
            print(string.format("NMI #%d fired", nmiCount))
        end
    end)
end

print("Breakpoints set. Run the game to see traces...")
