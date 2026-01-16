#!/usr/bin/env lua
--[[
  test_symbols_standalone.lua - Test symbol parser without Mesen2

  This script tests the symbols.lua parser using standard Lua.
  It can be run directly: lua test_symbols_standalone.lua

  Usage:
    lua test_symbols_standalone.lua [path/to/game.sym]
]]

-- Add current directory to path
package.path = package.path .. ";./?.lua"

local symbols = require("symbols")

-- Test helper
local passed = 0
local failed = 0

local function test(name, condition, msg)
    if condition then
        print("[PASS] " .. name)
        passed = passed + 1
    else
        print("[FAIL] " .. name .. (msg and (": " .. msg) or ""))
        failed = failed + 1
    end
end

-- Get .sym file from command line or use default
local symFile = arg[1] or "../debug-fixtures/clean/hello_world/hello_world.sym"

print("===========================================")
print("  snesdbg Symbol Parser Test")
print("===========================================")
print("")
print("Testing with: " .. symFile)
print("")

-- Test 1: Load symbols
print("--- Loading Symbols ---")
local ok, symtab = pcall(function()
    return symbols.load(symFile)
end)

test("Load symbols file", ok, tostring(symtab))

if not ok then
    print("Cannot continue without loading symbols")
    os.exit(1)
end

-- Test 2: Symbol count
local count = symtab:count()
test("Symbol count > 0", count > 0, "count = " .. count)
print("  Loaded " .. count .. " symbols")

-- Test 3: Check for specific symbols (based on OpenSNES structure)
print("")
print("--- Symbol Resolution ---")

-- Common symbols in OpenSNES ROMs
local commonSymbols = {
    "tcc__r0",
    "oamMemory",
    "main",
    "NmiHandler"
}

for _, name in ipairs(commonSymbols) do
    local addr, bank, localAddr = symtab:resolve(name)
    if addr then
        test("Resolve '" .. name .. "'", true,
            string.format("$%02X:%04X", bank, localAddr))
        print(string.format("  %s = $%02X:%04X (full: $%06X)", name, bank, localAddr, addr))
    else
        test("Resolve '" .. name .. "'", false, "symbol not found")
    end
end

-- Test 4: Symbol existence check
print("")
print("--- Symbol Existence ---")
test("exists('main')", symtab:exists("main"))
test("exists('nonexistent_xyz')", not symtab:exists("nonexistent_xyz"))

-- Test 5: Address lookup
print("")
print("--- Address Lookup ---")
local mainAddr = symtab:resolve("main")
if mainAddr then
    local foundName = symtab:lookup(mainAddr)
    test("Lookup main address", foundName == "main", "found: " .. tostring(foundName))
end

-- Test 6: Bank $00 symbols (WRAM low)
print("")
print("--- Bank $00 Symbols (WRAM Low) ---")
local bank00 = symtab:getBank(0x00)
local bank00Count = 0
for _, _ in ipairs(bank00) do bank00Count = bank00Count + 1 end
test("Has Bank $00 symbols", bank00Count > 0, "count = " .. bank00Count)

-- Count WRAM-range symbols
local wramCount = 0
for _, entry in ipairs(bank00) do
    if entry.addr < 0x2000 then
        wramCount = wramCount + 1
    end
end
print(string.format("  Bank $00 symbols in WRAM range ($0000-$1FFF): %d", wramCount))

-- Test 7: Check for oamMemory location
print("")
print("--- OAM Memory Location ---")
local oamAddr, oamBank, oamLocal = symtab:resolve("oamMemory")
if oamAddr then
    print(string.format("  oamMemory at $%02X:%04X", oamBank, oamLocal))

    -- Check if it's in the dangerous WRAM mirror range
    if oamBank == 0x7E and oamLocal < 0x2000 then
        print("  WARNING: oamMemory is in WRAM mirror range!")
        print("  This may cause overlaps with Bank $00 variables.")
    elseif oamBank == 0x00 and oamLocal < 0x2000 then
        print("  oamMemory is in Bank $00 WRAM range (normal for small buffer)")
    else
        print("  oamMemory is safely outside WRAM mirror range")
    end
else
    print("  oamMemory symbol not found")
end

-- Test 8: WRAM overlap detection
print("")
print("--- WRAM Overlap Detection ---")
local overlaps = symtab:checkWRAMOverlap()
if #overlaps == 0 then
    test("No WRAM overlaps", true)
else
    test("No WRAM overlaps", false, #overlaps .. " overlaps found")
    for _, overlap in ipairs(overlaps) do
        print("  " .. overlap.msg)
    end
end

-- Test 9: Address formatting
print("")
print("--- Address Formatting ---")
if mainAddr then
    local formatted = symtab:formatAddr(mainAddr)
    test("formatAddr() includes symbol name", formatted:find("main") ~= nil, formatted)
    print("  " .. formatted)

    -- Test offset formatting
    local formattedOffset = symtab:formatAddr(mainAddr + 5)
    print("  main+5: " .. formattedOffset)
end

-- Test 10: Find nearest symbol
print("")
print("--- Find Nearest Symbol ---")
if mainAddr then
    local nearName, nearOffset = symtab:findNearest(mainAddr + 10)
    if nearName then
        test("findNearest() works", nearName == "main" and nearOffset == 10,
            string.format("found %s+$%02X", nearName, nearOffset))
    else
        test("findNearest() works", false, "no result")
    end
end

-- Summary
print("")
print("===========================================")
print("  Results")
print("===========================================")
print(string.format("  Passed: %d", passed))
print(string.format("  Failed: %d", failed))
print("")

if failed == 0 then
    print("  ALL TESTS PASSED!")
    os.exit(0)
else
    print("  SOME TESTS FAILED")
    os.exit(1)
end
