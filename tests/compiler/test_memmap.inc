; Minimal memory map for testing assembly syntax
; This is included at the top of test assembly files

.MEMORYMAP
  SLOTSIZE $8000
  DEFAULTSLOT 0
  SLOT 0 $8000       ; ROM slot
  SLOT 1 $0000 $2000 ; RAM slot (WRAM at $7E:0000)
.ENDME

.ROMBANKMAP
  BANKSTOTAL 1
  BANKSIZE $8000
  BANKS 1
.ENDRO

.RAMSECTION "zeropage" BANK 0 SLOT 1
.ENDS

.BANK 0
.ORG 0
