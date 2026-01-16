--[[
  test_sprite.lua - Example: Test Sprite/OAM Functionality

  This script demonstrates how to test sprite initialization
  and OAM DMA transfer.

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

-- Set the OAM shadow buffer symbol
-- CHANGE THIS to match your OAM buffer variable name
dbg.setOAMBuffer("oamMemory")

-- Wait for initialization
print("Waiting for initialization (15 frames)...")
dbg.waitFrames(15)

-- Print all visible sprites from shadow buffer
print("")
print("=== Visible Sprites (Shadow Buffer) ===")
dbg.printVisibleSprites()

-- Compare individual sprites
print("")
print("=== Sprite 0 Details ===")
dbg.printSprite(0, "both")

-- Compare shadow buffer to hardware OAM
print("")
print("=== OAM Comparison ===")
local mismatches = dbg.compareOAM(16)  -- Check first 16 sprites

if mismatches == 0 then
    print("SUCCESS: Shadow buffer matches hardware OAM!")
else
    print(string.format("WARNING: %d sprite(s) don't match!", mismatches))
end

-- Run some assertions
print("")
print("=== Assertions ===")
dbg.resetTests()

-- Check that sprite 0 exists and is positioned correctly
local sprite = dbg.readSprite(0)
if not sprite.hidden then
    dbg.assertOAM(0, {
        x = sprite.x,  -- Just verify it reads correctly
        y = sprite.y
    })
else
    print("Sprite 0 is hidden (Y >= 240)")
end

-- Check OAM transfer
dbg.assertOAMTransferred("OAM should be DMA'd after VBlank")

dbg.printTestSummary()
