-- Simple state check script for hello_world ROM
-- Runs for a few seconds then checks key registers

local frames_to_run = 120  -- ~2 seconds

emu.log("=== Hello World State Check ===")

-- Run emulator for some frames
for i = 1, frames_to_run do
    emu.frameAdvance()
end

-- Check key PPU registers and memory
local inidisp = emu.read(0x2100, emu.memType.snesMemory)
local bgmode = emu.read(0x2105, emu.memType.snesMemory)
local bg1sc = emu.read(0x2107, emu.memType.snesMemory)
local tm = emu.read(0x212C, emu.memType.snesMemory)

emu.log("INIDISP ($2100): " .. string.format("$%02X", inidisp))
emu.log("BGMODE ($2105): " .. string.format("$%02X", bgmode))
emu.log("BG1SC ($2107): " .. string.format("$%02X", bg1sc))
emu.log("TM ($212C): " .. string.format("$%02X", tm))

-- Check if screen is enabled (INIDISP bit 7 = 0, bits 0-3 > 0)
if (inidisp & 0x80) == 0 and (inidisp & 0x0F) > 0 then
    emu.log("Screen is ON - Brightness: " .. (inidisp & 0x0F))
else
    emu.log("Screen is OFF or BLANK!")
end

-- Check if BG1 is enabled on main screen
if (tm & 0x01) ~= 0 then
    emu.log("BG1 is ENABLED on main screen")
else
    emu.log("BG1 is DISABLED on main screen!")
end

-- Read some VRAM to check if font data was loaded
-- Font should be at VRAM address $0000 (word address)
-- First, we'd need to set up VRAM address to read, but SNES PPU
-- doesn't allow reading VRAM during active display easily

-- Check CPU state
local pc = emu.getState().cpu.pc
local sp = emu.getState().cpu.sp
emu.log("PC: $" .. string.format("%04X", pc))
emu.log("SP: $" .. string.format("%04X", sp))

emu.log("=== Check Complete ===")
emu.stop(0)
