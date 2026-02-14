;==============================================================================
; ROM Header - LoROM
;==============================================================================

.MEMORYMAP
    SLOTSIZE $8000
    DEFAULTSLOT 0
    SLOT 0 $8000 $8000
    SLOT 1 $0000 $2000
    SLOT 2 $2000 $E000
    SLOT 3 $0000 $10000
.ENDME

.ROMBANKSIZE $8000
.ROMBANKS 8

.SNESHEADER
    ID "OPEN"
    NAME "AUDIO TESTS           "
    SLOWROM
    LOROM
    CARTRIDGETYPE $00
    ROMSIZE $06
    SRAMSIZE $00
    COUNTRY $01
    LICENSEECODE $00
    VERSION $00
.ENDSNES

.SNESNATIVEVECTOR
    COP EmptyHandler
    BRK EmptyHandler
    ABORT EmptyHandler
    NMI NmiHandler
    IRQ IrqHandler
.ENDNATIVEVECTOR

.SNESEMUVECTOR
    COP EmptyHandler
    ABORT EmptyHandler
    NMI EmptyHandler
    RESET Start
    IRQBRK EmptyHandler
.ENDEMUVECTOR

.BANK 0
.ORG 0

.SECTION ".empty_handler" SEMIFREE
EmptyHandler:
    sep #$20
    lda $4210
    rti
.ENDS
