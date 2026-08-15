/*
 * json.h — a minimal, ordered JSON DOM parser for aseprite2snes.
 *
 * Small on purpose: it parses exactly the JSON subset an Aseprite CLI export
 * uses (objects, arrays, strings, numbers, true/false/null) and preserves the
 * source order of both object members and array items — which the tool relies
 * on, since Aseprite emits animation frames in playback order.
 *
 * Ownership: json_parse() returns a tree the caller frees with json_free().
 * On error it returns NULL and fills `err` (line/column + message).
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */
#ifndef ASEPRITE2SNES_JSON_H
#define ASEPRITE2SNES_JSON_H

#include <stddef.h>

typedef enum {
    J_NULL,
    J_BOOL,
    J_NUM,
    J_STR,
    J_ARR,
    J_OBJ
} jtype;

typedef struct jnode   jnode;
typedef struct jmember jmember;

struct jmember {
    char    *key;   /* NUL-terminated, owned */
    jnode   *val;   /* owned */
    jmember *next;  /* source order */
};

struct jnode {
    jtype     type;
    double    num;      /* J_NUM; J_BOOL uses 0.0 / 1.0 */
    char     *str;      /* J_STR, owned */
    jnode   **items;    /* J_ARR, owned */
    int       nitems;   /* J_ARR */
    jmember  *members;  /* J_OBJ, owned, source order */
};

typedef struct {
    int  line;
    int  col;
    char msg[160];
} jerror;

/* Parse a NUL-terminated JSON text. Returns the root node (caller frees with
 * json_free) or NULL on error (err filled). */
jnode *json_parse(const char *text, jerror *err);

void json_free(jnode *n);

/* Object member lookup by key (NULL if not an object or key absent). */
jnode *json_get(const jnode *obj, const char *key);

#endif /* ASEPRITE2SNES_JSON_H */
