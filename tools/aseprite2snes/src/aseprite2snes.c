/*
 * aseprite2snes — Aseprite animation export → OpenSNES anim.h clip tables.
 *
 * Aseprite is where sprite artists actually author animation: they lay frames
 * on a timeline, group them into named tags (walk / idle / hurt), pick a
 * playback direction, and tune each frame's duration in milliseconds. The SNES
 * asset pipeline had no way to carry any of that across — gfx4snes turns a
 * spritesheet into tiles and (with -P) into metasprite geometry, but it has no
 * concept of a "tag" or a per-frame duration. The animation metadata was
 * retyped by hand into DECLARE_ANIM_CLIP() calls, frame by frame.
 *
 * aseprite2snes closes that gap. It reads the JSON Aseprite writes with
 * `--data --list-tags` and emits a C header of ready-to-use AnimClip tables —
 * one clip per tag, frame indices and per-frame tick durations included, the
 * playback direction folded into the frame order, looping vs one-shot derived
 * from the tag. It pairs with gfx4snes -P: gfx4snes owns the pixels and the
 * metasprite table; aseprite2snes owns the timeline that indexes it.
 *
 *   aseprite hero.aseprite --sheet hero.png --data hero.json --list-tags --format json-array
 *   gfx4snes -s 16 ... -P 2 -i hero.png        # tiles + metasprite pointer table
 *   aseprite2snes -o hero_anim.h -p hero hero.json   # the AnimClip tables
 *
 * v1 is the animation layer only: it never touches image data. Frame values
 * default to the frame's index into the metasprite pointer table (gfx4snes
 * emits one entry per sheet cell, in the same order Aseprite lists frames), so
 * clip frame i selects metasprite i. For a single-hardware-sprite animation
 * whose consecutive frames are consecutive OAM tile numbers, -t <stride>
 * multiplies the index into a tile number instead.
 *
 * Consumes: `frames` (json-hash OR json-array; each frame's `duration` in ms)
 * and `meta.frameTags` (name / from / to / direction / optional repeat).
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */
#include "anim.h"
#include "json.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#ifndef VERSION
#define VERSION TOOL_VERSION   /* from tools/tool.mk */
#endif


/*----------------------------------------------------------------------------
 * diagnostics
 *--------------------------------------------------------------------------*/

static void die(const char *fmt, ...)
{
    va_list ap;
    fputs("aseprite2snes: error: ", stderr);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
    exit(1);
}

/*----------------------------------------------------------------------------
 * CLI
 *--------------------------------------------------------------------------*/

static void usage(FILE *o, const char *argv0)
{
    fprintf(o,
        "aseprite2snes v%s — Aseprite animation export -> OpenSNES AnimClip tables\n"
        "\n"
        "Usage: %s [options] <export.json>\n"
        "\n"
        "  -o <file>    write header to <file> (default: stdout)\n"
        "  -p <prefix>  symbol/macro prefix (default: input basename)\n"
        "  -t <stride>  frame value = frame index x stride (default: 1)\n"
        "  -f <fps>     frame rate for ms->tick conversion (default: 60)\n"
        "  -h, --help   this help\n"
        "  -V, --version  print version\n"
        "\n"
        "Input: Aseprite `--data --list-tags` JSON (json-hash or json-array).\n",
        VERSION, argv0);
}

int main(int argc, char **argv)
{
    const char *in = NULL, *out_path = NULL, *prefix = NULL;
    int stride = 1, fps = 60;

    for (int i = 1; i < argc; i++) {
        const char *a = argv[i];
        if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0) {
            usage(stdout, argv[0]);
            return 0;
        } else if (strcmp(a, "-V") == 0 || strcmp(a, "--version") == 0) {
            printf("aseprite2snes %s\n", VERSION);
            return 0;
        } else if (strcmp(a, "-o") == 0) {
            if (++i >= argc) die("-o needs an argument");
            out_path = argv[i];
        } else if (strcmp(a, "-p") == 0) {
            if (++i >= argc) die("-p needs an argument");
            prefix = argv[i];
        } else if (strcmp(a, "-t") == 0) {
            if (++i >= argc) die("-t needs an argument");
            stride = atoi(argv[i]);
            if (stride < 1) die("stride must be >= 1");
        } else if (strcmp(a, "-f") == 0) {
            if (++i >= argc) die("-f needs an argument");
            fps = atoi(argv[i]);
            if (fps < 1) die("fps must be >= 1");
        } else if (a[0] == '-' && a[1] != '\0') {
            die("unknown option '%s' (try --help)", a);
        } else {
            if (in) die("multiple input files given");
            in = a;
        }
    }
    if (!in) {
        usage(stderr, argv[0]);
        return 2;
    }

    FILE *o = stdout;
    if (out_path) {
        o = fopen(out_path, "wb");
        if (!o)
            die("cannot write '%s'", out_path);
    }
    char err[512];
    int rc = aseprite_to_clips(in, prefix, stride, fps, "aseprite2snes v" VERSION, o,
                               err, sizeof err, NULL, NULL);
    if (o != stdout)
        fclose(o);
    if (rc)
        die("%s", err);
    return 0;
}
