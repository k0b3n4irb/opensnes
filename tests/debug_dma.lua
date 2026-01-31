--[[
  DMA Debug Script for Mesen2

  Monitors all DMA transfers to help debug VRAM, OAM, and CGRAM issues.

  Usage:
    Mesen --testrunner --lua tests/debug_dma.lua /path/to/rom.sfc

  Features:
  - Logs all DMA channel activations
  - Shows source address, destination register, transfer size
  - For VRAM transfers, shows destination address and data preview
  - Stops after configurable number of DMAs or frames
]]

local config = {
    maxDmaCount = 50,      -- Stop after this many DMAs (0 = unlimited)
    maxFrames = 300,       -- Stop after this many frames
    showDataPreview = true, -- Show first bytes of transfer data
    previewBytes = 16,     -- Number of bytes to preview
    verbose = true         -- Show frame markers
}

local frameCount = 0
local dmaCount = 0

-- DMA destination register names
local dmaDestNames = {
    [0x2104] = "OAM",
    [0x2118] = "VRAM",
    [0x2119] = "VRAM-H",
    [0x2122] = "CGRAM",
    [0x2180] = "WRAM"
}

-- Watch for DMA triggers (MDMAEN register $420B)
emu.addMemoryCallback(function(addr, value)
    if value == 0 then return end

    dmaCount = dmaCount + 1
    emu.log(string.format("=== DMA #%d Frame %d (MDMAEN=$%02X) ===",
        dmaCount, frameCount, value))

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
            local destName = dmaDestNames[destReg] or "???"

            emu.log(string.format("  CH%d: %s ($%04X) <- $%06X (%d bytes) DMAP=$%02X",
                ch, destName, destReg, srcAddr, size, dmap))

            -- For VRAM transfers, show destination address
            if destReg == 0x2118 or destReg == 0x2119 then
                local vramAddr = emu.read(0x2116, emu.memType.cpuDebug) +
                               (emu.read(0x2117, emu.memType.cpuDebug) * 256)
                emu.log(string.format("       VRAM addr=$%04X", vramAddr))
            end

            -- Show data preview
            if config.showDataPreview and size > 0 then
                local preview = ""
                local bytesToShow = math.min(config.previewBytes, size)
                for i = 0, bytesToShow - 1 do
                    local byte = emu.read(srcAddr + i, emu.memType.cpuDebug)
                    preview = preview .. string.format("%02X ", byte)
                end
                if size > bytesToShow then
                    preview = preview .. "..."
                end
                emu.log("       Data: " .. preview)
            end
        end
    end

    if config.maxDmaCount > 0 and dmaCount >= config.maxDmaCount then
        emu.log("[Debug] Stopping after " .. dmaCount .. " DMAs")
        emu.stop(0)
    end
end, emu.memCallbackType.cpuWrite, 0x420B, 0x420B)

-- Frame counter
emu.addEventCallback(function()
    frameCount = frameCount + 1

    if config.verbose and frameCount <= 10 then
        emu.log("--- Frame " .. frameCount .. " ---")
    end

    if frameCount >= config.maxFrames then
        emu.log("[Debug] " .. frameCount .. " frames, " .. dmaCount .. " DMAs total")
        emu.stop(0)
    end
end, emu.eventType.endFrame)

emu.log("[DMA Debug] Monitoring DMA transfers...")
emu.log("  Max DMAs: " .. (config.maxDmaCount > 0 and config.maxDmaCount or "unlimited"))
emu.log("  Max frames: " .. config.maxFrames)
