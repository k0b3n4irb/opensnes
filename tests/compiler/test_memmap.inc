; Minimal memory map for testing assembly syntax
; This is included at the top of test assembly files

.MEMORYMAP
  SLOTSIZE $8000
  DEFAULTSLOT 0
  SLOT 0 $8000
.ENDME

.ROMBANKMAP
  BANKSTOTAL 1
  BANKSIZE $8000
  BANKS 1
.ENDRO

.BANK 0
.ORG 0
