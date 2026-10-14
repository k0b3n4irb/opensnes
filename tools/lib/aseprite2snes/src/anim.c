/*
 * anim.c — Aseprite's `--data --list-tags` export → OpenSNES AnimClip tables.
 * The converter of aseprite2snes as a library (2026-10-05), so that
 * opensnes-sprite runs the same code and writes the same header: one clip
 * per tag, frame indices and per-frame tick durations, the playback
 * direction folded into the frame order, looping vs one-shot from the tag.
 * Errors come back as a return code and a message (no exit); warnings go
 * to the reporter the caller installs (aseprite2snes's own prints
 * "aseprite2snes: warning:" on stderr).
 */
#include "anim.h"

#include <ctype.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FRAMES 512          /* Aseprite sheets rarely exceed a few dozen */
#define TICK_MAX   255          /* AnimClip durations/speed are u8 */

static jmp_buf  g_jump;
static char    *g_err;
static size_t   g_errlen;
static anim_warn_fn g_warn;
static void    *g_user;

static void die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(g_err, g_errlen, fmt, ap);
    va_end(ap);
    longjmp(g_jump, 1);
}

static void warn(const char *fmt, ...)
{
    char msg[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    if (g_warn) g_warn(msg, g_user);
    else fprintf(stderr, "aseprite2snes: warning: %s\n", msg);
}

/*----------------------------------------------------------------------------
 * identifier sanitising
 *--------------------------------------------------------------------------*/

/* Turn an arbitrary label into a C identifier fragment: [A-Za-z0-9_] only,
 * a leading digit gets an underscore, empty becomes "anim". `upper` picks
 * the case (macros/enums vs symbols). */
static void sanitize(const char *in, char *out, size_t cap, int upper)
{
    size_t j = 0;
    if (cap == 0)
        return;
    if (in && isdigit((unsigned char)in[0]) && j + 1 < cap)
        out[j++] = '_';
    for (const char *c = in ? in : ""; *c && j + 1 < cap; c++) {
        unsigned char ch = (unsigned char)*c;
        char o = (isalnum(ch)) ? (char)ch : '_';
        out[j++] = upper ? (char)toupper((unsigned char)o)
                         : (char)tolower((unsigned char)o);
    }
    if (j == 0) {
        const char *d = upper ? "ANIM" : "anim";
        while (*d && j + 1 < cap)
            out[j++] = *d++;
    }
    out[j] = '\0';
}

/* Derive a default prefix from an input path: basename without extension. */
void anim_prefix_from_path(const char *path, char *out, size_t cap)
{
    const char *base = path;
    for (const char *c = path; *c; c++)
        if (*c == '/' || *c == '\\')
            base = c + 1;
    char stem[256];
    size_t j = 0;
    for (const char *c = base; *c && *c != '.' && j + 1 < sizeof(stem); c++)
        stem[j++] = *c;
    stem[j] = '\0';
    sanitize(stem, out, cap, 0);
}

/*----------------------------------------------------------------------------
 * file slurp
 *--------------------------------------------------------------------------*/

static char *read_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        die("cannot open '%s'", path);
    if (fseek(f, 0, SEEK_END) != 0)
        die("cannot seek '%s'", path);
    long n = ftell(f);
    if (n < 0)
        die("cannot size '%s'", path);
    rewind(f);
    char *buf = malloc((size_t)n + 1);
    if (!buf)
        die("out of memory reading '%s'", path);
    if (fread(buf, 1, (size_t)n, f) != (size_t)n)
        die("short read on '%s'", path);
    buf[n] = '\0';
    fclose(f);
    return buf;
}

/*----------------------------------------------------------------------------
 * model
 *--------------------------------------------------------------------------*/

typedef struct {
    char name[64];
    int  from;
    int  to;
    char direction[16];
    int  has_repeat;
    long repeat;                /* Aseprite 1.3 repeat count; 0 = infinite */
} Tag;

