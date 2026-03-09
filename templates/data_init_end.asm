;==============================================================================
; Initialized Data End Marker
;==============================================================================
; End marker for init data table (target_addr = 0 signals end of records)
;==============================================================================

.SECTION ".data_init_end" SEMIFREE APPENDTO ".data_init"
    .dw 0, 0    ; End marker: addr=0, size=0
DataInitEnd:
.ENDS
