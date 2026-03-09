;==============================================================================
; Initialized Data Section
;==============================================================================
; Base section for static variable initialization.
; Compiled init records are appended using APPENDTO directive.
; CopyInitData routine in crt0.asm copies values from ROM to RAM at startup.
;
; Format of each init record:
;   [ram_addr:2][size:2][data:N]
;
; End marker: [0:2][0:2]
;==============================================================================

.SECTION ".data_init" SEMIFREE
DataInitStart:
    .db 0   ; Placeholder byte to prevent section optimization
.ENDS
