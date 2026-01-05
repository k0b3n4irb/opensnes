--[[
  OpenSNES Screenshot Test - Mesen2 Lua API

  Takes a screenshot after ROM initialization and saves to disk.

  Usage:
    Mesen rom.sfc --testrunner --lua screenshot_test.lua

  Screenshot saved to: /tmp/opensnes_test.png
]]

local SCREENSHOT_PATH = "/tmp/opensnes_test.png"
local FRAMES_TO_WAIT = 60  -- 1 second at 60fps
local frameCount = 0

function log(msg)
    emu.log(msg)
    print(msg)
end

function onEndFrame()
    frameCount = frameCount + 1

    if frameCount == FRAMES_TO_WAIT then
        log("[Screenshot] Capturing after " .. FRAMES_TO_WAIT .. " frames...")

        -- Take screenshot (returns PNG as binary string)
        local png_data = emu.takeScreenshot()
        log("[Screenshot] Got " .. #png_data .. " bytes")

        -- Write to file
        local file = io.open(SCREENSHOT_PATH, "wb")
        if file then
            file:write(png_data)
            file:close()
            log("[Screenshot] Saved: " .. SCREENSHOT_PATH)
            emu.stop(0)
        else
            log("[Screenshot] ERROR: Could not write file")
            emu.stop(1)
        end
    end
end

emu.addEventCallback(onEndFrame, emu.eventType.endFrame)
log("[Screenshot] Script loaded, waiting " .. FRAMES_TO_WAIT .. " frames...")
