main:
	pea.w ramtab
	lda.w #ramtab
	tax
	lda.l ramtab,x
	rts
