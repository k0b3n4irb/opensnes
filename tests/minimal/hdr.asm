;==============================================================================
; Minimal OpenSNES Test ROM - Header
;==============================================================================

.MEMORYMAP
    SLOTSIZE $8000
    DEFAULTSLOT 0
    SLOT 0 $8000
    SLOT 1 $0000
.ENDME

.ROMBANKSIZE $8000
.ROMBANKS 2

.SNESHEADER
    ID "OPEN"
    NAME "OPENSNES MINIMAL    "
    SLOWROM
    LOROM
    CARTRIDGETYPE $00
    ROMSIZE $07
    SRAMSIZE $00
    COUNTRY $01
    LICENSEECODE $00
    VERSION $00
.ENDSNES

.SNESNATIVEVECTOR
    COP EmptyHandler
    BRK EmptyHandler
    ABORT EmptyHandler
    NMI EmptyHandler
    IRQ EmptyHandler
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

.SECTION "Vectors" SEMIFREE

EmptyHandler:
    rti

.ENDS
