--[[
  Screenshot Capture for Visual Comparison - Mesen2 Lua API

  Captures a screenshot after ROM stabilization and saves to specified path.
  Used by compare_screenshots.sh to capture both PVSnesLib and OpenSNES ROMs.

  Usage (testrunner mode):
    Mesen --testrunner script.lua rom.sfc

  Environment:
    SCREENSHOT_OUTPUT - Output path for screenshot (default: /tmp/screenshot.png)
    FRAMES_TO_WAIT    - Frames before capture (default: 120 = 2 seconds)
]]

-- Configuration from environment or defaults
local SCREENSHOT_PATH = os.getenv("SCREENSHOT_OUTPUT") or "/tmp/screenshot.png"
local FRAMES_TO_WAIT = tonumber(os.getenv("FRAMES_TO_WAIT")) or 120
local frameCount = 0
local screenshotTaken = false

emu.log("[Screenshot] Script loaded")
emu.log("[Screenshot] Output: " .. SCREENSHOT_PATH)
emu.log("[Screenshot] Waiting " .. FRAMES_TO_WAIT .. " frames...")

function onEndFrame()
    frameCount = frameCount + 1

    if frameCount >= FRAMES_TO_WAIT and not screenshotTaken then
        screenshotTaken = true
        emu.log("[Screenshot] Capturing after " .. frameCount .. " frames...")

        -- Take screenshot (returns PNG as binary string)
        local png_data = emu.takeScreenshot()

        if not png_data or #png_data == 0 then
            emu.log("[Screenshot] ERROR: No screenshot data")
            emu.stop(1)
            return
        end

        emu.log("[Screenshot] Got " .. #png_data .. " bytes")

        -- Write to file
        local file = io.open(SCREENSHOT_PATH, "wb")
        if file then
            file:write(png_data)
            file:close()
            emu.log("[Screenshot] Saved: " .. SCREENSHOT_PATH)
            emu.log("Status: PASS")
            emu.stop(0)
        else
            emu.log("[Screenshot] ERROR: Could not write to " .. SCREENSHOT_PATH)
            emu.stop(1)
        end
    end
end

emu.addEventCallback(onEndFrame, emu.eventType.endFrame)
