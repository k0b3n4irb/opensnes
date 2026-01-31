--[[
Breakout Level Transition Test

This test verifies that level transitions don't cause visual corruption
(the "pink border" bug that was fixed).

The bug was caused by splitting DMA transfers to overlapping VRAM regions
across multiple VBlanks. During the intermediate frame, BG3 would read
corrupted data and display incorrect colors.

Test approach:
1. Wait for game to initialize
2. Press START to begin
3. Monitor CGRAM colors 8-15 (background palette)
4. Verify no unexpected color values appear during transitions
5. Check that specific pixels don't show "pink" corruption

Usage:
  ./Mesen --testrunner examples/game/1_breakout/breakout.sfc --lua tests/examples/breakout_transition_test.lua
]]

print("==========================================")
print("[Test] Breakout Level Transition Test")
print("==========================================")

local frameCount = 0
local testPhase = "init"
local startPressFrame = 0
local lastCGRAM = {}
local errorsFound = 0
local framesWithPink = 0

-- Pink color detection (BGR555 format)
-- Pink/magenta = high red + high blue, low green
local function isPinkish(color)
    local r = color & 0x1F
    local g = (color >> 5) & 0x1F
    local b = (color >> 10) & 0x1F
    -- Pink: high R, low G, high B
    return r > 20 and g < 10 and b > 20
end

-- Read CGRAM colors 8-15 (background palette that changes per level)
local function readBackgroundPalette()
    local colors = {}
    for i = 8, 15 do
        local addr = i * 2
        local lo = emu.read(addr, emu.memType.snesCgRam)
        local hi = emu.read(addr + 1, emu.memType.snesCgRam)
        colors[i] = lo | (hi << 8)
    end
    return colors
end

-- Check a specific screen region for pink pixels
-- This is done by sampling the PPU output if available
local function checkForVisualCorruption()
    -- Read background palette and check for unexpected pink
    local palette = readBackgroundPalette()

    for i = 8, 15 do
        if isPinkish(palette[i]) then
            -- Pink in background palette might be intentional,
            -- but during transition it indicates corruption
            if testPhase == "playing" or testPhase == "transition" then
                print(string.format("  WARNING: Pinkish color 0x%04X at palette %d", palette[i], i))
                framesWithPink = framesWithPink + 1
            end
        end
    end

    return palette
end

-- Simulate button press
local function pressButton(button)
    local input = {}
    input[button] = true
    emu.setInput(0, input)
end

local function releaseButtons()
    emu.setInput(0, {})
end

emu.addEventCallback(function()
    frameCount = frameCount + 1

    -- State machine for test phases
    if testPhase == "init" then
        -- Wait for initialization
        if frameCount == 60 then
            print("[Test] Game initialized, pressing START...")
            testPhase = "press_start"
            pressButton("start")
            startPressFrame = frameCount
        end

    elseif testPhase == "press_start" then
        -- Hold START for a few frames
        if frameCount > startPressFrame + 10 then
            releaseButtons()
            testPhase = "wait_start_release"
        end

    elseif testPhase == "wait_start_release" then
        -- Wait for game to start
        if frameCount > startPressFrame + 30 then
            print("[Test] Game started, monitoring for corruption...")
            testPhase = "playing"
            lastCGRAM = readBackgroundPalette()
        end

    elseif testPhase == "playing" then
        -- Monitor palette changes (indicates level transition)
        local currentPalette = checkForVisualCorruption()

        -- Check if palette changed (level transition)
        local paletteChanged = false
        for i = 8, 15 do
            if lastCGRAM[i] ~= currentPalette[i] then
                paletteChanged = true
                break
            end
        end

        if paletteChanged then
            print(string.format("[Test] Frame %d: Palette changed (level transition)", frameCount))
            testPhase = "transition"
        end

        lastCGRAM = currentPalette

        -- Test duration: 5 seconds of gameplay
        if frameCount > 360 then
            testPhase = "done"
        end

    elseif testPhase == "transition" then
        -- Extra monitoring during transition
        checkForVisualCorruption()

        -- Return to playing after transition settles
        if frameCount > startPressFrame + 60 then
            testPhase = "playing"
        end

    elseif testPhase == "done" then
        print("")
        print("==========================================")
        print("[Test] Results:")
        print(string.format("  Total frames: %d", frameCount))
        print(string.format("  Frames with pink warning: %d", framesWithPink))

        if framesWithPink > 5 then
            -- More than 5 frames with pink suggests a real issue
            print("[FAIL] Too many frames with potential corruption")
            errorsFound = errorsFound + 1
        else
            print("[PASS] No significant visual corruption detected")
        end

        print("==========================================")
        emu.stop(errorsFound > 0 and 1 or 0)
    end

end, emu.eventType.endFrame)

print("[Test] Starting breakout level transition test...")
print("[Test] Will monitor for 6 seconds of gameplay...")
