/*---------------------------------------------------------------------------------

    Copyright (C) 2022
        Alekmaul

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any
    damages arising from the use of this software.

    Permission is granted to anyone to use this software for any
    purpose, including commercial applications, and to alter it and
    redistribute it freely, subject to the following restrictions:

    1.	The origin of this software must not be misrepresented; you
        must not claim that you wrote the original software. If you use
        this software in a product, an acknowledgment in the product
        documentation would be appreciated but is not required.
    2.	Altered source versions must be plainly marked as such, and
        must not be misrepresented as being the original software.
    3.	This notice may not be removed or altered from any source
        distribution.

    Convert Tiled tmx file to binary files compatible with pvsneslib

    Tiled is a tool to make graphic maps based on tiles
        https://www.mapeditor.org/

    2026-10-05 (OpenSNES): the conversion is this library, level_convert();
    tmx2snes's main() and opensnes-level call it. Errors come back in a
    buffer (longjmp from where the tool used to exit(1)), progress lines go
    to a callback.

---------------------------------------------------------------------------------*/

#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "level.h"

/* cute_tiled traps (SIGILL) on a truncated or malformed map by design; a
 * CLI should say what happened instead. Found by the fuzz harness on an
 * empty .tmj (gaps review H5, 2026-09-15). */
static void level_map_crash(void);
#define CUTE_TILED_CRASH() level_map_crash()
#define CUTE_TILED_IMPLEMENTATION
#include "cute_tiled.h"

#define HI_BYTE(n) (((int)n >> 8) & 0x00ff) // extracts the hi-byte of a word
#define LOW_BYTE(n) ((int)n & 0x00ff)       // extracts the low-byte of a word

#define N_METATILES 1024 // maximum tiles
#define N_OBJECTS 64     // maximum objects

//// M A I N   V A R I A B L E S ////////////////////////////////////////////////
typedef struct
{
    int x;    // x coordinate in pixels.
    int y;    // y coordinate in pixels.
    int type; // type of object (0=main character, 1..63 other types)
    int minx; // horizontal or vertical min x coordinate in pixels.
    int maxx; // horizontal or vertical max x coordinate in pixels.
} pvsneslib_object_t;

static FILE *fpi, *fpo;       // input and output file handlers
static unsigned int filesize; // input file size
static char filebase[1024];   // output base (the map's path without extension)
static char filemapname[1100]; // output filename for map & objects

static int *data;                          // data from Tiled layer
static cute_tiled_layer_t *layer;          // layers from Tiled  map
static cute_tiled_tileset_t *tset;         // tileset from Tiled
static cute_tiled_object_t *objm;          // objects from Tiled layer objects
static cute_tiled_map_t *map;              // map from Tiled
static cute_tiled_tile_descriptor_t *tile; // tiles from Tiled tiles attributes
static cute_tiled_property_t *propm;       // properties from Tiled tiles properties
static unsigned short tileprop
    [N_METATILES]
    [3];                                // to store tiles properties in correct order with index 0:attribute, 1:priority and 2:palette
static pvsneslib_object_t objsnes[N_OBJECTS];  // to store objects in correct order
static unsigned short tilesetmap[N_METATILES]; // to have map for each tile (optimization purpose)

/* the error path: where the tool used to print and exit(1) */
static jmp_buf level_jmp;
static char *level_err;
static size_t level_errlen;
static const level_opts *level_o;
static level_info_fn level_info;
static void *level_user;

#define FAIL(...) do { snprintf(level_err, level_errlen, __VA_ARGS__); longjmp(level_jmp, 1); } while (0)
#define INFO(...) do { if (level_info) { char _l[1400]; snprintf(_l, sizeof _l, __VA_ARGS__); level_info(_l, level_user); } } while (0)

static void level_map_crash(void)
{
    FAIL("malformed or truncated Tiled JSON map (cute_tiled aborted the parse)");
}

