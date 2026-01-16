; Library memory map for compiling library object files
; This provides minimal configuration for wla-65816 to create .o files

.MEMORYMAP
  SLOTSIZE $10000
  DEFAULTSLOT 0
  SLOT 0 $0000 SIZE $10000 "RAMSECTION"
  SLOT 1 $0000 SIZE $10000 "CODE"
.ENDME

.ROMBANKMAP
  BANKSTOTAL 2
  BANKSIZE $10000
  BANKS 2
.ENDRO
