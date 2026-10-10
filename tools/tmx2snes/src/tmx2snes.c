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

#include "level.h"   /* the conversion itself (shared with opensnes-level) */

#define TMX2SNESVERSION TOOL_VERSION
#define TMX2SNESDATE TOOL_BUILD_DATE

int quietmode = 0;     // 0 = not quiet, 1 = i can't say anything :P
char filebase[1024];   // input filename
char filebasetil[1024]; // input filename for tiles (map filename)

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



static void info_line(const char *line, void *user)
{
    (void)user;
    printf("tmx2snes: %s\n", line);
}

int emitheader = 0;   // -e: write <base>.inc, the entities as C defines
int quadrant = 0;     // -Q: write <layer>.q16, a quadrant-ordered 64x64 map
int cellcollision = 0; // -C: write <layer>.c16, one collision byte per map cell

int main(int argc, char **argv)
{
    int i;

    // init all filenames
    strcpy(filebase, "");
    strcpy(filebasetil, "");

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
    // the output base: the map's path without its extension
    {
        char outbase[1024];
        snprintf(outbase, sizeof outbase, "%s", filebase);
        if (outbase[strlen(outbase) - 5] == '.')
            outbase[strlen(outbase) - 5] = '\0';
        else if (outbase[strlen(outbase) - 4] == '.')
            outbase[strlen(outbase) - 4] = '\0';

        // Print what the user has selected
        printf("\n<layername>.m16 file for map, used by pvsneslib 'mapLoad' function as 1st argument "
               "(only 1 layer)\n");
        printf(
            "%s.b16 file for tile attributes, used by pvsneslib 'mapLoad' function  as 3rd argument\n",
            outbase);
        printf("%s.o16 file for objects, used by pvsneslib 'objLoadObjects' as argument\n\n", outbase);

        level_opts opts = { emitheader, quadrant, cellcollision, 0, "tmx2snes -e", NULL };
        level_result res;
        char err[1200];
        if (level_convert(filebase, filebasetil, outbase, &opts, &res, quietmode ? NULL : info_line, NULL,
                          err, sizeof err) != 0)
        {
            printf("tmx2snes: error '%s'\n", err);
            return 1;
        }
    }
    if (quietmode == 0)
        printf("tmx2snes: Done 'File converted'\n");
    return 0;
}