//// F U N C T I O N S //////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
static void PutWord(int data, FILE *fp)
{
    putc(LOW_BYTE(data), fp);
    putc(HI_BYTE(data), fp);
} // end of PutWord

/* <dir of filebase>/<layer>.<ext> */
static void layer_file(const char *ext)
{
    char *lastpostslash;
    strcpy(filemapname, filebase);
    lastpostslash = strrchr(filemapname, '/');
    if (lastpostslash != NULL)
        sprintf(lastpostslash + 1, "%s%s", layer->name.ptr, ext);
    else
        sprintf(filemapname, "%s%s", layer->name.ptr, ext);
}

//////////////////////////////////////////////////////////////////////////////
// -Q : a quadrant-ordered 64x64 tilemap, for the `background` module.
//
// The .m16 format above is what the `map` module streams. A game that
// just scrolls a fixed 64x64 area with bgSetScroll needs the layout the
// PPU actually reads: four 32x32 pages (TL, TR, BL, BR), no header, and
// the per-tile palette and priority folded into each entry so the blob
// can go straight to VRAM.
static void WriteQuadrantMap(void)
{
    int qx, qy, tx, ty, gx, gy, tileattr, tilesnes;

    if (map->width != 64 || map->height != 64)
        FAIL("-Q needs a 64x64 map (this one is %dx%d): a SNES 64x64 background is four 32x32 pages; "
             "any other size has no quadrant layout", map->width, map->height);

    layer_file(".q16");
    INFO("    Writing quadrant tiles map file...");
    fpo = fopen(filemapname, "wb");
    if (fpo == NULL)
        FAIL("Can't open quadrant map file [%s] for writing", filemapname);

    data = layer->data;
    for (qy = 0; qy < 2; qy++)
    {
        for (qx = 0; qx < 2; qx++)
        {
            for (ty = 0; ty < 32; ty++)
            {
                for (tx = 0; tx < 32; tx++)
                {
                    gx = qx * 32 + tx;
                    gy = qy * 32 + ty;
                    tileattr = data[gy * map->width + gx];
                    if (tileattr)
                    {
                        int t = (tileattr - 1) & 0x03FF;
                        tilesnes = t;
                        tilesnes |= (tileprop[t][2] & 0x07) << 10;  // palette
                        if (tileprop[t][1])
                            tilesnes |= (1 << 13);                  // priority
                        if (tileattr & CUTE_TILED_FLIPPED_HORIZONTALLY_FLAG)
                            tilesnes |= (1 << 14);
                        if (tileattr & CUTE_TILED_FLIPPED_VERTICALLY_FLAG)
                            tilesnes |= (1 << 15);
                        PutWord(tilesnes, fpo);
                    }
                    else
                        PutWord(0x0000, fpo);
                }
            }
        }
    }
    fclose(fpo);
}

//////////////////////////////////////////////////////////////////////////////
// -C : one collision byte per map CELL.
//
// The .b16 output is per TILESET TILE: 32 bytes for a 16-tile tileset,
// which a game indexes after reading the tile id back out of the
// tilemap. That is the right shape for the `map` module. A game that
// keeps its own map in ROM and asks "is the tile at (x,y) solid?" wants
// the answer already flattened — one byte per cell, indexed directly,
// which is what collideTile() takes.
//
// Costs w*h bytes instead of 32, and buys a lookup with no indirection.
// For a 64x64 map that is 4 KB, which belongs outside bank $00 anyway
// (ASSET_SECTION).
static void WriteCellCollision(void)
{
    int i, idx;

    layer_file(".c16");
    INFO("    Writing per-cell collision grid...");
    fpo = fopen(filemapname, "wb");
    if (fpo == NULL)
        FAIL("Can't open collision grid [%s] for writing", filemapname);

    data = layer->data;
    for (i = 0; i < layer->data_count; i++)
    {
        int solid = 0;
        if (data[i])
        {
            idx = (data[i] - 1) & 0x03FF;
            solid = tileprop[idx][0] ? 1 : 0;
        }
        fputc(solid, fpo);
    }
    fclose(fpo);
}

