--[[
  watch_variable.lua - Example: Watch Variables for Changes

  This script demonstrates how to watch variables by name and
  get notified when they change.

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

-- Watch some variables
dbg.watch("player_x", function(old, new)
    print(string.format("player_x changed: %d -> %d", old, new))
end)

dbg.watch("player_y", function(old, new)
    print(string.format("player_y changed: %d -> %d", old, new))
end)

dbg.watch("player_health", function(old, new)
    print(string.format("player_health changed: %d -> %d", old, new))
    if new < old then
        print("  DAMAGE TAKEN!")
    elseif new > old then
        print("  HEALED!")
    end
    if new == 0 then
        print("  GAME OVER!")
    end
end)

-- Also print initial values
print("")
print("=== Initial Values ===")
print(string.format("player_x: %d", dbg.read("player_x")))
print(string.format("player_y: %d", dbg.read("player_y")))
print(string.format("player_health: %d", dbg.read("player_health")))
print("")
print("Watching for changes... (run the game)")
