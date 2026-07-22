typedef unsigned char u8;
typedef unsigned short u16;

typedef struct {
    u16 spcAddress;
    u16 size;
} S;

static S mirror[4];

u8 getinfo(u8 id, S *info) {
    if (id >= 4 || !info) {
        return 2;
    }
    info->spcAddress = mirror[id].spcAddress;
    info->size = mirror[id].size;
    return 0;
}
