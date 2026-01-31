--[[
  Example Review Script - captures detailed info about each ROM

  Outputs:
  - PNG size
  - Screenshot saved to /tmp/
  - ROM name from argument
]]

local FRAMES_TO_WAIT = 120  -- 2 seconds

local frameCount = 0
local romName = "unknown"

-- Get ROM name from environment or use default
if emu.getState then
    local state = emu.getState()
    if state and state.romInfo and state.romInfo.name then
        romName = state.romInfo.name
    end
end

function onEndFrame()
    frameCount = frameCount + 1

    if frameCount == FRAMES_TO_WAIT then
        local screenshot = emu.takeScreenshot()
        if screenshot then
            local size = #screenshot
            print(string.format("REVIEW_RESULT:PNG_SIZE=%d", size))

            -- Categorize based on size
            local category = "UNKNOWN"
            if size < 150 then
                category = "BLACK"
            elseif size < 300 then
                category = "MINIMAL"
            elseif size < 1000 then
                category = "SIMPLE"
            elseif size < 5000 then
                category = "NORMAL"
            else
                category = "COMPLEX"
            end
            print(string.format("REVIEW_RESULT:CATEGORY=%s", category))
        else
            print("REVIEW_RESULT:ERROR=no_screenshot")
        end

        emu.stop(0)
    end
end

emu.addEventCallback(onEndFrame, emu.eventType.endFrame)
print("[Review] Waiting " .. FRAMES_TO_WAIT .. " frames...")
