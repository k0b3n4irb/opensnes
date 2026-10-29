;==============================================================================
; The VRAM queue's storage, at the project's size (make/common.mk assembles
; this file when `vramqueue` is in LIB_MODULES; VRAM_QUEUE_MAX, 32 by default).
;
; The library is built once for every project, so its code cannot be
; assembled at a project's size: the arrays live here, and the library
; reads the capacity from vram_queue_max (lib/source/vramqueue.asm). C sees
; the same number as VRAM_QUEUE_MAX (snes/vramqueue.h, -DVRAM_QUEUE_MAX).
; 12 bytes of the C RAM band per entry.
;==============================================================================

.RAMSECTION ".vram_queue" BANK 0 SLOT 1
    vram_queue_src      dsb VRAM_QUEUE_MAX_VAL*2    ; source address
    vram_queue_bank     dsb VRAM_QUEUE_MAX_VAL*2    ; its bank (low byte)
    vram_queue_addr     dsb VRAM_QUEUE_MAX_VAL*2    ; VRAM word address
    vram_queue_size     dsb VRAM_QUEUE_MAX_VAL*2    ; bytes
    vram_queue_step     dsb VRAM_QUEUE_MAX_VAL*2    ; VMAIN (low byte)
    vram_queue_cum      dsb VRAM_QUEUE_MAX_VAL*2    ; vram_queue_cost up to this entry
    vram_queue_count    dsb 2                       ; entries waiting
    vram_queue_cost     dsb 2                       ; bytes + VRAM_QUEUE_ENTRY_COST each
.ENDS

.SECTION ".rodata.vram_queue_max" SUPERFREE
vram_queue_max:
    .dw VRAM_QUEUE_MAX_VAL                          ; read with .l by the library
.ENDS
