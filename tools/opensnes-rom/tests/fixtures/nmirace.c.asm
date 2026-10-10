main:
	pea.w my_cb
	jsl nmiSet
	rts
my_cb:
	jsr fill
	rts
fill:
	sta.w $2180
	rts
