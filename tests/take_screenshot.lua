--[[
  Screenshot tool for Mesen2

  Outputs screenshot as base64 to stdout (avoids file I/O crash in testrunner mode)
  Decode with: grep "^SCREENSHOT_DATA:" output.txt | cut -d: -f2 | base64 -d > screenshot.png
]]

local frameCount = 0
local FRAMES_TO_WAIT = 60

-- Base64 encoding table
local b64chars = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'

local function base64_encode(data)
    local result = {}
    local len = #data
    local i = 1

    while i <= len do
        local a = string.byte(data, i) or 0
        local b = string.byte(data, i + 1) or 0
        local c = string.byte(data, i + 2) or 0

        local n = a * 65536 + b * 256 + c

        local c1 = math.floor(n / 262144) % 64
        local c2 = math.floor(n / 4096) % 64
        local c3 = math.floor(n / 64) % 64
        local c4 = n % 64

        result[#result + 1] = string.sub(b64chars, c1 + 1, c1 + 1)
        result[#result + 1] = string.sub(b64chars, c2 + 1, c2 + 1)

        if i + 1 <= len then
            result[#result + 1] = string.sub(b64chars, c3 + 1, c3 + 1)
        else
            result[#result + 1] = '='
        end

        if i + 2 <= len then
            result[#result + 1] = string.sub(b64chars, c4 + 1, c4 + 1)
        else
            result[#result + 1] = '='
        end

        i = i + 3
    end

    return table.concat(result)
end

print("[Screenshot] Script starting, waiting " .. FRAMES_TO_WAIT .. " frames...")

local function onEndFrame()
    frameCount = frameCount + 1

    if frameCount == FRAMES_TO_WAIT then
        print("[Screenshot] Capturing at frame " .. frameCount)

        local pngData = emu.takeScreenshot()

        if pngData and #pngData > 0 then
            print("[Screenshot] PNG size: " .. #pngData .. " bytes")

            -- Output as base64 (split into chunks for readability)
            local b64 = base64_encode(pngData)
            print("SCREENSHOT_BASE64_START")
            -- Output in 76-char lines
            local pos = 1
            while pos <= #b64 do
                print(string.sub(b64, pos, pos + 75))
                pos = pos + 76
            end
            print("SCREENSHOT_BASE64_END")
            print("SCREENSHOT_RESULT:PASS:" .. #pngData)
        else
            print("SCREENSHOT_RESULT:FAIL:empty")
        end

        emu.stop(0)
    end
end

emu.addEventCallback(onEndFrame, emu.eventType.endFrame)
