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

---------------------------------------------------------------------------------*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CUTE_TILED_IMPLEMENTATION
#include "cute_tiled.h"

#define TMX2SNESVERSION __BUILD_VERSION
#define TMX2SNESDATE __BUILD_DATE

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

int quietmode = 0;     // 0 = not quiet, 1 = i can't say anything :P
FILE *fpi, *fpo;       // input and output file handlers
unsigned int filesize; // input file size
char filebase[256];    // input filename
char filebasetil[256]; // input filename for tiles (map filename)
char filemapname[260]; // output filename for map & objects

int *data;                          // data from Tiled layer
cute_tiled_layer_t *layer;          // layers from Tiled  map
cute_tiled_tileset_t *tset;         // tileset from Tiled
cute_tiled_object_t *objm;          // objects from Tiled layer objects
cute_tiled_map_t *map;              // map from Tiled
cute_tiled_tile_descriptor_t *tile; // tiles from Tiled tiles attributes
cute_tiled_property_t *propm;       // properties from Tiled tiles properties
unsigned short tileprop
    [N_METATILES]
    [3];                                // to store tiles properties in correct order with index 0:attribute, 1:priority and 2:palette
pvsneslib_object_t objsnes[N_OBJECTS];  // to store objects in correct order
unsigned short tilesetmap[N_METATILES]; // to have map for each tile (optimization purpose)

//// F U N C T I O N S //////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
void PutWord(int data, FILE *fp)
{
    putc(LOW_BYTE(data), fp);
    putc(HI_BYTE(data), fp);
} // end of PutWord

// Structure representing a software version number.
typedef struct
{
    int major; // Major version number.
    int minor; // Minor version number.
} Version;

// Convert a floating point version number to its major and minor components.
void floatToVersion(float version, Version *v)
{

    // Convert the floating point version to an integer representation by multiplying by 100.
    // This allows us to handle two decimal places. Additionally, add 0.5 to round to the nearest integer
    // to account for potential floating point inaccuracies.
    int fullVersion = (int)(version * 100 + 0.5);

    // The major version is obtained by dividing the integer representation by 100.
    v->major = fullVersion / 100;

    // The minor version is obtained by taking the remainder when divided by 100.
    v->minor = fullVersion % 100;
}

// Compare two Version structures to determine their order.
int compareVersions(Version v1, Version v2)
{
    // Compare the major version numbers first.
    if (v1.major < v2.major)
        return -1;
    if (v1.major > v2.major)
        return 1;

    // If major version numbers are equal, compare the minor version numbers.
    if (v1.minor < v2.minor)
        return -1;
    if (v1.minor > v2.minor)
        return 1;

    // Both major and minor version numbers are equal.
    return 0;
}

//////////////////////////////////////////////////////////////////////////////
void PrintOptions(char *str)
{
    printf("\n\nUsage : tmx2snes [options] tmxfilename mapfilename");
    printf("\n  where tmxfilename is a Tiled tmx file (in json format)");
    printf("\n        mapfilename is the map file of tileset for tileset optimization");
    printf("\n\n  tmx2snes will do:");
    printf("\n  options:");
    printf("\n  	-e  also write <map>.inc, the Entities layer as C defines");
    printf("\n  	-Q  also write <layer>.q16, a quadrant-ordered 64x64 tilemap");
    printf("\n  	-C  also write <layer>.c16, one collision byte per map cell");
    printf("\n  	-q  quiet");
    printf("\n");
    printf("\n  	.m16 file for map");
    printf("\n  	.b16 file for tileset attribute (blocker, etc...)");
    printf("\n  	.o16 file for objects");
    printf("\n  	.t16 file for tileset properties (palette,priority)");

    if (str[0] != 0)
        printf("\ntmx2snes: error 'The [%s] parameter is not recognized'", str);

    printf("\n\nMisc options:");
    printf("\n-h                Display this information");
    printf("\n-q                Quiet mode");
    printf("\n-v                Display version information");
    printf("\n");

} // end of PrintOptions()

