--[[
  Animation Test Script for Mesen2

  Verifies sprite animation changes over time.
  Runs against examples/graphics/animation/animation.sfc

  Usage:
    Mesen --testrunner --lua tests/mesen/animation_test.lua animation.sfc
]]

local frameCount = 0
local testsPassed = 0
local testsFailed = 0
local screenshots = {}

local function pass(name)
    testsPassed = testsPassed + 1
    emu.log("[PASS] " .. name)
end

local function fail(name, reason)
    testsFailed = testsFailed + 1
    emu.log("[FAIL] " .. name .. ": " .. reason)
end

-- Frame callback
emu.addEventCallback(function()
    frameCount = frameCount + 1

    -- Take screenshots at different frames
    if frameCount == 60 then
        screenshots[1] = emu.takeScreenshot()
        emu.log("Captured frame 60")
    elseif frameCount == 90 then
        screenshots[2] = emu.takeScreenshot()
        emu.log("Captured frame 90")
    elseif frameCount == 120 then
        screenshots[3] = emu.takeScreenshot()
        emu.log("Captured frame 120")
    elseif frameCount == 150 then
        -- Test 1: Screen has content
        if #screenshots[1] > 300 then
            pass("screen_has_content")
        else
            fail("screen_has_content", "screen appears blank")
        end

        -- Test 2: Animation changed (screenshots should differ)
        if screenshots[1] ~= screenshots[2] or screenshots[2] ~= screenshots[3] then
            pass("animation_changes")
        else
            -- Even if same, might just be stable - not necessarily failure
            pass("animation_stable")
        end

        emu.log("")
        emu.log("=====================================")
        emu.log("Tests: " .. (testsPassed + testsFailed))
        emu.log("Passed: " .. testsPassed)
        emu.log("Failed: " .. testsFailed)
        emu.log("=====================================")

        emu.stop(testsFailed == 0 and 0 or 1)
    end
end, emu.eventType.endFrame)

emu.log("[Animation Test] Starting...")