static int    g_nframes;
static double g_durations_ms[MAX_FRAMES];

/* ms → ticks at the target frame rate, clamped to [1, TICK_MAX]. */
static int ms_to_ticks(double ms, int fps, const char *tag)
{
    double t = ms * (double)fps / 1000.0;
    long ticks = (long)(t + 0.5);
    if (ticks < 1)
        ticks = 1;
    if (ticks > TICK_MAX) {
        warn("tag '%s': frame duration %.0fms = %ld ticks exceeds %d, clamped",
             tag, ms, ticks, TICK_MAX);
        ticks = TICK_MAX;
    }
    return (int)ticks;
}

/*----------------------------------------------------------------------------
 * frame + tag extraction
 *--------------------------------------------------------------------------*/

static void collect_frames(const jnode *root)
{
    jnode *frames = json_get(root, "frames");
    if (!frames)
        die("no \"frames\" in export (run Aseprite with --data)");

    if (frames->type == J_ARR) {
        g_nframes = frames->nitems;
        if (g_nframes > MAX_FRAMES)
            die("%d frames exceeds MAX_FRAMES (%d)", g_nframes, MAX_FRAMES);
        for (int i = 0; i < g_nframes; i++) {
            jnode *d = json_get(frames->items[i], "duration");
            g_durations_ms[i] = (d && d->type == J_NUM) ? d->num : 100.0;
        }
    } else if (frames->type == J_OBJ) {
        int i = 0;
        for (jmember *m = frames->members; m; m = m->next) {
            if (i >= MAX_FRAMES)
                die("more than MAX_FRAMES (%d) frames", MAX_FRAMES);
            jnode *d = json_get(m->val, "duration");
            g_durations_ms[i++] = (d && d->type == J_NUM) ? d->num : 100.0;
        }
        g_nframes = i;
    } else {
        die("\"frames\" is neither an array nor an object");
    }
    if (g_nframes == 0)
        die("export has zero frames");
}

static long num_field(const jnode *obj, const char *key, long dflt)
{
    jnode *n = json_get(obj, key);
    return (n && n->type == J_NUM) ? (long)n->num : dflt;
}

static int collect_tags(const jnode *root, Tag *tags, int max)
{
    jnode *meta = json_get(root, "meta");
    jnode *ft   = meta ? json_get(meta, "frameTags") : NULL;

    if (!ft || ft->type != J_ARR || ft->nitems == 0) {
        /* No tags exported: synthesise one clip covering every frame. */
        warn("no frameTags found — emitting a single clip over all %d frames "
             "(export with --list-tags to get named clips)", g_nframes);
        snprintf(tags[0].name, sizeof(tags[0].name), "all");
        tags[0].from = 0;
        tags[0].to   = g_nframes - 1;
        snprintf(tags[0].direction, sizeof(tags[0].direction), "forward");
        tags[0].has_repeat = 0;
        tags[0].repeat = 0;
        return 1;
    }

    int n = 0;
    for (int i = 0; i < ft->nitems && n < max; i++) {
        jnode *t = ft->items[i];
        jnode *nm = json_get(t, "name");
        jnode *dir = json_get(t, "direction");
        jnode *rp = json_get(t, "repeat");
        snprintf(tags[n].name, sizeof(tags[n].name), "%s",
                 (nm && nm->type == J_STR) ? nm->str : "tag");
        tags[n].from = (int)num_field(t, "from", 0);
        tags[n].to   = (int)num_field(t, "to", g_nframes - 1);
        snprintf(tags[n].direction, sizeof(tags[n].direction), "%s",
                 (dir && dir->type == J_STR) ? dir->str : "forward");
        if (rp && rp->type == J_STR) {
            tags[n].has_repeat = 1;
            tags[n].repeat = strtol(rp->str, NULL, 10);
        } else if (rp && rp->type == J_NUM) {
            tags[n].has_repeat = 1;
            tags[n].repeat = (long)rp->num;
        } else {
            tags[n].has_repeat = 0;
            tags[n].repeat = 0;
        }
        if (tags[n].from < 0 || tags[n].to >= g_nframes ||
            tags[n].from > tags[n].to)
            die("tag '%s': frame range %d..%d out of bounds (0..%d)",
                tags[n].name, tags[n].from, tags[n].to, g_nframes - 1);
        n++;
    }
    return n;
}