//////////////////////////////////////////////////////////////////////////////
// -e : the Entities layer as C defines.
//
// Tiled's object layer is where a designer puts the spawn point, the
// NPCs, the doors — with custom properties on each (an NPC's line of
// dialogue, a door's destination). The .o16 output packs that into the
// `object` module's binary format, which is no use to a game that does
// not use that module. This writes the same information as a header you
// #include, so that adding a villager is a map edit.
//
// Objects are grouped by their Tiled TYPE. For each type:
//     <TYPE>_COUNT           how many there are
//     <TYPE>_FIELDS          a struct-member list: tx, ty, then one
//                            member per custom property
//     <TYPE>_TABLE           a brace-enclosed initialiser, one row each
// and, when there is exactly one, the scalar forms <TYPE>_TX, <TYPE>_TY,
// <TYPE>_<PROP> as well.
//
// The table is an array of structs, which is what a game wants to write.
// It briefly was not: until 2026-07-22 a `const` array of structs indexed
// at runtime lost its bank byte, so this emitted parallel scalar tables
// and the README called it a design choice. It was a compiler bug
// (issue #132) and it is fixed; the workaround is gone with it.
static void upcase_ident(const char *in, char *out, int outsz)
{
    int i;
    for (i = 0; i < outsz - 1 && in[i] != '\0'; i++)
    {
        char c = in[i];
        if (c >= 'a' && c <= 'z')
            c = (char)(c - 'a' + 'A');
        else if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')))
            c = '_';
        out[i] = c;
    }
    out[i] = '\0';
}

