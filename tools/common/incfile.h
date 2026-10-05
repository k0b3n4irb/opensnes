/* incfile.h — the .inc header and the _data.as .incbin fragment of a
 * converted graphic, in the lib's asset.h naming (see incfile.c). */
#ifndef OPENSNES_INCFILE_H
#define OPENSNES_INCFILE_H

#include <stddef.h>
#include <stdio.h>

typedef struct {
    const char *generator;     /* "opensnes-tileset 1.0.0" */
    int bpp;                   /* 2, 4 or 8 */
    int has_pal, has_map, has_meta, mode7;
    int map_blocks_x, map_blocks_y;   /* the tilemap's size in entries, when has_map */
    int lz;                    /* the tiles are LZ77-compressed: no bundle, lzssDecodeVram at runtime */
} incfile_spec;

/* Writes <outbase>.inc and <outbase>_data.as for the asset named `name`
 * (a C identifier). Returns 0, or -1 with the reason in err[]. */
int incfile_write(const char *outbase, const char *name, const incfile_spec *spec, char *err, size_t errlen);

/* One blob of a _data.as: its own ASSET_SECTION named <name>_<what>, or one per 32 KB part
 * (<name>_<what>_1, ...); each with a <label>_end. For tools whose outputs are not pictures. */
void incfile_write_blob(FILE *f, const char *name, const char *what, const char *path);

#endif