/*----------------------------------------------------------------------------
 * per-tag frame ordering (direction) + mode
 *--------------------------------------------------------------------------*/

/* Fill `order` with the absolute frame indices in playback order for a tag's
 * direction. Returns the count, or -1 for an unknown direction. */
static int build_order(const Tag *t, int *order, int cap)
{
    int n = 0;
    int lo = t->from, hi = t->to;
    if (strcmp(t->direction, "forward") == 0) {
        for (int i = lo; i <= hi && n < cap; i++)
            order[n++] = i;
    } else if (strcmp(t->direction, "reverse") == 0) {
        for (int i = hi; i >= lo && n < cap; i--)
            order[n++] = i;
    } else if (strcmp(t->direction, "pingpong") == 0) {
        for (int i = lo; i <= hi && n < cap; i++)
            order[n++] = i;
        for (int i = hi - 1; i > lo && n < cap; i--)   /* skip both endpoints */
            order[n++] = i;
    } else if (strcmp(t->direction, "pingpong_reverse") == 0) {
        for (int i = hi; i >= lo && n < cap; i--)
            order[n++] = i;
        for (int i = lo + 1; i < hi && n < cap; i++)
            order[n++] = i;
    } else {
        return -1;
    }
    return n;
}

/* ANIM_ONCE only when the tag asks to play exactly once; everything else
 * (infinite repeat, absent repeat, repeat>1) loops. */
static const char *mode_of(const Tag *t)
{
    if (t->has_repeat && t->repeat == 1)
        return "ANIM_ONCE";
    if (t->has_repeat && t->repeat > 1)
        warn("tag '%s': repeat=%ld (finite >1) is not representable by "
             "AnimClip; emitting ANIM_LOOP", t->name, t->repeat);
    return "ANIM_LOOP";
}

/*----------------------------------------------------------------------------
 * emission
 *--------------------------------------------------------------------------*/