static void WriteEntityHeader(void)
{
    cute_tiled_object_t *o, *p;
    char hdrname[1200];
    char type[64], prop[64];
    int i, count, first, tw, th;
    FILE *fph;

    if (level_o->entities_header_path)
        snprintf(hdrname, sizeof hdrname, "%s", level_o->entities_header_path);
    else
        snprintf(hdrname, sizeof hdrname, "%s.inc", filebase);
    fph = fopen(hdrname, "w");
    if (fph == NULL)
        FAIL("Can't open entity header [%s] for writing", hdrname);
    INFO("    Writing entity header [%s]...", hdrname);

    tw = map->tilewidth;
    th = map->tileheight;

    fprintf(fph, "/* Generated from the Entities layer by %s.\n"
                 " * Do not edit: edit the map. */\n", level_o->generator ? level_o->generator : "tmx2snes -e");

    // one pass per distinct type, in first-seen order
    for (o = layer->objects; o != NULL; o = o->next)
    {
        // skip if this type was already emitted
        for (p = layer->objects; p != o; p = p->next)
        {
            if (strcmp(p->type.ptr, o->type.ptr) == 0)
                break;
        }
        if (p != o)
            continue;

        upcase_ident(o->type.ptr, type, sizeof(type));
        count = 0;
        for (p = layer->objects; p != NULL; p = p->next)
            if (strcmp(p->type.ptr, o->type.ptr) == 0)
                count++;

        fprintf(fph, "\n#define %s_COUNT %d\n", type, count);

        // the struct shape: tx, ty, then one member per custom property
        fprintf(fph, "#define %s_FIELDS \\\n    u8 tx; u8 ty;", type);
        for (i = 0; i < o->property_count; i++)
        {
            cute_tiled_property_t *pr = o->properties + i;
            char lower[64];
            int k;
            for (k = 0; k < 63 && pr->name.ptr[k] != '\0'; k++)
            {
                char ch = pr->name.ptr[k];
                if (ch >= 'A' && ch <= 'Z')
                    ch = (char)(ch - 'A' + 'a');
                else if (!((ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9')))
                    ch = '_';
                lower[k] = ch;
            }
            lower[k] = '\0';
            if (pr->type == CUTE_TILED_PROPERTY_STRING)
                fprintf(fph, " const char *%s;", lower);
            else
                fprintf(fph, " u16 %s;", lower);
        }
        fprintf(fph, "\n");

        // the rows
        fprintf(fph, "#define %s_TABLE {", type);
        first = 1;
        for (p = layer->objects; p != NULL; p = p->next)
        {
            int j;
            if (strcmp(p->type.ptr, o->type.ptr) != 0)
                continue;
            fprintf(fph, "%s \\\n    { %d, %d", first ? "" : ",",
                    (int)(p->x) / tw, (int)(p->y) / th);
            first = 0;
            for (i = 0; i < o->property_count; i++)
            {
                cute_tiled_property_t *pr = o->properties + i;
                for (j = 0; j < p->property_count; j++)
                {
                    cute_tiled_property_t *q = p->properties + j;
                    if (strcmp(q->name.ptr, pr->name.ptr) != 0)
                        continue;
                    if (q->type == CUTE_TILED_PROPERTY_STRING)
                        fprintf(fph, ", \"%s\"", q->data.string.ptr);
                    else if (q->type == CUTE_TILED_PROPERTY_INT)
                        fprintf(fph, ", %d", q->data.integer);
                    else if (q->type == CUTE_TILED_PROPERTY_BOOL)
                        fprintf(fph, ", %d", q->data.boolean ? 1 : 0);
                    else
                        fprintf(fph, ", 0");
                    break;
                }
                if (j >= p->property_count)
                    fprintf(fph, ", 0");
            }
            fprintf(fph, " }");
        }
        fprintf(fph, " \\\n}\n");

        // scalars for a lone object of its type — the common case for a
        // spawn point, a door, an exit
        if (count == 1)
        {
            fprintf(fph, "#define %s_TX %d\n", type, (int)(o->x) / tw);
            fprintf(fph, "#define %s_TY %d\n", type, (int)(o->y) / th);
            for (i = 0; i < o->property_count; i++)
            {
                cute_tiled_property_t *pr = o->properties + i;
                upcase_ident(pr->name.ptr, prop, sizeof(prop));
                fprintf(fph, "#define %s_%s ", type, prop);
                if (pr->type == CUTE_TILED_PROPERTY_STRING)
                    fprintf(fph, "\"%s\"\n", pr->data.string.ptr);
                else if (pr->type == CUTE_TILED_PROPERTY_INT)
                    fprintf(fph, "%d\n", pr->data.integer);
                else if (pr->type == CUTE_TILED_PROPERTY_BOOL)
                    fprintf(fph, "%d\n", pr->data.boolean ? 1 : 0);
                else
                    fprintf(fph, "0\n");
            }
        }
    }
    fclose(fph);
}

//////////////////////////////////////////////////////////////////////////////
static void WriteMap(void)
{
    int tileattr, tilesnes, i;
    unsigned gid;

    layer_file(".m16");
    INFO("    Writing tiles map file...");
    fpo = fopen(filemapname, "wb");
    if (fpo == NULL)
        FAIL("Can't open layer map file [%s] for writing", filemapname);

    // write map header: width, height in pixels and size in bytes
    PutWord(map->width * map->tilewidth, fpo);
    PutWord(map->height * map->tileheight, fpo);
    PutWord(layer->data_count * 2, fpo);

    data = layer->data;
    fflush(stdout);
    for (i = 0; i < layer->data_count; i++)
    {
        tileattr = data[i];
        if (tileattr)
        {
            gid = tileattr & 0x0FFFFFFF;
            if (tileattr & CUTE_TILED_FLIPPED_DIAGONALLY_FLAG)
            {
                fclose(fpo);
                FAIL("tile %d of layer [%s] is rotated (diagonal flip): "
                     "the SNES tilemap has no rotation, only horizontal and vertical flips",
                     i, layer->name.ptr ? layer->name.ptr : "?");
            }
            if (gid > N_METATILES)
            {
                fclose(fpo);
                FAIL("tile %d of layer [%s] uses id %u: the SNES tilemap holds %d tiles (ids 1..%d)",
                     i, layer->name.ptr ? layer->name.ptr : "?", gid, N_METATILES, N_METATILES);
            }
            tilesnes = gid - 1;
            if (tileattr & CUTE_TILED_FLIPPED_HORIZONTALLY_FLAG) // Flipx attribute
                tilesnes |= (1 << 14);
            if (tileattr & CUTE_TILED_FLIPPED_VERTICALLY_FLAG) // Flipy attribute
                tilesnes |= (1 << 15);
            PutWord(tilesnes, fpo);
        }
        else
            PutWord(0x0000, fpo);
    }
    fclose(fpo);
}

//////////////////////////////////////////////////////////////////////////////
static void WriteTileset(void)
{
    int i, blkprop;
    char *pend;

    INFO("Writing tiles attribute file...");
    sprintf(filemapname, "%s.b16", filebase);
    fpo = fopen(filemapname, "wb");
    if (fpo == NULL)
        FAIL("Can't open tiles attribute file [%s] for writing", filemapname);

    tset = map->tilesets;
    tile = tset->tiles;
    if (tset->tilecount > N_METATILES)
    {
        fclose(fpo);
        FAIL("too much tiles in tileset (%d tiles, %d max expected)", tset->tilecount, N_METATILES);
    }

    // get tiles properties
    memset(tileprop, 0x00, sizeof(tileprop));
    while (tile)
    {
        for (i = 0; i < tile->property_count; i++)
        {
            propm = tile->properties + i;
            if (strcmp(propm->name.ptr, "attribute") == 0)
            {
                blkprop = (unsigned short)strtol(propm->data.string.ptr, &pend, 16);
                tileprop[tile->tile_index][0] = blkprop;
            }
            if (strcmp(propm->name.ptr, "priority") == 0)
            {
                blkprop = (unsigned short)strtol(propm->data.string.ptr, &pend, 16);
                tileprop[tile->tile_index][1] = blkprop;
            }
            if (strcmp(propm->name.ptr, "palette") == 0)
            {
                blkprop = (unsigned short)strtol(propm->data.string.ptr, &pend, 16);
                tileprop[tile->tile_index][2] = blkprop;
            }
        }
        tile = tile->next;
    }

    fflush(stdout);
    INFO("    Writing %d tiles attributes to file...", tset->tilecount);
    for (i = 0; i < tset->tilecount; i++)
    {
        PutWord(tileprop[i][0], fpo);
    }
    fclose(fpo);
}

//////////////////////////////////////////////////////////////////////////////
static void WriteMapTileset(void)
{
    int i, blkprop;

    sprintf(filemapname, "%s.t16", filebase);
    fpo = fopen(filemapname, "wb");
    if (fpo == NULL)
        FAIL("Can't open tiles properties file [%s] for writing", filemapname);

    fflush(stdout);
    INFO("    Writing %d tiles properties to file...", tset->tilecount);
    for (i = 0; i < tset->tilecount; i++)
    {
        blkprop = tilesetmap[i] & 0x03FF;            // get tile number
        blkprop |= tileprop[i][1] ? 0x2000 : 0x0000; // check priority
        blkprop |= (tileprop[i][2] << 10);           // check palette
        PutWord(blkprop, fpo);
    }
    fclose(fpo);
}

//////////////////////////////////////////////////////////////////////////////
static void WriteEntities(void)
{
    int i, blkprop, objidx;
    char *pend;

    INFO("Writing entities object file...");
    sprintf(filemapname, "%s.o16", filebase);
    fpo = fopen(filemapname, "wb");
    if (fpo == NULL)
        FAIL("Can't open layer object file [%s] for writing", filemapname);

    objm = layer->objects;
    if (layer->object_count > N_OBJECTS)
    {
        fclose(fpo);
        FAIL("too much entities in map (%d entities, %d max expected)", layer->object_count, N_OBJECTS);
    }

    fflush(stdout);
    memset(objsnes, 0x00, sizeof(objsnes));
    objidx = layer->object_count - 1;
    if (layer->object_count)
    {
        while (objm)
        {
            objsnes[objidx].type = atoi(
                objm->type.ptr); //(unsigned short) strtol(objm->type.ptr,&pend,10);
            objsnes[objidx].x = (int)(objm->x);
            objsnes[objidx].y = (int)(objm->y);
            for (i = 0; i < objm->property_count; i++)
            {
                propm = objm->properties + i;
                if (strcmp(propm->name.ptr, "minx") == 0)
                {
                    blkprop = (unsigned short)strtol(propm->data.string.ptr, &pend, 10);
                    objsnes[objidx].minx = blkprop;
                }
                if (strcmp(propm->name.ptr, "maxx") == 0)
                {
                    blkprop = (unsigned short)strtol(propm->data.string.ptr, &pend, 10);
                    objsnes[objidx].maxx = blkprop;
                }
            }
            objm = objm->next;
            objidx--;
        }
    }

    INFO("    Writing %d objects to file...", layer->object_count);
    for (i = 0; i < layer->object_count; i++)
    {
        PutWord(objsnes[i].x, fpo);
        PutWord(objsnes[i].y, fpo);
        PutWord(objsnes[i].type, fpo);
        PutWord(objsnes[i].minx, fpo);
        PutWord(objsnes[i].maxx, fpo);
    }
    PutWord(0xFFFF, fpo);
    fclose(fpo);
}

//////////////////////////////////////////////////////////////////////////////
int level_convert(const char *tmj_path, const char *tileset_map_path, const char *outbase,
                  const level_opts *opts, level_result *res, level_info_fn info, void *user,
                  char *err, size_t errlen)
{
    static const level_opts defaults = { 0, 0, 0, 0, "tmx2snes -e", NULL };
    char *json = NULL;

    level_err = err; level_errlen = errlen; level_o = opts ? opts : &defaults;
    level_info = info; level_user = user;
    map = NULL; fpi = NULL; fpo = NULL;
    if (res) memset(res, 0, sizeof *res);
    if (errlen) err[0] = '\0';
    snprintf(filebase, sizeof filebase, "%s", outbase);

    if (setjmp(level_jmp))
    {
        if (fpi) fclose(fpi);
        if (json) free(json);
        if (map) cute_tiled_free_map(map);
        return -1;
    }

    fpi = fopen(tmj_path, "rb");
    if (fpi == NULL)
        FAIL("Can't open file [%s]", tmj_path);
    fseek(fpi, 0, SEEK_END);
    filesize = ftell(fpi);
    fseek(fpi, 0, SEEK_SET);

    INFO("Loading map: [%s]", tmj_path);
    {
        json = (char *)malloc((size_t)filesize + 1);
        if (json == NULL)
            FAIL("Out of memory reading [%s]", tmj_path);
        size_t got = fread(json, 1, (size_t)filesize, fpi);
        json[got] = '\0';
        /* cute_tiled wants the JSON without insignificant whitespace */
        size_t w = 0;
        int in_string = 0, escaped = 0;
        for (size_t r = 0; r < got; r++)
        {
            char c = json[r];
            if (in_string)
            {
                json[w++] = c;
                if (escaped)          escaped = 0;
                else if (c == '\\')   escaped = 1;
                else if (c == '"')    in_string = 0;
                continue;
            }
            if (c == '"') { in_string = 1; json[w++] = c; continue; }
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') continue;
            json[w++] = c;
        }
        map = cute_tiled_load_map_from_memory(json, (int)w, 0);
        if (map && map->tilesets && map->tilesets->next)
            FAIL("the map uses two or more tilesets: tmx2snes converts one tileset per map "
                 "(the first, [%s]); merge them in Tiled or split the layers",
                 map->tilesets->name.ptr ? map->tilesets->name.ptr : "?");
        if (map == NULL)
        {
            if (cute_tiled_error_reason != NULL)
                FAIL("Cannot load map [%s]: %s (json line %d); is it a Tiled JSON map (.tmj / .json)?",
                     tmj_path, cute_tiled_error_reason, cute_tiled_error_line);
            FAIL("Cannot load map [%s]; is it a Tiled JSON map (.tmj / .json)?", tmj_path);
        }
        free(json); json = NULL;
    }
    fclose(fpi); fpi = NULL;

    // the tileset's optimisation map (gfx4snes / opensnes-tileset .map); inspect may pass none
    memset(tilesetmap, 0, sizeof tilesetmap);
    if (tileset_map_path)
    {
        fpi = fopen(tileset_map_path, "rb");
        if (fpi == NULL)
            FAIL("Can't open file [%s]", tileset_map_path);
        fseek(fpi, 0, SEEK_END);
        filesize = ftell(fpi);
        fseek(fpi, 0, SEEK_SET);
        if (filesize > N_METATILES * 2) // no more than nb metatiles in words
            FAIL("tileset map file is too big [%u bytes]", filesize);
        if (fread(tilesetmap, filesize, 1, fpi) != 1 && filesize)
            FAIL("Can't read file [%s]", tileset_map_path);
        fclose(fpi); fpi = NULL;
    }
    else if (!level_o->dry_run)
        FAIL("no tileset map: the .map the tileset converter wrote is needed to write .t16");

    if ((map->width * map->height) > 16384)
        FAIL("map is too big (max 32K)! (%dK)", (map->width * map->height * 2) / 1024);
    if (map->height > 256)
        FAIL("map height is too big! (max 256) (%d)", map->height);
    if ((map->tilewidth != 8) || (map->tileheight != 8))
        FAIL("tile width or height are not 8px! (%d %d)", map->tilewidth, map->tileheight);

    if (res)
    {
        res->map_width = map->width; res->map_height = map->height;
        res->tilewidth = map->tilewidth; res->tileheight = map->tileheight;
        res->tilecount = map->tilesets ? map->tilesets->tilecount : 0;
        res->object_count = -1;
        for (layer = map->layers; layer; layer = layer->next)
        {
            if (strcmp(layer->name.ptr, "Entities") == 0)
                res->object_count = layer->object_count;
            else if (res->nlayers < LEVEL_MAX_LAYERS)
                snprintf(res->layer_name[res->nlayers++], sizeof res->layer_name[0], "%s", layer->name.ptr);
        }
    }
    if (level_o->dry_run)
    {
        cute_tiled_free_map(map); map = NULL;
        return 0;
    }

    // loop over the map's layers and write them to disk
    INFO("Writing layers map & object files...");
    layer = map->layers;
    // write .b16 file first (to have priority flag of each tile for map...
    WriteTileset();
    while (layer)
    {
        INFO("Found layer %s...", layer->name.ptr);
        // if it is an entity layer
        if (strcmp(layer->name.ptr, "Entities") == 0)
        {
            // Write .o16 file ...
            WriteEntities();
            if (level_o->entities_header)
                WriteEntityHeader();
        }
        // No it is a map layer
        else
        {
            // write .m16 and .t16 files ...
            WriteMap();
            WriteMapTileset();
            if (level_o->quadrant)
                WriteQuadrantMap();
            if (level_o->cell_collision)
                WriteCellCollision();
        }
        layer = layer->next;
    }

    // free the Tiled map object
    cute_tiled_free_map(map); map = NULL;
    return 0;
}
