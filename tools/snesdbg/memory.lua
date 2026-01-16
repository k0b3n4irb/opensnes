--[[
  memory.lua - Memory Access Helpers for Mesen2

  Provides symbol-aware memory reading/writing using Mesen2's Lua API.

  Mesen2 Memory Types (from KNOWLEDGE.md):
    - snesSpriteRam: OAM data (NOT snesOam!)
    - snesWorkRam: WRAM ($7E-$7F)
    - snesVideoRam: VRAM
    - snesCgRam: CGRAM (palettes)
    - snesMemory: Full address space
    - snesPrgRom: ROM

  Usage:
    local mem = require("memory")
    mem.setSymbols(symbolTable)
    local value = mem.read("monster_x")
    mem.write("monster_x", 120)
]]

local M = {}

-- Symbol table reference
local symbols = nil

--- Set the symbol table for name resolution
function M.setSymbols(symtab)
    symbols = symtab
end

--- Determine the memory type for a given bank
-- @param bank Bank number (0-255)
-- @return Mesen2 memory type
local function getMemType(bank)
    if bank == 0x7E or bank == 0x7F then
        return emu.memType.snesWorkRam
    elseif bank >= 0x00 and bank <= 0x3F then
        -- LoROM: check address range
        -- $0000-$1FFF -> WRAM mirror (use snesWorkRam)
        -- $8000-$FFFF -> ROM
        -- For simplicity, use snesMemory for full address space
        return emu.memType.snesMemory
    else
        return emu.memType.snesMemory
    end
end

--- Convert bank + address to memory type + offset
-- @param bank Bank number
-- @param addr Local address
-- @return memType, offset
local function toMesenAddr(bank, addr)
    if bank == 0x7E then
        -- Direct WRAM access
        return emu.memType.snesWorkRam, addr
    elseif bank == 0x7F then
        -- Extended WRAM
        return emu.memType.snesWorkRam, 0x10000 + addr
    elseif bank >= 0x00 and bank <= 0x3F and addr < 0x2000 then
        -- Bank $00-$3F low range mirrors WRAM
        return emu.memType.snesWorkRam, addr
    else
        -- Use full 24-bit address
        return emu.memType.snesMemory, (bank * 0x10000) + addr
    end
end

--- Read a byte by symbol name
-- @param nameOrAddr Symbol name or full 24-bit address
-- @return byte value
function M.readByte(nameOrAddr)
    local bank, addr

    if type(nameOrAddr) == "string" then
        if not symbols then
            error("Symbols not loaded. Call setSymbols() first.")
        end
        local fullAddr
        fullAddr, bank, addr = symbols:resolve(nameOrAddr)
        if not fullAddr then
            error("Unknown symbol: " .. nameOrAddr)
        end
    else
        bank = math.floor(nameOrAddr / 0x10000)
        addr = nameOrAddr % 0x10000
    end

    local memType, offset = toMesenAddr(bank, addr)
    return emu.read(offset, memType)
end

--- Read a 16-bit word by symbol name (little-endian)
-- @param nameOrAddr Symbol name or full 24-bit address
-- @return 16-bit value
function M.readWord(nameOrAddr)
    local low = M.readByte(nameOrAddr)
    local high

    if type(nameOrAddr) == "string" then
        local fullAddr = symbols:resolve(nameOrAddr)
        high = M.readByte(fullAddr + 1)
    else
        high = M.readByte(nameOrAddr + 1)
    end

    return low + (high * 256)
end

--- Read a signed 16-bit word
-- @param nameOrAddr Symbol name or full 24-bit address
-- @return signed 16-bit value
function M.readWordSigned(nameOrAddr)
    local val = M.readWord(nameOrAddr)
    if val >= 0x8000 then
        return val - 0x10000
    end
    return val
end

--- Read a 24-bit long address (little-endian)
-- @param nameOrAddr Symbol name or full 24-bit address
-- @return 24-bit value
function M.readLong(nameOrAddr)
    local word = M.readWord(nameOrAddr)
    local bank

    if type(nameOrAddr) == "string" then
        local fullAddr = symbols:resolve(nameOrAddr)
        bank = M.readByte(fullAddr + 2)
    else
        bank = M.readByte(nameOrAddr + 2)
    end

    return word + (bank * 0x10000)
end

