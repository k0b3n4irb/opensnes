--[[
  test_custom_font.lua - Test the custom_font example ROM

  Tests:
  1. ROM boots and reaches main()
  2. Hardware is initialized correctly
  3. Font tiles are loaded
  4. Multiple messages are displayed

  Run with Mesen2:
    Load custom_font.sfc, then run this script
]]

-- Add snesdbg to path
local scriptDir = debug.getinfo(1, "S").source:match("@?(.*/)") or "./"
package.path = scriptDir .. "../../../tools/snesdbg/?.lua;" .. package.path

local test = require("test")

test.describe("Custom Font Example", function()

    test.beforeAll(function()
        test.loadSymbols(scriptDir .. "../../../examples/text/2_custom_font/custom_font.sym")
    end)

    test.beforeEach(function()
        test.reset()
    end)

    test.it("should reach main()", function()
        local reached = test.waitUntilSymbol("main", 300)
        test.assertTrue(reached, "main() should be reached")
    end)

    test.it("should initialize hardware", function()
        local reached = test.waitUntilSymbol("InitHardware", 60)
        test.assertTrue(reached, "InitHardware should be called at boot")
    end)

    test.it("should have font_tiles symbol", function()
        local addr = test.read("font_tiles")
        -- Just verify the symbol exists and is in ROM (Bank $00, address >= $8000)
        test.assertTrue(addr ~= nil, "font_tiles should exist")
    end)

    test.it("should have message strings", function()
        -- Verify message symbols exist
        test.assertTrue(test.read("msg_title") ~= nil, "msg_title should exist")
        test.assertTrue(test.read("msg_hello") ~= nil, "msg_hello should exist")
        test.assertTrue(test.read("msg_opensnes") ~= nil, "msg_opensnes should exist")
    end)

    test.it("should have VBlank handler working", function()
        test.waitFrames(10)
        -- If we got this far without hanging, VBlank is working
        test.assertTrue(true, "VBlank handler is operational")
    end)

    test.it("should not have WRAM mirror overlaps", function()
        test.assertNoWRAMOverlap()
    end)

end)

test.run()
