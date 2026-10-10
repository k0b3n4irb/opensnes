/* A `__ramcode` function is never copied into its caller: it must run from
 * the RAM code window, and inlined it would run from wherever the caller
 * is (2026-10-10: a `RAM_CODE static` with one call site was absorbed into
 * a ROM caller and the Super FX test ROM never finished; inline.c). The
 * other way round is fine: an ordinary helper poured into a RAM function
 * runs from RAM with it. */
typedef unsigned short u16;
typedef unsigned char u8;
extern volatile u8 status;
extern u16 polls, frames;

__ramcode static void wait_in_ram(void) {
    u16 n = 0;
    while (!(status & 0x80))
        n++;
    polls = n;
}

/* one call site, static: everything else about it says "absorb" */
void launch(void) {
    status = 1;
    wait_in_ram();
    frames++;
}

static u16 twice(u16 v) { return v + v; }

/* an ordinary static with one call site, called from RAM code: absorbed */
__ramcode void ram_caller(void) {
    polls = twice(frames);
}
