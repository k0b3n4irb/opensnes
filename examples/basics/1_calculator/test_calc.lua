-- Calculator initial state test
local testsPassed = 0
local testsFailed = 0
local frameCount = 0
local done = false

function log(msg)
    print("[TEST] " .. msg)
end

function pass(name)
    testsPassed = testsPassed + 1
    log("PASS: " .. name)
end

function fail(name, expected, actual)
    testsFailed = testsFailed + 1
    log("FAIL: " .. name .. " (expected: " .. tostring(expected) .. ", got: " .. tostring(actual) .. ")")
end

function readVRAM(wordAddr)
    return emu.read(wordAddr * 2, emu.memType.snesVideoRam, false)
end

function readWRAM(addr)
    return emu.read(addr, emu.memType.snesWorkRam, false)
end

function checkTile(wordAddr, expected, name)
    local actual = readVRAM(wordAddr)
    if actual == expected then
        pass(name)
    else
        fail(name, expected, actual)
    end
end

function onFrame()
    frameCount = frameCount + 1

    if frameCount < 10 then return end

    if not done then
        done = true

        log("=== Calculator Initial State Test ===")
        log("")

        log("-- Checking button grid row 10 (7 8 9 /) --")
        checkTile(0x054A, 8, "Tile '7' at $054A")
        checkTile(0x054E, 9, "Tile '8' at $054E")
        checkTile(0x0552, 10, "Tile '9' at $0552")
        checkTile(0x0556, 14, "Tile '/' at $0556")

        log("")
        log("-- Checking button grid row 12 (4 5 6 *) --")
        checkTile(0x058A, 5, "Tile '4' at $058A")
        checkTile(0x058E, 6, "Tile '5' at $058E")
        checkTile(0x0592, 7, "Tile '6' at $0592")
        checkTile(0x0596, 13, "Tile '*' at $0596")

        log("")
        log("-- Checking button grid row 14 (1 2 3 -) --")
        checkTile(0x05CA, 2, "Tile '1' at $05CA")
        checkTile(0x05CE, 3, "Tile '2' at $05CE")
        checkTile(0x05D2, 4, "Tile '3' at $05D2")
        checkTile(0x05D6, 12, "Tile '-' at $05D6")

        log("")
        log("-- Checking button grid row 16 (0 C = +) --")
        checkTile(0x060A, 1, "Tile '0' at $060A")
        checkTile(0x060E, 16, "Tile 'C' at $060E")
        checkTile(0x0612, 15, "Tile '=' at $0612")
        checkTile(0x0616, 11, "Tile '+' at $0616")

        log("")
        log("-- Checking bracket cursor at position 0 --")
        checkTile(0x0549, 17, "Left bracket '[' at $0549")
        checkTile(0x054B, 18, "Right bracket ']' at $054B")

        log("")
        log("-- Checking WRAM variables --")
        local calcPos = readWRAM(0x002F)
        if calcPos == 0 then
            pass("calc_pos = 0")
        else
            fail("calc_pos", 0, calcPos)
        end

        local calcNewnum = readWRAM(0x0037)
        if calcNewnum == 1 then
            pass("calc_newnum = 1")
        else
            fail("calc_newnum", 1, calcNewnum)
        end

        log("")
        log("=== Test Summary ===")
        log("Passed: " .. testsPassed)
        log("Failed: " .. testsFailed)
        if testsFailed == 0 then
            log("ALL TESTS PASSED!")
        end
    end

    if frameCount > 15 then
        emu.stop()
    end
end

emu.addEventCallback(onFrame, emu.eventType.endFrame)
log("Calculator test loaded")
