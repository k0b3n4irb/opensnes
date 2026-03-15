-- debug_div16.lua — Mesen2 diagnostic for __div16 from stateLineClear
--
-- Load in Mesen2: Script Window > Open Script > Run
-- Then play Tetris until you clear a line.
-- Check Debug > Log Window for output.
--
-- Addresses from tetris.sym (current build with debug_div_result):

local DIV16_ENTRY   = 0x0085B8   -- tcc_div16 entry
local DIV16_RTL     = 0x0085F0   -- tcc_div16 hardware path RTL
local DIV16_DIVZERO = 0x0085F1   -- tcc_div16 @div_zero
local DIV16_SOFTDIV = 0x0085F9   -- tcc_div16 @software_div

local SLC_IF_TRUE   = 0x00BB9E   -- stateLineClear @if_true.97 (completion code)
local SLC_AFTER_DIV = 0x00BBAF   -- PEA #0 after sta.w debug_div_result
local SLC_FOR_JOIN  = 0x00BC62   -- @for_join.102 (after level-up loop)
local SLC_IF_FALSE  = 0x00BCF2   -- @if_false.98 (normal exit)
local SLC_GAMEOVER  = 0x00BCF9   -- @if_true.105 (game over exit)

-- WRAM addresses (use emu.memType.snesWorkRam)
local GAME_STATE    = 0x007F
local FLASH_TIMER   = 0x01E7
local TOTAL_LINES   = 0x007B
local DEBUG_DIV_RES = 0x007D
local CLEAR_RESULT  = 0x0072
local SCORE         = 0x0077
local LEVEL         = 0x0079
local TCC_R0        = 0x0000
local TCC_R1        = 0x0004

-- Tracking state
local in_slc_completion = false
local hit_count = 0
local MAX_HITS = 5

local function rb(addr)
    return emu.read(addr, emu.memType.snesWorkRam)
end

local function rw(addr)
    return rb(addr) + rb(addr + 1) * 256
end

local function dumpState(label)
    emu.log(string.format("  [%s] gameState=%d flash=%d totalLines=%d score=%d level=%d divResult=%d",
        label, rb(GAME_STATE), rb(FLASH_TIMER), rw(TOTAL_LINES),
        rw(SCORE), rw(LEVEL), rw(DEBUG_DIV_RES)))
    emu.log(string.format("    clearResult: count=%d rows=[%d,%d,%d,%d]",
        rb(CLEAR_RESULT), rb(CLEAR_RESULT+1), rb(CLEAR_RESULT+2),
        rb(CLEAR_RESULT+3), rb(CLEAR_RESULT+4)))
    emu.log(string.format("    tcc_r0=$%04X tcc_r1=$%04X", rw(TCC_R0), rw(TCC_R1)))
end

-- BP 1: Completion code entry (flash_timer >= 10)
emu.addMemoryCallback(function()
    if hit_count >= MAX_HITS then return end
    hit_count = hit_count + 1
    in_slc_completion = true
    emu.log("=============================================")
    emu.log(string.format(">>> LINE CLEAR COMPLETION #%d", hit_count))
    dumpState("ENTRY")
end, emu.callbackType.exec, SLC_IF_TRUE)

-- BP 2: __div16 entry (only when called from stateLineClear completion)
emu.addMemoryCallback(function()
    if not in_slc_completion then return end
    emu.log(string.format("  >>> __div16 ENTRY: r0=$%04X (dividend=%d) r1=$%04X (divisor=%d)",
        rw(TCC_R0), rw(TCC_R0), rw(TCC_R1), rw(TCC_R1)))
end, emu.callbackType.exec, DIV16_ENTRY)

-- BP 3: __div16 normal return
emu.addMemoryCallback(function()
    if not in_slc_completion then return end
    emu.log(string.format("  >>> __div16 RTL: r0=$%04X (quotient=%d) r1=$%04X (remainder=%d)",
        rw(TCC_R0), rw(TCC_R0), rw(TCC_R1), rw(TCC_R1)))
end, emu.callbackType.exec, DIV16_RTL)

-- BP 4: __div16 div-by-zero path (should NOT happen)
emu.addMemoryCallback(function()
    if not in_slc_completion then return end
    emu.log("  !!! __div16 DIV_ZERO PATH (divisor was zero!)")
end, emu.callbackType.exec, DIV16_DIVZERO)

-- BP 5: __div16 software division path (should NOT happen for divisor=10)
emu.addMemoryCallback(function()
    if not in_slc_completion then return end
    emu.log(string.format("  !!! __div16 SOFTWARE_DIV PATH (r1=$%04X)", rw(TCC_R1)))
end, emu.callbackType.exec, DIV16_SOFTDIV)

-- BP 6: After sta.w debug_div_result (proves __div16 returned successfully)
emu.addMemoryCallback(function()
    if not in_slc_completion then return end
    emu.log(string.format("  >>> After div16: divResult=$%04X gameState=%d",
        rw(DEBUG_DIV_RES), rb(GAME_STATE)))
end, emu.callbackType.exec, SLC_AFTER_DIV)

-- BP 7: @for_join (after level-up loop completed)
emu.addMemoryCallback(function()
    if not in_slc_completion then return end
    emu.log("  >>> FOR_JOIN reached (level-up loop done)")
    dumpState("LOOP_DONE")
end, emu.callbackType.exec, SLC_FOR_JOIN)

-- BP 8: Normal function exit
emu.addMemoryCallback(function()
    if not in_slc_completion then return end
    emu.log("  >>> NORMAL EXIT (@if_false.98)")
    dumpState("EXIT")
    emu.log("=============================================")
    in_slc_completion = false
end, emu.callbackType.exec, SLC_IF_FALSE)

-- BP 9: Game over exit (unexpected during line clear)
emu.addMemoryCallback(function()
    if not in_slc_completion then return end
    emu.log("  !!! GAME OVER EXIT (@if_true.105)")
    dumpState("GAMEOVER")
    emu.log("=============================================")
    in_slc_completion = false
end, emu.callbackType.exec, SLC_GAMEOVER)

emu.log("=== div16 debug script loaded ===")
emu.log("=== Play until you clear a line, then check this log ===")
