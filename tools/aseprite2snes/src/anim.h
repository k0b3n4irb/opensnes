/* anim.h — Aseprite export → AnimClip tables, as a library (see anim.c). */
#ifndef ASEPRITE2SNES_ANIM_H
#define ASEPRITE2SNES_ANIM_H

#include <stddef.h>
#include <stdio.h>
#include "json.h"

typedef void (*anim_warn_fn)(const char *msg, void *user);

/* Write the header for json_path to out. prefix NULL = the file's stem;
 * generator is the first comment line's author ("aseprite2snes v1.0.0").
 * Returns 0, or 1 with the reason in err[] (nothing partial is written
 * before the first fprintf, which happens after every check). */
int aseprite_to_clips(const char *json_path, const char *prefix, int stride, int fps,
                      const char *generator, FILE *out, char *err, size_t errlen,
                      anim_warn_fn warn_fn, void *user);

/* "res/my-hero.json" → "my_hero" (a C identifier fragment). */
void anim_prefix_from_path(const char *path, char *out, size_t cap);

#endif
