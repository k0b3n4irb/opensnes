/* The scaled index of a loop over parallel arrays is computed once per
 * iteration, not in front of every access: the address of each access is
 * kept next to it, its `i * 2` is not copied with it (2026-10-10, issue
 * #166; gcm.c, sinkref). */
typedef unsigned short u16;
typedef short s16;
extern s16 px[18], py[18], dx[18], dy[18];
extern u16 dirty[18], depth[18];

void move_all(void) {
    u16 i;
    for (i = 0; i < 18; i++) {
        if (dx[i] | dy[i]) {
            s16 x = px[i], y = py[i];
            if (x > 592) x = 592;
            if (y > 1104) y = 1104;
            px[i] = x + dx[i];
            py[i] = y + dy[i];
        } else if (!dirty[i])
            continue;
        dirty[i] = 0;
        if (depth[i] != py[i])
            depth[i] = py[i];
    }
}
