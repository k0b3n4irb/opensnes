--[[
  Simple screenshot test - just takes a screenshot after startup
]]

local frameCount = 0
local SCREENSHOT_DIR = os.getenv("SCREENSHOT_DIR") or "."

function onEndFrame()
    frameCount = frameCount + 1

    if frameCount == 60 then
        print("[Test] Taking screenshot at frame 60...")
        local data = emu.takeScreenshot()
        if data then
            local filename = SCREENSHOT_DIR .. "/startup.png"
            local file = io.open(filename, "wb")
            if file then
                file:write(data)
                file:close()
                print("[Test] Saved: " .. filename .. " (" .. #data .. " bytes)")
            else
                print("[Test] ERROR: Could not write file")
            end
        else
            print("[Test] ERROR: takeScreenshot returned nil")
        end
        print("TEST_DONE")
        emu.stop(0)
    end
end

emu.addEventCallback(onEndFrame, emu.eventType.endFrame)
print("[Test] Script loaded, waiting 60 frames...")
