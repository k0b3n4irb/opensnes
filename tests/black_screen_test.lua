--[[
  OpenSNES Black Screen Test - Mesen2 Lua API

  Detects if a ROM produces a black (or nearly black) screen,
  which usually indicates a broken/non-working ROM.

  Detection method:
  PNG screenshot size is used as a proxy for screen content.
  Black screens compress extremely well (<300 bytes), while
  screens with actual content have more entropy (>300 bytes).

  Usage:
    Mesen --testrunner --enablestdout rom.sfc black_screen_test.lua

  Exit codes:
    0 = PASS (screen has content)
    1 = FAIL (black screen detected)

  Output format (to stdout with --enablestdout):
    BLACKTEST_RESULT:PASS:details
    BLACKTEST_RESULT:FAIL:details
]]

local FRAMES_TO_WAIT = 90  -- 1.5 seconds at 60fps (allow time for init)

-- Minimum PNG size for "has content"
-- Empirically measured:
-- - Pure black screen: ~100-150 bytes (PNG compresses extremely well)
-- - Small sprite (8x8): ~250-280 bytes
-- - Screen with text/graphics: 500+ bytes
-- Using 200 as threshold to allow minimal content while catching black screens
local MIN_PNG_SIZE = 200

local frameCount = 0

function log(msg)
    -- emu.log() writes to Mesen's log window (always works)
    emu.log(msg)
    -- print() writes to stdout (requires --enablestdout)
    print(msg)
end

function onEndFrame()
    frameCount = frameCount + 1

    if frameCount == FRAMES_TO_WAIT then
        log("[BlackTest] Taking screenshot after " .. FRAMES_TO_WAIT .. " frames...")

        -- Take screenshot (returns PNG binary string)
        local screenshot = emu.takeScreenshot()

        if not screenshot then
            log("BLACKTEST_RESULT:FAIL:takeScreenshot returned nil")
            emu.stop(1)
            return
        end

        local size = #screenshot
        log("[BlackTest] Screenshot PNG size: " .. size .. " bytes")

        if size >= MIN_PNG_SIZE then
            log("BLACKTEST_RESULT:PASS:PNG size " .. size .. " bytes (has content)")
            emu.stop(0)
        else
            log("BLACKTEST_RESULT:FAIL:PNG size " .. size .. " bytes (black screen)")
            emu.stop(1)
        end
    end
end

-- Register callback
emu.addEventCallback(onEndFrame, emu.eventType.endFrame)
log("[BlackTest] Script loaded, waiting " .. FRAMES_TO_WAIT .. " frames...")