static void emit_header(FILE *o, const char *src, const char *prefix, const char *generator,
                        int stride, int fps, Tag *tags, int ntags)
{
    char sp[64], mp[64];
    sanitize(prefix, sp, sizeof(sp), 0);
    sanitize(prefix, mp, sizeof(mp), 1);

    fprintf(o,
        "/* Generated by %s — do not edit by hand.\n"
        " * Source export: %s\n"
        " *\n"
        " * One AnimClip per Aseprite tag, for the OpenSNES anim.h player.\n"
        " * Frame values are metasprite-table indices (frame index x stride=%d);\n"
        " * pair with a gfx4snes -P pointer table, or index OAM tiles directly.\n"
        " * Durations are Aseprite per-frame milliseconds converted at %d fps.\n"
        " * Include this AFTER <snes.h> (needs AnimClip, ANIM_LOOP, ANIM_ONCE).\n"
        " */\n"
        "#ifndef %s_ANIM_H\n"
        "#define %s_ANIM_H\n\n",
        generator, src, stride, fps, mp, mp);

    int order[MAX_FRAMES];

    for (int ti = 0; ti < ntags; ti++) {
        Tag *t = &tags[ti];
        char tg[64], TG[64];
        sanitize(t->name, tg, sizeof(tg), 0);
        sanitize(t->name, TG, sizeof(TG), 1);

        int n = build_order(t, order, MAX_FRAMES);
        if (n < 0) {
            warn("tag '%s': unknown direction '%s', treating as forward",
                 t->name, t->direction);
            snprintf(t->direction, sizeof(t->direction), "forward");
            n = build_order(t, order, MAX_FRAMES);
        }

        /* durations in ticks, and whether they are all equal (uniform path) */
        int ticks[MAX_FRAMES];
        int uniform = 1;
        ticks[0] = 0;   /* a tag with no frames must not read it uninitialised */
        for (int i = 0; i < n; i++) {
            ticks[i] = ms_to_ticks(g_durations_ms[order[i]], fps, t->name);
            if (i && ticks[i] != ticks[0])
                uniform = 0;
        }

        fprintf(o, "/* tag \"%s\" — %s, frames %d..%d */\n",
                t->name, t->direction, t->from, t->to);

        fprintf(o, "static const u16 %s_%s_frames[] = {", sp, tg);
        for (int i = 0; i < n; i++)
            fprintf(o, "%s%d", i ? ", " : " ", order[i] * stride);
        fprintf(o, " };\n");

        if (!uniform) {
            fprintf(o, "static const u8  %s_%s_durations[] = {", sp, tg);
            for (int i = 0; i < n; i++)
                fprintf(o, "%s%d", i ? ", " : " ", ticks[i]);
            fprintf(o, " };\n");
            fprintf(o,
                "static const AnimClip %s_%s = { %s_%s_frames, %s_%s_durations, "
                "%d, 0, %s, 0 };\n\n",
                sp, tg, sp, tg, sp, tg, n, mode_of(t));
        } else {
            fprintf(o,
                "static const AnimClip %s_%s = { %s_%s_frames, 0, "
                "%d, %d, %s, 0 };\n\n",
                sp, tg, sp, tg, n, ticks[0], mode_of(t));
        }
    }

    /* index enum + a pointer table over every clip, for state-driven play */
    fprintf(o, "enum {\n");
    for (int ti = 0; ti < ntags; ti++) {
        char TG[64];
        sanitize(tags[ti].name, TG, sizeof(TG), 1);
        fprintf(o, "    %s_ANIM_%s = %d,\n", mp, TG, ti);
    }
    fprintf(o, "    %s_ANIM_COUNT = %d\n};\n\n", mp, ntags);

    fprintf(o, "static const AnimClip *const %s_anims[%s_ANIM_COUNT] = {\n", sp, mp);
    for (int ti = 0; ti < ntags; ti++) {
        char tg[64];
        sanitize(tags[ti].name, tg, sizeof(tg), 0);
        fprintf(o, "    &%s_%s,\n", sp, tg);
    }
    fprintf(o, "};\n\n");

    fprintf(o, "#endif /* %s_ANIM_H */\n", mp);
}


static char  *g_text;
static jnode *g_root;

int aseprite_to_clips(const char *json_path, const char *prefix, int stride, int fps,
                      const char *generator, FILE *out, char *err, size_t errlen,
                      anim_warn_fn warn_fn, void *user)
{
    g_err = err; g_errlen = errlen; g_warn = warn_fn; g_user = user;
    if (errlen) err[0] = '\0';
    char pbuf[64];
    if (!prefix) anim_prefix_from_path(json_path, pbuf, sizeof pbuf);
    const char *volatile pfx = prefix ? prefix : pbuf;   /* lives across setjmp, never written after it */
    g_text = NULL;
    g_root = NULL;
    if (setjmp(g_jump)) {
        if (g_root) json_free(g_root);
        free(g_text);
        g_root = NULL; g_text = NULL;
        return 1;
    }
    g_text = read_file(json_path);
    jerror jerr;
    g_root = json_parse(g_text, &jerr);
    if (!g_root)
        die("%s:%d:%d: %s", json_path, jerr.line, jerr.col, jerr.msg);
    if (g_root->type != J_OBJ)
        die("top-level JSON is not an object");
    collect_frames(g_root);
    Tag tags[128];
    int ntags = collect_tags(g_root, tags, 128);
    emit_header(out, json_path, pfx, generator, stride, fps, tags, ntags);
    json_free(g_root);
    free(g_text);
    g_root = NULL; g_text = NULL;
    return 0;
}
