/*
 * json.c — implementation of the minimal ordered JSON DOM (see json.h).
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */
#include "json.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    const char *p;      /* current position */
    const char *start;  /* text start, for line/col */
    jerror     *err;
    int         failed;
} jparser;

static jnode *parse_value(jparser *js);

/*----------------------------------------------------------------------------
 * error + position helpers
 *--------------------------------------------------------------------------*/

static void set_error(jparser *js, const char *msg)
{
    if (js->failed)
        return;                 /* keep the first error */
    js->failed = 1;
    if (!js->err)
        return;
    int line = 1, col = 1;
    for (const char *c = js->start; c < js->p && *c; c++) {
        if (*c == '\n') { line++; col = 1; }
        else            { col++; }
    }
    js->err->line = line;
    js->err->col  = col;
    strncpy(js->err->msg, msg, sizeof(js->err->msg) - 1);
    js->err->msg[sizeof(js->err->msg) - 1] = '\0';
}

static void skip_ws(jparser *js)
{
    while (*js->p == ' ' || *js->p == '\t' || *js->p == '\n' || *js->p == '\r')
        js->p++;
}

/*----------------------------------------------------------------------------
 * node allocation
 *--------------------------------------------------------------------------*/

static jnode *node_new(jtype t)
{
    jnode *n = calloc(1, sizeof(*n));
    if (n)
        n->type = t;
    return n;
}

void json_free(jnode *n)
{
    if (!n)
        return;
    switch (n->type) {
    case J_STR:
        free(n->str);
        break;
    case J_ARR:
        for (int i = 0; i < n->nitems; i++)
            json_free(n->items[i]);
        free(n->items);
        break;
    case J_OBJ: {
        jmember *m = n->members;
        while (m) {
            jmember *next = m->next;
            free(m->key);
            json_free(m->val);
            free(m);
            m = next;
        }
        break;
    }
    default:
        break;
    }
    free(n);
}

/*----------------------------------------------------------------------------
 * string parsing (handles the standard JSON escapes + \uXXXX → UTF-8 BMP)
 *--------------------------------------------------------------------------*/

