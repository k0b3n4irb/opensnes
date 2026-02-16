--[[
  HDMA Wave Test - Memory write after NMI

  The NMI handler reads auto-joypad and computes pad_keysdown.
  We write AFTER the NMI handler to override pad_keysdown.
]]

local frameCount = 0
local injectA = false
local nmiCount = 0

-- Base64 encoding
local b64 = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'
local function toBase64(data)
    local r = {}
    for i = 1, #data, 3 do
        local a, b, c = string.byte(data, i, i+2)
        b, c = b or 0, c or 0
        local n = a * 65536 + b * 256 + c
        r[#r+1] = b64:sub(math.floor(n/262144)%64+1, math.floor(n/262144)%64+1)
        r[#r+1] = b64:sub(math.floor(n/4096)%64+1, math.floor(n/4096)%64+1)
        r[#r+1] = i+1 <= #data and b64:sub(math.floor(n/64)%64+1, math.floor(n/64)%64+1) or '='
        r[#r+1] = i+2 <= #data and b64:sub(n%64+1, n%64+1) or '='
    end
    return table.concat(r)
end

local function screenshot(name)
    local png = emu.takeScreenshot()
    if png then
        print("SCREENSHOT:" .. name .. ":SIZE:" .. #png)
        print("SCREENSHOT:" .. name .. ":DATA:" .. toBase64(png))
    end
end

-- After NMI completes, inject A if flagged
local function onNmi()
    nmiCount = nmiCount + 1

    if injectA then
        -- Write KEY_A (0x0080) to pad_keysdown ($0040)
        -- This happens RIGHT AFTER NMI handler has computed pad_keysdown
        print("[Test] Injecting A via memory write after NMI #" .. nmiCount)
        emu.writeWord(0x0040, 0x0080, emu.memType.cpu)
        emu.writeWord(0x002C, 0x0080, emu.memType.cpu)
        injectA = false
    end
end

local function onEndFrame()
    frameCount = frameCount + 1

    if frameCount == 60 then
        print("[Test] Startup screenshot at frame 60 (NMI count: " .. nmiCount .. ")")
        screenshot("startup")
        -- Flag to inject A on next NMI
        injectA = true
        print("[Test] Will inject A after next NMI")

    elseif frameCount == 120 then
        print("[Test] Wave screenshot at frame 120 (NMI count: " .. nmiCount .. ")")
        screenshot("wave")
        emu.stop(0)
    end
end

emu.addEventCallback(onNmi, emu.eventType.nmi)
emu.addEventCallback(onEndFrame, emu.eventType.endFrame)
print("[Test] HDMA Wave Test - NMI callback method")
