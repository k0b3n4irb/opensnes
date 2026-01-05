
; Function: main (framesize=2, slots=0, alloc_slots=0)
.SECTION ".text.main" SUPERFREE
main:
	php
	rep #$30
@start.1:
	jmp @body.2
@body.2:
	jmp @while_cond.3
@while_cond.3:
	jmp @while_body.4
@while_body.4:
	jmp @while_cond.3
.ENDS
/* end function main */


; End of generated code