static int hex4(const char *s, unsigned *out)
{
    unsigned v = 0;
    for (int i = 0; i < 4; i++) {
        char c = s[i];
        v <<= 4;
        if (c >= '0' && c <= '9')      v |= (unsigned)(c - '0');
        else if (c >= 'a' && c <= 'f') v |= (unsigned)(c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') v |= (unsigned)(c - 'A' + 10);
        else                           return 0;
    }
    *out = v;
    return 1;
}

/* Parse a JSON string (js->p points at the opening quote). Returns a freshly
 * malloc'd NUL-terminated buffer, or NULL on error. */
static char *parse_string_raw(jparser *js)
{
    if (*js->p != '"') {
        set_error(js, "expected string");
        return NULL;
    }
    js->p++;                            /* opening quote */
    /* Worst case the decoded string is no longer than the source span. */
    const char *scan = js->p;
    size_t cap = 0;
    while (*scan && *scan != '"') {
        if (*scan == '\\' && scan[1])
            scan++;
        scan++;
        cap++;
    }
    char *out = malloc(cap * 4 + 1);    /* \uXXXX → up to 3 UTF-8 bytes */
    if (!out) {
        set_error(js, "out of memory");
        return NULL;
    }
    char *w = out;
    while (*js->p && *js->p != '"') {
        char c = *js->p;
        if (c == '\\') {
            char e = js->p[1];
            switch (e) {
            case '"':  *w++ = '"';  js->p += 2; break;
            case '\\': *w++ = '\\'; js->p += 2; break;
            case '/':  *w++ = '/';  js->p += 2; break;
            case 'b':  *w++ = '\b'; js->p += 2; break;
            case 'f':  *w++ = '\f'; js->p += 2; break;
            case 'n':  *w++ = '\n'; js->p += 2; break;
            case 'r':  *w++ = '\r'; js->p += 2; break;
            case 't':  *w++ = '\t'; js->p += 2; break;
            case 'u': {
                unsigned cp;
                if (!hex4(js->p + 2, &cp)) {
                    set_error(js, "bad \\u escape");
                    free(out);
                    return NULL;
                }
                js->p += 6;
                if (cp < 0x80) {
                    *w++ = (char)cp;
                } else if (cp < 0x800) {
                    *w++ = (char)(0xC0 | (cp >> 6));
                    *w++ = (char)(0x80 | (cp & 0x3F));
                } else {
                    *w++ = (char)(0xE0 | (cp >> 12));
                    *w++ = (char)(0x80 | ((cp >> 6) & 0x3F));
                    *w++ = (char)(0x80 | (cp & 0x3F));
                }
                break;
            }
            default:
                set_error(js, "bad escape");
                free(out);
                return NULL;
            }
        } else {
            *w++ = c;
            js->p++;
        }
    }
    if (*js->p != '"') {
        set_error(js, "unterminated string");
        free(out);
        return NULL;
    }
    js->p++;                            /* closing quote */
    *w = '\0';
    return out;
}

/*----------------------------------------------------------------------------
 * value parsing
 *--------------------------------------------------------------------------*/

static jnode *parse_string(jparser *js)
{
    char *s = parse_string_raw(js);
    if (!s)
        return NULL;
    jnode *n = node_new(J_STR);
    if (!n) { free(s); set_error(js, "out of memory"); return NULL; }
    n->str = s;
    return n;
}

static jnode *parse_number(jparser *js)
{
    char *end = NULL;
    double v = strtod(js->p, &end);
    if (end == js->p) {
        set_error(js, "invalid number");
        return NULL;
    }
    js->p = end;
    jnode *n = node_new(J_NUM);
    if (!n) { set_error(js, "out of memory"); return NULL; }
    n->num = v;
    return n;
}

static jnode *parse_literal(jparser *js, const char *word, jtype t, double num)
{
    size_t len = strlen(word);
    if (strncmp(js->p, word, len) != 0) {
        set_error(js, "invalid literal");
        return NULL;
    }
    js->p += len;
    jnode *n = node_new(t);
    if (!n) { set_error(js, "out of memory"); return NULL; }
    n->num = num;
    return n;
}

static jnode *parse_array(jparser *js)
{
    js->p++;                            /* '[' */
    jnode *n = node_new(J_ARR);
    if (!n) { set_error(js, "out of memory"); return NULL; }
    skip_ws(js);
    if (*js->p == ']') { js->p++; return n; }
    for (;;) {
        jnode *v = parse_value(js);
        if (!v) { json_free(n); return NULL; }
        jnode **grown = realloc(n->items, sizeof(*n->items) * (n->nitems + 1));
        if (!grown) { json_free(v); json_free(n); set_error(js, "out of memory"); return NULL; }
        n->items = grown;
        n->items[n->nitems++] = v;
        skip_ws(js);
        if (*js->p == ',') { js->p++; skip_ws(js); continue; }
        if (*js->p == ']') { js->p++; break; }
        set_error(js, "expected ',' or ']' in array");
        json_free(n);
        return NULL;
    }
    return n;
}

static jnode *parse_object(jparser *js)
{
    js->p++;                            /* '{' */
    jnode   *n    = node_new(J_OBJ);
    jmember *tail = NULL;
    if (!n) { set_error(js, "out of memory"); return NULL; }
    skip_ws(js);
    if (*js->p == '}') { js->p++; return n; }
    for (;;) {
        skip_ws(js);
        char *key = parse_string_raw(js);
        if (!key) { json_free(n); return NULL; }
        skip_ws(js);
        if (*js->p != ':') {
            set_error(js, "expected ':' after object key");
            free(key); json_free(n); return NULL;
        }
        js->p++;
        skip_ws(js);
        jnode *v = parse_value(js);
        if (!v) { free(key); json_free(n); return NULL; }
        jmember *m = calloc(1, sizeof(*m));
        if (!m) { free(key); json_free(v); json_free(n); set_error(js, "out of memory"); return NULL; }
        m->key = key;
        m->val = v;
        if (tail) tail->next = m;
        else      n->members = m;
        tail = m;
        skip_ws(js);
        if (*js->p == ',') { js->p++; continue; }
        if (*js->p == '}') { js->p++; break; }
        set_error(js, "expected ',' or '}' in object");
        json_free(n);
        return NULL;
    }
    return n;
}

static jnode *parse_value(jparser *js)
{
    skip_ws(js);
    switch (*js->p) {
    case '"': return parse_string(js);
    case '{': return parse_object(js);
    case '[': return parse_array(js);
    case 't': return parse_literal(js, "true",  J_BOOL, 1.0);
    case 'f': return parse_literal(js, "false", J_BOOL, 0.0);
    case 'n': return parse_literal(js, "null",  J_NULL, 0.0);
    case '-': case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
        return parse_number(js);
    case '\0':
        set_error(js, "unexpected end of input");
        return NULL;
    default:
        set_error(js, "unexpected character");
        return NULL;
    }
}

/*----------------------------------------------------------------------------
 * public API
 *--------------------------------------------------------------------------*/

jnode *json_parse(const char *text, jerror *err)
{
    jparser js = { text, text, err, 0 };
    if (err) { err->line = err->col = 0; err->msg[0] = '\0'; }
    jnode *root = parse_value(&js);
    if (!root)
        return NULL;
    skip_ws(&js);
    if (*js.p != '\0') {
        set_error(&js, "trailing data after JSON value");
        json_free(root);
        return NULL;
    }
    return root;
}

jnode *json_get(const jnode *obj, const char *key)
{
    if (!obj || obj->type != J_OBJ)
        return NULL;
    for (jmember *m = obj->members; m; m = m->next)
        if (strcmp(m->key, key) == 0)
            return m->val;
    return NULL;
}
