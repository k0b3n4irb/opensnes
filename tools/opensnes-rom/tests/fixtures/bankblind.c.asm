main:
	lda.w #mapdata
	tax
	lda.l $0000,x
	rts
