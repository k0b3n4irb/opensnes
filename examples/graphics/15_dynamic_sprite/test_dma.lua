-- Simplified DMA test
local dmaCount = 0

emu.addMemoryCallback(function(addr, value)
    if value ~= 0 then
        dmaCount = dmaCount + 1
        emu.log("DMA #" .. dmaCount .. " MDMAEN=" .. string.format("%02X", value))

        for ch = 0, 7 do
            local chBit = 1 << ch
            if (value & chBit) ~= 0 then
                local base = 0x4300 + (ch * 0x10)
                local a1tl = emu.read(base + 2, emu.memType.cpuDebug)
                local a1th = emu.read(base + 3, emu.memType.cpuDebug)
                local a1b = emu.read(base + 4, emu.memType.cpuDebug)
                local dasl = emu.read(base + 5, emu.memType.cpuDebug)
                local dash = emu.read(base + 6, emu.memType.cpuDebug)
                local srcAddr = a1tl + (a1th * 256) + (a1b * 65536)
                local size = dasl + (dash * 256)
                emu.log("  CH" .. ch .. ": Src=$" .. string.format("%06X", srcAddr) .. " Size=" .. size)
            end
        end

        if dmaCount >= 10 then
            emu.stop()
        end
    end
end, emu.memCallbackType.cpuWrite, 0x420B, 0x420B)

emu.log("[Test] Monitoring DMA...")