--- Read multiple bytes
-- @param nameOrAddr Symbol name or full 24-bit address
-- @param count Number of bytes to read
-- @return array of bytes
function M.readBytes(nameOrAddr, count)
    local result = {}
    local baseAddr

    if type(nameOrAddr) == "string" then
        baseAddr = symbols:resolve(nameOrAddr)
        if not baseAddr then
            error("Unknown symbol: " .. nameOrAddr)
        end
    else
        baseAddr = nameOrAddr
    end

    for i = 0, count - 1 do
        result[i + 1] = M.readByte(baseAddr + i)
    end

    return result
end

--- Write a byte by symbol name
-- @param nameOrAddr Symbol name or full 24-bit address
-- @param value Byte value to write
function M.writeByte(nameOrAddr, value)
    local bank, addr

    if type(nameOrAddr) == "string" then
        if not symbols then
            error("Symbols not loaded. Call setSymbols() first.")
        end
        local fullAddr
        fullAddr, bank, addr = symbols:resolve(nameOrAddr)
        if not fullAddr then
            error("Unknown symbol: " .. nameOrAddr)
        end
    else
        bank = math.floor(nameOrAddr / 0x10000)
        addr = nameOrAddr % 0x10000
    end

    local memType, offset = toMesenAddr(bank, addr)
    emu.write(offset, value, memType)
end

--- Write a 16-bit word by symbol name (little-endian)
-- @param nameOrAddr Symbol name or full 24-bit address
-- @param value 16-bit value to write
function M.writeWord(nameOrAddr, value)
    local low = value % 256
    local high = math.floor(value / 256) % 256

    M.writeByte(nameOrAddr, low)

    if type(nameOrAddr) == "string" then
        local fullAddr = symbols:resolve(nameOrAddr)
        M.writeByte(fullAddr + 1, high)
    else
        M.writeByte(nameOrAddr + 1, high)
    end
end

--- Hex dump memory region with symbol annotations
-- @param nameOrAddr Symbol name or full 24-bit address
-- @param count Number of bytes to dump
-- @return formatted hex dump string
function M.hexdump(nameOrAddr, count)
    local baseAddr
    local baseName = nil

    if type(nameOrAddr) == "string" then
        baseName = nameOrAddr
        baseAddr = symbols:resolve(nameOrAddr)
        if not baseAddr then
            error("Unknown symbol: " .. nameOrAddr)
        end
    else
        baseAddr = nameOrAddr
        if symbols then
            baseName = symbols:lookup(baseAddr)
        end
    end

    local lines = {}
    local bytesPerLine = 16

    for offset = 0, count - 1, bytesPerLine do
        local addr = baseAddr + offset
        local hex = {}
        local ascii = {}

        for i = 0, bytesPerLine - 1 do
            if offset + i < count then
                local b = M.readByte(addr + i)
                table.insert(hex, string.format("%02X", b))
                if b >= 32 and b < 127 then
                    table.insert(ascii, string.char(b))
                else
                    table.insert(ascii, ".")
                end
            end
        end

        local label
        if baseName then
            label = string.format("%s+$%04X", baseName, offset)
        else
            label = string.format("$%06X", addr)
        end

        local line = string.format("%-20s %s  %s",
            label,
            table.concat(hex, " "),
            table.concat(ascii))
        table.insert(lines, line)
    end

    return table.concat(lines, "\n")
end

--- Read a struct by symbol and struct definition
-- @param nameOrAddr Base symbol name or address
-- @param structDef Struct definition table
-- @return table with struct fields
function M.readStruct(nameOrAddr, structDef)
    local baseAddr

    if type(nameOrAddr) == "string" then
        baseAddr = symbols:resolve(nameOrAddr)
        if not baseAddr then
            error("Unknown symbol: " .. nameOrAddr)
        end
    else
        baseAddr = nameOrAddr
    end

    local result = {}

    for fieldName, fieldDef in pairs(structDef) do
        local offset = fieldDef.offset or 0
        local size = fieldDef.size or 1
        local signed = fieldDef.signed or false

        local addr = baseAddr + offset

        if size == 1 then
            result[fieldName] = M.readByte(addr)
            if signed and result[fieldName] >= 0x80 then
                result[fieldName] = result[fieldName] - 0x100
            end
        elseif size == 2 then
            if signed then
                result[fieldName] = M.readWordSigned(addr)
            else
                result[fieldName] = M.readWord(addr)
            end
        elseif size == 3 then
            result[fieldName] = M.readLong(addr)
        end
    end

    return result
end

return M
