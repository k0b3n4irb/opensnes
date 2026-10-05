main:
	pea.w :mapdata
	pea.w mapdata
	jsl mapLoad
	lda.w counter
	rts
helper:
	lda.w #counter
	rts
