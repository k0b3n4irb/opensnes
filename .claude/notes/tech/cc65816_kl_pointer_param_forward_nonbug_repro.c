typedef void (*fp)(void);
extern void sink(fp cb, unsigned char bank);
void forward(fp cb) { sink(cb, 0); }

/* control: forward a data pointer param the same way */
extern void dsink(char *p, unsigned char b);
void dforward(char *p) { dsink(p, 0); }
