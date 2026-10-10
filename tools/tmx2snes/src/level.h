/* level.h — the Tiled (.tmj) to SNES map data converter of tmx2snes, as a
 * library: tmx2snes's main() and opensnes-level call it, so the bytes of
 * .m16 / .b16 / .t16 / .o16 / .q16 / .c16 are the same whoever asks. */
#ifndef TMX2SNES_LEVEL_H
#define TMX2SNES_LEVEL_H

#include <stddef.h>

#define LEVEL_MAX_LAYERS 8

typedef struct {
    int entities_header;          /* write the Entities layer as C defines */
    int quadrant;                 /* <layer>.q16: a quadrant-ordered 64x64 tilemap */
    int cell_collision;           /* <layer>.c16: one collision byte per map cell */
    int dry_run;                  /* load, validate, fill the result; write nothing */
    const char *generator;        /* named in the entities header ("tmx2snes -e") */
    const char *entities_header_path;  /* NULL: <outbase>.inc */
} level_opts;

typedef struct {
    int map_width, map_height;    /* in tiles */
    int tilewidth, tileheight;    /* in pixels (8) */
    int tilecount;                /* entries of .b16 / .t16 */
    int nlayers;                  /* tile layers (<layer>.m16 beside outbase) */
    char layer_name[LEVEL_MAX_LAYERS][64];
    int object_count;             /* -1: no Entities layer (no .o16) */
} level_result;

/* A progress line ("    Writing tiles map file..."); may be NULL. */
typedef void (*level_info_fn)(const char *line, void *user);

/* Converts tmj_path with the tileset's optimisation .map (tileset_map_path)
 * into <outbase>.b16 / .t16 / .o16, <dir of outbase>/<layer>.m16 (.q16, .c16)
 * and the entities header. Returns 0, or -1 with the reason in err[]. */
int level_convert(const char *tmj_path, const char *tileset_map_path, const char *outbase,
                  const level_opts *opts, level_result *res, level_info_fn info, void *user,
                  char *err, size_t errlen);

#endif
