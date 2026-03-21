/**
 * @file sprite_dynamic_meta.c
 * @brief Dynamic metasprite engine — C implementation
 *
 * Iterates MetaspriteItem arrays and calls oamDynamic32/16/8Draw for each
 * sub-sprite. Uses the proven single-sprite dynamic draw functions internally.
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#include <snes.h>

void oamMetaDrawDyn32(u16 id, s16 x, s16 y,
                      const MetaspriteItem *meta, u8 *gfxptr) {
    u8 refresh = oambuffer[id].oamrefresh;

    while (meta->dx != metasprite_end) {
        oambuffer[id].oamx = x + meta->dx;
        oambuffer[id].oamy = y + meta->dy;
        oambuffer[id].oamframeid = meta->tile;
        oambuffer[id].oamattribute = meta->attr;
        oambuffer[id].oamrefresh = refresh;
        OAM_SET_GFX(id, gfxptr);
        oamDynamic32Draw(id);
        meta++;
        id++;
    }
}

void oamMetaDrawDyn16(u16 id, s16 x, s16 y,
                      const MetaspriteItem *meta, u8 *gfxptr, u16 sprsize) {
    u8 refresh = oambuffer[id].oamrefresh;
    /* When 16x16 is SMALL (sprsize == OBJ_SMALL), tile bit 8 must be set
     * to address the second name table in VRAM ($1000+) */
    u8 attr_or = (sprsize == OBJ_SMALL) ? 0x01 : 0x00;

    while (meta->dx != metasprite_end) {
        oambuffer[id].oamx = x + meta->dx;
        oambuffer[id].oamy = y + meta->dy;
        oambuffer[id].oamframeid = meta->tile;
        oambuffer[id].oamattribute = meta->attr | attr_or;
        oambuffer[id].oamrefresh = refresh;
        OAM_SET_GFX(id, gfxptr);
        oamDynamic16Draw(id);
        meta++;
        id++;
    }
}

void oamMetaDrawDyn8(u16 id, s16 x, s16 y,
                     const MetaspriteItem *meta, u8 *gfxptr) {
    u8 refresh = oambuffer[id].oamrefresh;

    while (meta->dx != metasprite_end) {
        oambuffer[id].oamx = x + meta->dx;
        oambuffer[id].oamy = y + meta->dy;
        oambuffer[id].oamframeid = meta->tile;
        oambuffer[id].oamattribute = meta->attr;
        oambuffer[id].oamrefresh = refresh;
        OAM_SET_GFX(id, gfxptr);
        oamDynamic8Draw(id);
        meta++;
        id++;
    }
}
