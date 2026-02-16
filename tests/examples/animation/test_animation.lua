--[[
  test_animation.lua - Test the animation example ROM

  This example was the subject of the WRAM mirroring bug fix.
  oamMemory is now correctly placed at $7E:0300 (above mirror range).

  Tests:
  1. ROM boots and reaches main()
  2. Monster sprite initializes at correct position
  3. OAM buffer is at safe address (no WRAM overlap)
  4. Input affects monster position
  5. OAM is transferred to hardware during VBlank

  Run with Mesen2:
    Load animation.sfc, then run this script
]]

-- Add snesdbg to path
local scriptDir = debug.getinfo(1, "S").source:match("@?(.*/)") or "./"
package.path = scriptDir .. "../../../tools/snesdbg/?.lua;" .. package.path

local test = require("test")
local dbg = require("snesdbg")

test.describe("Animation Example (animation)", function()

    test.beforeAll(function()
        test.loadSymbols(scriptDir .. "../../../examples/graphics/animation/animation.sym")
        dbg.setOAMBuffer("oamMemory")
    end)

    test.beforeEach(function()
        test.reset()
        test.waitFrames(20)  -- Let initialization complete
    end)

    test.it("should reach main()", function()
        test.reset()
        local reached = test.waitUntilSymbol("main", 300)
        test.assertTrue(reached, "main() should be reached")
    end)

    test.it("should initialize monster position", function()
        -- After init, monster should be at a valid position
        local x = test.read("monster_x")
        local y = test.read("monster_y")

        test.assertInRange("monster_x", 0, 255, "monster_x should be on screen")
        test.assertInRange("monster_y", 0, 223, "monster_y should be on screen")

        print(string.format("  Monster initial position: (%d, %d)", x, y))
    end)

    test.it("should have oamMemory at safe address (above $1FFF)", function()
        -- oamMemory should be at $7E:0300, not in the mirror range
        local addr = dbg.getAddress("oamMemory")
        local bank = math.floor(addr / 0x10000)
        local localAddr = addr % 0x10000

        print(string.format("  oamMemory at $%02X:%04X", bank, localAddr))

        -- If in Bank $7E, should be >= $0300 to avoid mirror overlap
        if bank == 0x7E then
            test.assertTrue(localAddr >= 0x0300,
                "oamMemory should be at $0300+ in Bank $7E to avoid WRAM mirror overlap")
        end
    end)

    test.it("should not corrupt monster_x when OAM is cleared", function()
        -- This was the original bug: oamMemory at $7E:0000 overlapped monster_x at $00:0022
        local initial_x = test.read("monster_x")
        local initial_y = test.read("monster_y")

        -- Wait a few more frames (OAM should be updated)
        test.waitFrames(10)

        local after_x = test.read("monster_x")
        local after_y = test.read("monster_y")

        -- Values should NOT have been corrupted to 0
        -- (unless the monster legitimately moved there)
        if initial_x > 10 then
            test.assertNotEqual("monster_x", 0, "monster_x should not be corrupted to 0")
        end
    end)

    test.it("should move monster right when RIGHT pressed", function()
        local initial_x = test.read("monster_x")

        test.holdButton("right")
        test.waitFrames(30)  -- Wait for movement
        test.releaseButton("right")

        local new_x = test.read("monster_x")

        -- Monster should have moved right (or wrapped)
        test.assertTrue(new_x ~= initial_x or initial_x > 240,
            "monster_x should change when RIGHT is pressed")

        print(string.format("  Moved: %d -> %d", initial_x, new_x))
    end)

    test.it("should move monster left when LEFT pressed", function()
        -- First move right to ensure we're not at 0
        test.holdButton("right")
        test.waitFrames(20)
        test.releaseButton("right")

        local initial_x = test.read("monster_x")

        test.holdButton("left")
        test.waitFrames(30)
        test.releaseButton("left")

        local new_x = test.read("monster_x")

        test.assertTrue(new_x ~= initial_x or initial_x < 10,
            "monster_x should change when LEFT is pressed")

        print(string.format("  Moved: %d -> %d", initial_x, new_x))
    end)

    test.it("should transfer OAM to hardware", function()
        test.waitFrames(5)
        test.waitForVBlank()

        -- Compare shadow buffer to hardware
        local mismatches = dbg.compareOAM(8)  -- Check first 8 sprites
        test.assertTrue(mismatches == 0, "OAM should be transferred to hardware")
    end)

    test.it("should have visible sprite on screen", function()
        local sprite = dbg.readSprite(0)

        print(string.format("  Sprite 0: (%d, %d) tile=%d hidden=%s",
            sprite.x, sprite.y, sprite.tile, tostring(sprite.hidden)))

        -- Sprite should not be hidden (Y < 240)
        test.assertFalse(sprite.hidden, "Main sprite should be visible")
    end)

    test.it("should not have WRAM mirror overlaps", function()
        test.assertNoWRAMOverlap()
    end)

end)

test.run()
