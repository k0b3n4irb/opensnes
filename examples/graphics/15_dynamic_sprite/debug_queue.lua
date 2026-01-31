--[[
  Debug script for Dynamic Sprite Queue
  Monitors oamQueueEntry to see what values are being set for DMA transfers
]]

local frameCount = 0
local dmaCount = 0

-- Watch for DMA triggers (MDMAEN register)
emu.addMemoryCallback(function(addr, value)
    if value ~= 0 then
        dmaCount = dmaCount + 1
        emu.log("=== DMA #" .. dmaCount .. " triggered (MDMAEN=" .. string.format("%02X", value) .. ") ===")

        -- Check each active channel
        for ch = 0, 7 do
            local chBit = 1 << ch
            if (value & chBit) ~= 0 then
                local base = 0x4300 + (ch * 0x10)
                local dmap = emu.read(base + 0, emu.memType.cpuDebug)
                local bbad = emu.read(base + 1, emu.memType.cpuDebug)
                local a1tl = emu.read(base + 2, emu.memType.cpuDebug)
                local a1th = emu.read(base + 3, emu.memType.cpuDebug)
                local a1b = emu.read(base + 4, emu.memType.cpuDebug)
                local dasl = emu.read(base + 5, emu.memType.cpuDebug)
                local dash = emu.read(base + 6, emu.memType.cpuDebug)

                local srcAddr = a1tl + (a1th * 256) + (a1b * 65536)
                local size = dasl + (dash * 256)
                local destReg = 0x2100 + bbad

                emu.log(string.format("  CH%d: DMAP=%02X BBAD=%04X Src=$%06X Size=%d",
                    ch, dmap, destReg, srcAddr, size))

                -- If this is VRAM transfer (to $2118), show where data is coming from
                if destReg == 0x2118 then
                    local vramAddr = emu.read(0x2116, emu.memType.cpuDebug) +
                                   (emu.read(0x2117, emu.memType.cpuDebug) * 256)
                    emu.log(string.format("       VRAM dest=$%04X", vramAddr))

                    -- Read first few bytes of source data
                    local preview = ""
                    for i = 0, 15 do
                        local byte = emu.read(srcAddr + i, emu.memType.cpuDebug)
                        preview = preview .. string.format("%02X ", byte)
                    end
                    emu.log("       Source data: " .. preview)
                end
            end
        end

        if dmaCount >= 20 then
            emu.log("[Debug] Stopping after 20 DMAs")
            emu.stop()
        end
    end
end, emu.memCallbackType.cpuWrite, 0x420B, 0x420B)

-- End of frame callback
emu.addEventCallback(function()
    frameCount = frameCount + 1

    if frameCount <= 5 then
        emu.log("--- Frame " .. frameCount .. " complete ---")
    end

    if frameCount >= 120 then
        emu.log("[Debug] 120 frames complete, stopping")
        emu.stop()
    end
end, emu.eventType.endFrame)

emu.log("[Debug] Queue monitoring started")
emu.log("Waiting for DMA transfers...")