//////////////////////////////////////////////////////////////////////////////
void PrintVersion(void)
{
    printf("tmx2snes (" TMX2SNESDATE ") version " TMX2SNESVERSION "");
    printf("\nCopyright (c) 2022 Alekmaul\n");
}


//////////////////////////////////////////////////////////////////////////////
// -Q : a quadrant-ordered 64x64 tilemap, for the `background` module.
//
// The .m16 format above is what the `map` module streams. A game that
// just scrolls a fixed 64x64 area with bgSetScroll needs the layout the
// PPU actually reads: four 32x32 pages (TL, TR, BL, BR), no header, and
// the per-tile palette and priority folded into each entry so the blob
// can go straight to VRAM.
void WriteQuadrantMap(void)
{
    int qx, qy, tx, ty, gx, gy, tileattr, tilesnes;
    char *lastpostslash;

    if (map->width != 64 || map->height != 64)
    {
        printf("tmx2snes: error '-Q needs a 64x64 map (this one is %dx%d)'\n",
               map->width, map->height);
        printf("tmx2snes:        A SNES 64x64 background is four 32x32 pages;\n");
        printf("tmx2snes:        any other size has no quadrant layout.\n");
        exit(1);
    }

    strcpy(filemapname, filebase);
    lastpostslash = strrchr(filemapname, '/');
    if (lastpostslash != NULL)
        sprintf(lastpostslash + 1, "%s.q16", layer->name.ptr);
    else
        sprintf(filemapname, "%s.q16", layer->name.ptr);

    if (quietmode == 0)
        printf("tmx2snes:     Writing quadrant tiles map file...\n");
    fpo = fopen(filemapname, "wb");
    if (fpo == NULL)
    {
        printf("tmx2snes: error 'Can't open quadrant map file [%s] for writing'\n", filemapname);
        exit(1);
    }

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
void WriteCellCollision(void)
{
    int i, idx;
    char *lastpostslash;

    strcpy(filemapname, filebase);
    lastpostslash = strrchr(filemapname, '/');
    if (lastpostslash != NULL)
        sprintf(lastpostslash + 1, "%s.c16", layer->name.ptr);
    else
        sprintf(filemapname, "%s.c16", layer->name.ptr);

    if (quietmode == 0)
        printf("tmx2snes:     Writing per-cell collision grid...\n");
    fpo = fopen(filemapname, "wb");
    if (fpo == NULL)
    {
        printf("tmx2snes: error 'Can't open collision grid [%s] for writing'\n", filemapname);
        exit(1);
    }

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
    for (i = 0; in[i] != '\0' && i < outsz - 1; i++)
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

void WriteEntityHeader(void)
{
    cute_tiled_object_t *o, *p;
    char hdrname[1024];
    char type[64], prop[64];
    int i, count, first, tw, th;
    FILE *fph;

    strcpy(hdrname, filebase);
    {
        char *dot = strrchr(hdrname, '.');
        if (dot != NULL)
            *dot = '\0';
    }
    strcat(hdrname, ".inc");

    fph = fopen(hdrname, "w");
    if (fph == NULL)
    {
        printf("tmx2snes: error 'Can't open entity header [%s] for writing'\n", hdrname);
        exit(1);
    }
    if (quietmode == 0)
        printf("tmx2snes:     Writing entity header [%s]...\n", hdrname);

    tw = map->tilewidth;
    th = map->tileheight;
    fprintf(fph, "/* Generated from the Entities layer by tmx2snes -e.\n"
                 " * Do not edit: edit the map. */\n");

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
            for (k = 0; pr->name.ptr[k] != '\0' && k < 63; k++)
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
void WriteMap(void)
{
    int tileattr, tilesnes, i;
    char *lastpostslash;

    // We use directory and replace file name with layer name
    strcpy(filemapname, filebase);
    lastpostslash = strrchr(filemapname, '/');
    if (lastpostslash != NULL)
        sprintf(lastpostslash + 1, "%s.m16", layer->name.ptr);
    else
        sprintf(filemapname, "%s.m16", layer->name.ptr);

    if (quietmode == 0)
        printf("tmx2snes:     Writing tiles map file...\n");
    // sprintf(filemapname,"%s.m16",layer->name.ptr);
    fpo = fopen(filemapname, "wb");
    if (fpo == NULL)
    {
        printf("tmx2snes: error 'Can't open layer map file [%s] for writing'\n", filemapname);
        exit(1);
    }
    // Put width & height
    PutWord(map->width * map->tilewidth, fpo);
    PutWord(map->height * map->tileheight, fpo);

    // Put number of tiles
    PutWord(layer->data_count * 2, fpo);

    // Write tile data to file
    // Tiled data format for flipping : flip x ->0x8000 0000 flip y -> 0x4000 0000
    // SNES format :
    // 		High     Low          Legend->  c: Starting character (tile) number
    // 		vhopppcc cccccccc               h: horizontal flip  v: vertical flip
    //				                        p: palette number   o: priority bit
    data = layer->data;
    fflush(stdout);
    for (i = 0; i < layer->data_count; i++)
    {
        // is data not "empty" ?
        tileattr = data[i];
        if (tileattr)
        {
            tilesnes = (tileattr - 1) & 0x03FF; // keep on the low 16bits of tile number

            if (tileattr & CUTE_TILED_FLIPPED_HORIZONTALLY_FLAG) // Flipx attribute
                tilesnes |= (1 << 14);
            if (tileattr & CUTE_TILED_FLIPPED_VERTICALLY_FLAG) // Flipy attribute
                tilesnes |= (1 << 15);

            PutWord(tilesnes, fpo);
        }
        // no (certainly an error in the map with no tile assignment), write 0
        else
            PutWord(0x0000, fpo);
    }
    // close current layer map file
    fclose(fpo);
}

void WriteTileset(void)
{
    int i, blkprop;
    char *pend;

    if (quietmode == 0)
        printf("tmx2snes: Writing tiles attribute file...\n");
    sprintf(filemapname, "%s.b16", filebase);
    fpo = fopen(filemapname, "wb");
    if (fpo == NULL)
    {
        printf("tmx2snes: error 'Can't open tiles attribute file [%s] for writing'\n", filemapname);
        exit(1);
    }

    // Write tile properties to file
    // 2 data are currently managed : priority & block attribute
    // tiles with library is in reverse order (why ?)
    tset = map->tilesets;
    tile = tset->tiles;

    if (tset->tilecount > N_METATILES)
    {
        printf("tmx2snes: error 'too much tiles in tileset (%d tiles, %d max expected)'\n",
               tset->tilecount,
               N_METATILES);
        fclose(fpo);
        exit(1);
    }

    // browse and store in table
    memset(tileprop, 0x00, sizeof(tileprop));
    while (tile)
    {
        // browse through all properties of current tile
        for (i = 0; i < tile->property_count; i++)
        {
            propm = tile->properties + i;
            // write attribute (blocker, etc..) property (which is a string)
            if (strcmp(propm->name.ptr, "attribute") == 0)
            {
                blkprop = (unsigned short)strtol(propm->data.string.ptr, &pend, 16);
                tileprop[tile->tile_index][0] = blkprop;
            }
            // write priority property (which is a string)
            if (strcmp(propm->name.ptr, "priority") == 0)
            {
                blkprop = (unsigned short)strtol(propm->data.string.ptr, &pend, 16);
                tileprop[tile->tile_index][1] = blkprop;
            }
            // write palette property (which is a string)
            if (strcmp(propm->name.ptr, "palette") == 0)
            {
                blkprop = (unsigned short)strtol(propm->data.string.ptr, &pend, 16);
                tileprop[tile->tile_index][2] = blkprop;
            }
        }
        // switch to next tile
        tile = tile->next;
    }

    // now write to file
    fflush(stdout);
    if (quietmode == 0)
        printf("tmx2snes:     Writing %d tiles attributes to file...\n", tset->tilecount);

    for (i = 0; i < tset->tilecount; i++)
    {
        PutWord(tileprop[i][0], fpo);
    }

    // close current layer attribute file
    fclose(fpo);
}

void WriteMapTileset(void)
{
    int i, blkprop;

    // now write tileset file with properties (only priority & palette for the moment)
    sprintf(filemapname, "%s.t16", filebase);
    fpo = fopen(filemapname, "wb");
    if (fpo == NULL)
    {
        printf("tmx2snes: error 'Can't open tiles properties file [%s] for writing'\n", filemapname);
        exit(1);
    }

    // now write to file
    fflush(stdout);
    if (quietmode == 0)
        printf("tmx2snes:     Writing %d tiles properties to file...\n", tset->tilecount);

    for (i = 0; i < tset->tilecount; i++)
    {
        // compute attribute to match with vhopppcc cccccccc
        blkprop = tilesetmap[i] & 0x03FF;            // get tile number
        blkprop |= tileprop[i][1] ? 0x2000 : 0x0000; // check priority
        blkprop |= (tileprop[i][2] << 10);           // check palette
        PutWord(blkprop, fpo);
    }

    // close current layer attribute file
    fclose(fpo);
}

void WriteEntities(void)
{
    int i, blkprop, objidx;
    char *pend;

    if (quietmode == 0)
        printf("tmx2snes: Writing entities object file...\n");
    sprintf(filemapname, "%s.o16", filebase);
    fpo = fopen(filemapname, "wb");
    if (fpo == NULL)
    {
        printf("tmx2snes: error 'Can't open layer object file [%s] for writing'\n", filemapname);
        exit(1);
    }

    // write objects to file
    objm = layer->objects;
    if (layer->object_count > N_OBJECTS)
    {
        printf("tmx2snes: error 'too much entities in map (%d entities, %d max expected)'\n",
               layer->object_count,
               N_OBJECTS);
        exit(1);
    }

    // browse and store in table
    fflush(stdout);
    memset(objsnes, 0x00, sizeof(objsnes));
    objidx = layer->object_count - 1;

    // if we have some objects to store
    if (layer->object_count)
    {
        while (objm)
        {
            // put object in reverse order
            objsnes[objidx].type = atoi(
                objm->type.ptr); //(unsigned short) strtol(objm->type.ptr,&pend,10);
            objsnes[objidx].x = (int)(objm->x);
            objsnes[objidx].y = (int)(objm->y);
            for (i = 0; i < objm->property_count; i++)
            {
                propm = objm->properties + i;
                // write blocker property (which is a string)
                if (strcmp(propm->name.ptr, "minx") == 0)
                {
                    blkprop = (unsigned short)strtol(propm->data.string.ptr, &pend, 10);
                    objsnes[objidx].minx = blkprop;
                }
                // write prio property (which is a string)
                if (strcmp(propm->name.ptr, "maxx") == 0)
                {
                    blkprop = (unsigned short)strtol(propm->data.string.ptr, &pend, 10);
                    objsnes[objidx].maxx = blkprop;
                }
            }

            // switch to next object
            objm = objm->next;
            objidx--;
        }
    }

    // now write to file
    if (quietmode == 0)
        printf("tmx2snes:     Writing %d objects to file...\n", layer->object_count);
    for (i = 0; i < layer->object_count; i++)
    {
        PutWord(objsnes[i].x, fpo);
        PutWord(objsnes[i].y, fpo);
        PutWord(objsnes[i].type, fpo);
        PutWord(objsnes[i].minx, fpo);
        PutWord(objsnes[i].maxx, fpo);
    }
    PutWord(0xFFFF, fpo);

    // close current layer map file
    fclose(fpo);
}

/// M A I N ////////////////////////////////////////////////////////////
int emitheader = 0;   // -e: write <base>.inc, the entities as C defines
int quadrant = 0;     // -Q: write <layer>.q16, a quadrant-ordered 64x64 map
int cellcollision = 0; // -C: write <layer>.c16, one collision byte per map cell

int main(int argc, char **argv)
{
    int i;

    CUTE_TILED_UNUSED(argc);
    CUTE_TILED_UNUSED(argv);

    // init all filenames
    strcpy(filebase, "");
    strcpy(filebasetil, "");
    strcpy(filemapname, "");

    // parse the arguments
    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            if (argv[i][1] == 'v') // show version
            {
                PrintVersion();
                exit(0);
            }
            else if (argv[i][1] == 'h') // show help
            {
                PrintOptions((char *)"");
                exit(0);
            }
            else if (argv[i][1] == 'q') // quiet mode
            {
                quietmode = 1;
            }
            else if (argv[i][1] == 'e') // emit a C header of entities
            {
                emitheader = 1;
            }
            else if (argv[i][1] == 'Q') // quadrant-ordered 64x64 tilemap
            {
                quadrant = 1;
            }
            else if (argv[i][1] == 'C') // per-CELL collision grid
            {
                cellcollision = 1;
            }
            else // invalid option
            {
                PrintOptions(argv[i]);
                exit(1);
            }
        }
        else
        {
            // its not an option flag, so it must be the filebase
            if (filebase[0] != 0) // if already defined... there's a problem
            {
                if (filebasetil[0] != 0) // if already defined... there's a problem
                {
                    PrintOptions(argv[i]);
                    exit(1);
                }
                else // not defined, ok it is the map file
                {
                    strcpy(filebasetil, argv[i]);
                }
            }
            else // not defined, ok it is the tmx file
            {
                strcpy(filebase, argv[i]);
            }
        }
    }

    // make sure options are valid
    if (filebase[0] == 0)
    {
        printf("\ntmx2snes: error 'You must specify a tmx filename'");
        PrintOptions("");
        exit(1);
    }
    if (filebasetil[0] == 0)
    {
        printf("\ntmx2snes: error 'You must specify a tileset map filename'");
        PrintOptions("");
        exit(1);
    }

    // open the tmx file
    fpi = fopen(filebase, "rb");
    if (fpi == NULL)
    {
        printf("\ntmx2snes: error 'Can't open file [%s]'", filebase);
        exit(1);
    }

    // get filesize
    fseek(fpi, 0, SEEK_END);
    filesize = ftell(fpi);
    fseek(fpi, 0, SEEK_SET);

    // load the map in memory
    if (quietmode == 0)
        printf("tmx2snes: Loading map: [%s]\n", filebase);

    // cute_tiled's parser does not skip insignificant whitespace, so a
    // pretty-printed .tmj — anything written with an indent, which is
    // what a generator script produces by default — fails to parse.
    // Tiled's own export happens to be compact, so the trap only springs
    // on generated maps, which is exactly what a content pipeline makes.
    // Minify the buffer ourselves rather than make every generator author
    // discover this.
    {
        char *json = (char *)malloc((size_t)filesize + 1);
        if (json == NULL)
        {
            printf("tmx2snes: error 'Out of memory reading [%s]'\n", filebase);
            fclose(fpi);
            return 1;
        }
        size_t got = fread(json, 1, (size_t)filesize, fpi);
        json[got] = '\0';

        // strip whitespace outside string literals, in place
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
        if (map == NULL)
        {
            printf("tmx2snes: error 'Cannot load map [%s]'\n", filebase);
            if (cute_tiled_error_reason != NULL)
                printf("tmx2snes:        %s (json line %d)\n",
                       cute_tiled_error_reason, cute_tiled_error_line);
            printf("tmx2snes:        Is it a Tiled JSON map (.tmj / .json)?\n");
            free(json);
            fclose(fpi);
            return 1;
        }
        free(json);
    }

    // close the input file
    fclose(fpi);

    // open the tileset map file
    fpi = fopen(filebasetil, "rb");
    if (fpi == NULL)
    {
        printf("\ntmx2snes: error 'Can't open file [%s]'", filebasetil);
        exit(1);
    }

    // get filesize
    fseek(fpi, 0, SEEK_END);
    filesize = ftell(fpi);
    fseek(fpi, 0, SEEK_SET);

    if (filesize > N_METATILES * 2) // no more than nb metatiles in words
    {
        printf("\ntmx2snes: error 'tileset map file is too big [%d bytes]'", filesize);
        fclose(fpi);
        exit(1);
    }

    // read the map
    fread(tilesetmap, filesize, 1, fpi);

    // close the input file
    fclose(fpi);

    /*
    // Setting the minimum supported version.
    Version minExportSupportedVersion = {1, 9};
    // Setting the maximum supported version.
    Version maxExportSupportedVersion = {1, 10};

    // Convert the map's version (which is a float) to a Version struct for easy comparison.
    Version mapVersion;
    floatToVersion(map->version, &mapVersion);

    // Check if the map's version is outside of the supported range.
    if (compareVersions(mapVersion, minExportSupportedVersion) < 0  // If map version is less than minimum supported version
        || compareVersions(mapVersion, maxExportSupportedVersion) > 0) { // Or if map version is greater than maximum supported version

        printf("tmx2snes: error 'the export version you used (%d.%d) is not supported. The "
               "tool supports only the versions from %d.%d to %d.%d.'\n",
               mapVersion.major,
               mapVersion.minor,
               minExportSupportedVersion.major,
               minExportSupportedVersion.minor,
               maxExportSupportedVersion.major,
               maxExportSupportedVersion.minor);

        return 1;
    }
    */

    if ((map->width * map->height) > 16384)
    {
        printf("tmx2snes: error 'map is too big (max 32K)! (%dK)'\n",
               (map->width * map->height * 2) / 1024);
        return 1;
    }
    if (map->height > 256)
    {
        printf("tmx2snes: error 'map height is too big! (max 256) (%d)'\n", map->height);
        return 1;
    }
    if ((map->tilewidth != 8) || (map->tileheight != 8))
    {
        printf("tmx2snes: error 'tile width or height are not 8px! (%d %d)\n",
               map->tilewidth,
               map->tileheight);
        return 1;
    }

    // remove filename extension (tmj or json)
    if (filebase[strlen(filebase) - 5] == '.')
    {
        filebase[strlen(filebase) - 5] = '\0';
    }
    else if (filebase[strlen(filebase) - 4] == '.')
    {
        filebase[strlen(filebase) - 4] = '\0';
    }

    // Print what the user has selected
    printf("\n<layername>.m16 file for map, used by pvsneslib 'mapLoad' function as 1st argument "
           "(only 1 layer)\n");
    printf(
        "%s.b16 file for tile attributes, used by pvsneslib 'mapLoad' function  as 3rd argument\n",
        filebase);
    printf("%s.o16 file for objects, used by pvsneslib 'objLoadObjects' as argument\n\n", filebase);

    // loop over the map's layers and write them to disk
    if (quietmode == 0)
        printf("tmx2snes: Writing layers map & object files...\n");
    layer = map->layers;

    // write .b16 file first (to have priority flag of each tile for map...
    WriteTileset();

    while (layer)
    {
        if (quietmode == 0)
            printf("tmx2snes: Found layer %s...\n", layer->name.ptr);

        // if it is an entity layer
        if (strcmp(layer->name.ptr, "Entities") == 0)
        {
            // Write .o16 file ...
            WriteEntities();
            if (emitheader)
                WriteEntityHeader();
        }
        // No it is a map layer
        else
        {
            // write .m16 and .t16 files ...
            WriteMap();
            WriteMapTileset();
            if (quadrant)
                WriteQuadrantMap();
            if (cellcollision)
                WriteCellCollision();
        }

        layer = layer->next;
    }

    // free the Tiled map object
    cute_tiled_free_map(map);

    if (quietmode == 0)
        printf("tmx2snes: Done 'File converted'\n");

    return 0;
}
