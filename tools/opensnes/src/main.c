/*
 * opensnes — the project tool of the OpenSNES SDK.
 *
 *   opensnes init <name> [--template blank|game]
 *   opensnes build [--clean]
 *   opensnes clean
 *   opensnes run [--emulator NAME]
 *   opensnes test [--update]
 *   opensnes doctor [--json]
 *   opensnes upgrade [-q] [--removed-only] <folder-or-file>...
 *
 * A compiled program since 2026-10-06 (it was a shell script,
 * scripts/opensnes): the game developer's zip holds binaries only
 * (.claude/rules/two_audiences.md). It drives `make` in the project — the
 * build itself is make/common.mk — and finds the SDK from $OPENSNES_HOME,
 * then from its own place (<sdk>/bin/opensnes), then by walking up from
 * the current directory.
 */
#include <ctype.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <io.h>
#include <process.h>
#include <windows.h>
#define os_mkdir(p) _mkdir(p)
#define os_getcwd _getcwd
#define os_popen _popen
#define os_pclose _pclose
#define PATH_LIST_SEP ';'
#define DEVNULL "NUL"
#define EXE ".exe"
#else
#include <dirent.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#define os_mkdir(p) mkdir(p, 0777)
#define os_getcwd getcwd
#define os_popen popen
#define os_pclose pclose
#define PATH_LIST_SEP ':'
#define DEVNULL "/dev/null"
#define EXE ""
#endif

#include "cli.h"
#include "templates.h"

#define PATHN 2048

static const char *g_argv0;

/* ================================================================= paths */

static void slashes(char *p) { for (; *p; p++) if (*p == '\\') *p = '/'; }

static int is_file(const char *p) { struct stat st; return stat(p, &st) == 0 && (st.st_mode & S_IFMT) == S_IFREG; }
static int is_dir(const char *p)  { struct stat st; return stat(p, &st) == 0 && (st.st_mode & S_IFMT) == S_IFDIR; }

/* An executable at `path` (with .exe on Windows); the path found goes to out. */
static int is_exe(const char *path, char *out, size_t n)
{
    const char *suffix[2] = { "", EXE };
    for (int i = 0; i < 2; i++) {
        char p[PATHN];
        snprintf(p, sizeof p, "%s%s", path, suffix[i]);
#ifdef _WIN32
        if (is_file(p)) { if (out) snprintf(out, n, "%s", p); return 1; }
#else
        if (is_file(p) && access(p, X_OK) == 0) { if (out) snprintf(out, n, "%s", p); return 1; }
#endif
        if (!*EXE) break;
    }
    return 0;
}

/* `name` on the PATH (what `command -v` answers). */
static int which(const char *name, char *out, size_t n)
{
    if (strchr(name, '/') || strchr(name, '\\')) return is_exe(name, out, n);
    const char *path = getenv("PATH");
    if (!path) return 0;
    for (const char *p = path; *p;) {
        const char *e = strchr(p, PATH_LIST_SEP);
        size_t len = e ? (size_t)(e - p) : strlen(p);
        if (len) {
            char cand[PATHN];
            snprintf(cand, sizeof cand, "%.*s/%s", (int)len, p, name);
            if (is_exe(cand, out, n)) { if (out) slashes(out); return 1; }
        }
        p += len + (e ? 1 : 0);
    }
    return 0;
}

static int abs_path(const char *p, char *out, size_t n)
{
#ifdef _WIN32
    if (!_fullpath(out, p, n)) return 0;
#else
    char buf[PATH_MAX > PATHN ? PATH_MAX : PATHN];
    if (!realpath(p, buf)) return 0;
    snprintf(out, n, "%s", buf);
#endif
    slashes(out);
    return 1;
}

static void parent_of(char *p)
{
    char *s = strrchr(p, '/');
    if (s && s != p && !(s == p + 2 && p[1] == ':')) *s = '\0';
    else if (s) s[1] = '\0';
    else *p = '\0';
}

/* A path as the project's make reads it. Under MSYS2 (MSYSTEM set) make is
 * the MSYS one, to which "C:/x" in a rule is a target named C with a
 * dependency: it wants "/c/x". Elsewhere the path is already right. */
static void make_path(const char *in, char *out, size_t n)
{
#ifdef _WIN32
    if (getenv("MSYSTEM") && isalpha((unsigned char)in[0]) && in[1] == ':' && (in[2] == '/' || in[2] == '\\')) {
        snprintf(out, n, "/%c%s", tolower((unsigned char)in[0]), in + 2);
        slashes(out);
        return;
    }
#endif
    snprintf(out, n, "%s", in);
}

static const char *leaf_of(const char *p)
{
    const char *b = p;
    for (const char *q = p; *q; q++) if ((*q == '/' || *q == '\\') && q[1]) b = q + 1;
    return b;
}

/* Where this program is, for <sdk>/bin/opensnes. */
static int self_path(char *out, size_t n)
{
#if defined(_WIN32)
    DWORD r = GetModuleFileNameA(NULL, out, (DWORD)n);
    if (r == 0 || r >= n) return 0;
    slashes(out);
    return 1;
#elif defined(__APPLE__)
    char buf[PATHN];
    uint32_t sz = sizeof buf;
    if (_NSGetExecutablePath(buf, &sz) != 0) return 0;
    return abs_path(buf, out, n);
#else
    if (abs_path("/proc/self/exe", out, n)) return 1;
    char found[PATHN];
    return g_argv0 && which(g_argv0, found, sizeof found) && abs_path(found, out, n);
#endif
}

static int has_common_mk(const char *dir)
{
    char p[PATHN];
    snprintf(p, sizeof p, "%s/make/common.mk", dir);
    return is_file(p);
}

/* Walk up from the current directory until `match(dir)`. */
static int walk_up(int (*match)(const char *), char *out, size_t n)
{
    char d[PATHN];
    if (!os_getcwd(d, sizeof d)) return 0;
    slashes(d);
    for (;;) {
        if (match(d)) { snprintf(out, n, "%s", d); return 1; }
        char before[PATHN];
        snprintf(before, sizeof before, "%s", d);
        parent_of(d);
        if (!*d || strcmp(before, d) == 0) return 0;
    }
}

static int find_sdk(char *out, size_t n)
{
    const char *home = getenv("OPENSNES_HOME");
    if (home && *home && has_common_mk(home) && abs_path(home, out, n)) return 1;
    char self[PATHN];
    if (self_path(self, sizeof self)) {
        parent_of(self);      /* <sdk>/bin */
        parent_of(self);      /* <sdk> */
        if (has_common_mk(self)) { snprintf(out, n, "%s", self); return 1; }
    }
    return walk_up(has_common_mk, out, n);
}

static int require_sdk(cli_ctx *ctx, char *out, size_t n)
{
    if (find_sdk(out, n)) return 1;
    cli_error(ctx, NULL, "cannot find the OpenSNES SDK — set OPENSNES_HOME, or run the opensnes of the SDK's bin/");
    return 0;
}

/* A project: a Makefile that includes common.mk. */
static int is_project(const char *dir)
{
    char p[PATHN], line[1024];
    snprintf(p, sizeof p, "%s/Makefile", dir);
    FILE *f = fopen(p, "r");
    if (!f) return 0;
    int found = 0;
    while (!found && fgets(line, sizeof line, f)) {
        const char *s = line;
        while (*s == ' ' || *s == '-') s++;      /* `include` or `-include` */
        found = strncmp(s, "include", 7) == 0 && strstr(s, "make/common.mk") != NULL;
    }
    fclose(f);
    return found;
}

static int require_project(cli_ctx *ctx, char *out, size_t n)
{
    if (walk_up(is_project, out, n)) return 1;
    cli_error(ctx, NULL, "no OpenSNES project here (a Makefile that includes make/common.mk) — `opensnes init <name>` makes one");
    return 0;
}

/* The first line a command prints (a version). */
static void first_line(const char *cmd, char *out, size_t n)
{
    out[0] = '\0';
    char full[PATHN + 64];
    snprintf(full, sizeof full, "%s 2>" DEVNULL, cmd);
    FILE *p = os_popen(full, "r");
    if (!p) return;
    if (fgets(out, (int)n, p)) out[strcspn(out, "\r\n")] = '\0'; else out[0] = '\0';
    os_pclose(p);
}

static void set_home(const char *sdk)
{
#ifdef _WIN32
    _putenv_s("OPENSNES_HOME", sdk);
#else
    setenv("OPENSNES_HOME", sdk, 1);
#endif
}

/* make -C <proj> [target]; the exit code of make, as one of ours. */
static int run_make(cli_ctx *ctx, const char *sdk, const char *proj, const char *target)
{
    char cmd[PATHN * 2], dir[PATHN];
    set_home(sdk);
    make_path(proj, dir, sizeof dir);
    snprintf(cmd, sizeof cmd, "make --no-print-directory -C \"%s\"%s%s", dir, target ? " " : "", target ? target : "");
    fflush(stdout);
    int rc = system(cmd);
    if (rc == -1) { cli_error(ctx, NULL, "cannot run make — is it installed and on the PATH?"); return CLI_IO; }
    return rc == 0 ? CLI_OK : CLI_REFUSED;
}

/* ================================================================== init */

static int write_text(cli_ctx *ctx, const char *path, const char *text)
{
    return cli_write_file(ctx, path, text, strlen(text));
}

/* text with every @LEAF@ replaced. */
static int write_template(cli_ctx *ctx, const char *path, const char *tpl, const char *leaf)
{
    size_t cap = strlen(tpl) + 8 * (strlen(leaf) + 1) + 1, j = 0;
    char *out = malloc(cap);
    if (!out) return CLI_IO;
    for (const char *p = tpl; *p;) {
        if (strncmp(p, "@LEAF@", 6) == 0 && j + strlen(leaf) < cap) { j += (size_t)sprintf(out + j, "%s", leaf); p += 6; }
        else if (j + 1 < cap) out[j++] = *p++;
        else break;
    }
    out[j] = '\0';
    int rc = cli_write_file(ctx, path, out, j);
    free(out);
    return rc;
}

/* mkdir -p */
static int make_dirs(const char *path)
{
    char p[PATHN];
    snprintf(p, sizeof p, "%s", path);
    slashes(p);
    for (char *s = p + 1; *s; s++)
        if (*s == '/') { *s = '\0'; if (*p && !is_dir(p)) os_mkdir(p); *s = '/'; }
    if (!is_dir(p)) os_mkdir(p);
    return is_dir(p);
}

static const cli_opt init_opts[] = {
    { "template", 0, CLI_STR, "NAME", "blank (a text screen) or game (a sprite you steer, with two luna tests)", "blank" },
};

static int run_init(cli_ctx *ctx)
{
    if (ctx->nargs != 1) { cli_error(ctx, NULL, "init takes one name: `opensnes init my-game --template game`"); return CLI_USAGE; }
    const char *name = ctx->args[0], *tpl = cli_str(ctx, "template", "blank");
    const char *modules;
    if (strcmp(tpl, "blank") == 0) modules = "console dma text background";
    else if (strcmp(tpl, "game") == 0) modules = "console sprite dma input background";
    else { cli_error(ctx, NULL, "unknown template '%s' — blank or game", tpl); return CLI_REFUSED; }

    char sdk[PATHN];
    if (!require_sdk(ctx, sdk, sizeof sdk)) return CLI_REFUSED;
    struct stat st;
    if (stat(name, &st) == 0) { cli_error(ctx, name, "already exists — init makes a new folder"); return CLI_REFUSED; }

    char p[PATHN], leaf[256], rom_name[32];
    snprintf(leaf, sizeof leaf, "%s", leaf_of(name));
    size_t ll = strlen(leaf);
    while (ll && (leaf[ll - 1] == '/' || leaf[ll - 1] == '\\')) leaf[--ll] = '\0';
    /* the leaf names the ROM and a make target: make cannot hold a space */
    for (const char *c = leaf; *c; c++)
        if (!isalnum((unsigned char)*c) && *c != '_' && *c != '-' && *c != '.') {
            cli_error(ctx, name, "'%c' in the project's name — letters, digits, - _ . only (it names the ROM, and make cannot hold a space)", *c);
            return CLI_REFUSED;
        }
    if (!ll) { cli_error(ctx, name, "no name"); return CLI_REFUSED; }
    snprintf(p, sizeof p, "%s/res", name);
    if (!make_dirs(p)) { cli_error(ctx, name, "cannot create the folder"); return CLI_IO; }

    int rc;
    snprintf(p, sizeof p, "%s/main.c", name);
    if (strcmp(tpl, "blank") == 0) rc = write_text(ctx, p, TEMPLATE_BLANK_MAIN);
    else {
        rc = write_text(ctx, p, TEMPLATE_GAME_MAIN);
        snprintf(p, sizeof p, "%s/test", name);
        if (rc == CLI_OK && !make_dirs(p)) rc = CLI_IO;
        snprintf(p, sizeof p, "%s/test/boot.toml", name);
        if (rc == CLI_OK) rc = write_template(ctx, p, TEMPLATE_GAME_BOOT, leaf);
        snprintf(p, sizeof p, "%s/test/walk_right.toml", name);
        if (rc == CLI_OK) rc = write_template(ctx, p, TEMPLATE_GAME_WALK, leaf);
    }
    if (rc != CLI_OK) return rc;

    /* the header's title: 21 characters, upper case, _ and - as spaces */
    size_t k;
    for (k = 0; k < 21; k++) {
        char c = k < ll ? leaf[k] : ' ';
        rom_name[k] = (c == '_' || c == '-') ? ' ' : (char)toupper((unsigned char)c);
    }
    rom_name[21] = '\0';
    char mk[PATHN * 2], sdk_make[PATHN];
    make_path(sdk, sdk_make, sizeof sdk_make);
    snprintf(mk, sizeof mk,
        "# OpenSNES project: %s\n# Generated by: opensnes init\n\nOPENSNES := %s\n\nTARGET   := %s.sfc\nROM_NAME := %s\n\n"
        "CSRC     := main.c\n\nUSE_LIB     := 1\nLIB_MODULES := %s\n\ninclude $(OPENSNES)/make/common.mk\n",
        leaf, sdk_make, leaf, rom_name, modules);
    snprintf(p, sizeof p, "%s/Makefile", name);
    if ((rc = write_text(ctx, p, mk)) != CLI_OK) return rc;

    if (ctx->json) {
        cli_json_begin(ctx); cli_json_str(ctx, "project", name); cli_json_str(ctx, "template", tpl);
        cli_json_str(ctx, "sdk", sdk); cli_json_str(ctx, "rom", leaf); cli_json_end(ctx);
    } else if (!ctx->quiet) {
        printf("Created project: %s/\n  Template:  %s\n  SDK:       %s\n\nNext steps:\n  cd %s\n  opensnes build\n  opensnes run\n", name, tpl, sdk, name);
    }
    return CLI_OK;
}

/* ================================================ build, clean, test, run */

static const cli_opt build_opts[] = {
    { "clean", 0, CLI_FLAG, NULL, "clean first: a full rebuild", NULL },
};

static int run_build(cli_ctx *ctx)
{
    char sdk[PATHN], proj[PATHN];
    if (!require_sdk(ctx, sdk, sizeof sdk) || !require_project(ctx, proj, sizeof proj)) return CLI_REFUSED;
    int rc = CLI_OK;
    if (cli_has(ctx, "clean")) rc = run_make(ctx, sdk, proj, "clean");
    return rc == CLI_OK ? run_make(ctx, sdk, proj, NULL) : rc;
}

static int run_clean(cli_ctx *ctx)
{
    char sdk[PATHN], proj[PATHN];
    if (!require_sdk(ctx, sdk, sizeof sdk) || !require_project(ctx, proj, sizeof proj)) return CLI_REFUSED;
    return run_make(ctx, sdk, proj, "clean");
}

static const cli_opt test_opts[] = {
    { "update", 0, CLI_FLAG, NULL, "rewrite the visual baselines (asserts.fbhash) of test/*.toml instead of judging", NULL },
};

static int run_test(cli_ctx *ctx)
{
    char sdk[PATHN], proj[PATHN];
    if (!require_sdk(ctx, sdk, sizeof sdk) || !require_project(ctx, proj, sizeof proj)) return CLI_REFUSED;
    return run_make(ctx, sdk, proj, cli_has(ctx, "update") ? "test-update" : "test");
}

static const char *const EMULATORS[] = { "luna-gui", "mesen", "Mesen", "Mesen2", "bsnes", "bsnes-hd", "snes9x", "snes9x-gtk" };

/* The emulator `run` opens: the one asked for, the luna GUI the installer
 * put in the SDK, then the first known one on the PATH. */
static int find_emulator(const char *sdk, const char *preferred, char *out, size_t n)
{
    if (preferred) return which(preferred, out, n);
    char p[PATHN];
    snprintf(p, sizeof p, "%s/testing/bin/luna-gui", sdk);
    if (is_exe(p, out, n)) return 1;
    for (size_t i = 0; i < sizeof EMULATORS / sizeof EMULATORS[0]; i++)
        if (which(EMULATORS[i], out, n)) return 1;
    return 0;
}

static int find_luna(const char *sdk, char *out, size_t n)
{
    const char *env = getenv("LUNA_BIN");
    if (env && *env && is_exe(env, out, n)) return 1;
    char p[PATHN];
    snprintf(p, sizeof p, "%s/testing/bin/luna", sdk);
    if (is_exe(p, out, n)) return 1;
    return which("luna", out, n);
}

/* The ROM the project builds: TARGET of its Makefile. */
static int project_rom(const char *proj, char *out, size_t n)
{
    char p[PATHN], line[1024];
    snprintf(p, sizeof p, "%s/Makefile", proj);
    FILE *f = fopen(p, "r");
    if (!f) return 0;
    int found = 0;
    while (!found && fgets(line, sizeof line, f)) {
        char *s = line;
        while (*s == ' ') s++;
        if (strncmp(s, "TARGET", 6) != 0) continue;
        s += 6;
        while (*s == ' ' || *s == '\t') s++;
        if (*s == ':' || *s == '?') s++;
        if (*s != '=') continue;
        s++;
        while (*s == ' ' || *s == '\t') s++;
        s[strcspn(s, " \t\r\n#")] = '\0';
        if (*s) { snprintf(out, n, "%s/%s", proj, s); found = 1; }
    }
    fclose(f);
    return found && is_file(out);
}

static const cli_opt run_opts[] = {
    { "emulator", 0, CLI_STR, "NAME", "the emulator to open the ROM with (a name on the PATH, or a path)", "luna-gui, then mesen, bsnes, snes9x" },
};

static int run_run(cli_ctx *ctx)
{
    char sdk[PATHN], proj[PATHN], rom[PATHN], emu[PATHN];
    if (!require_sdk(ctx, sdk, sizeof sdk) || !require_project(ctx, proj, sizeof proj)) return CLI_REFUSED;
    int rc = run_make(ctx, sdk, proj, NULL);
    if (rc != CLI_OK) return rc;
    if (!project_rom(proj, rom, sizeof rom)) { cli_error(ctx, proj, "no ROM after the build — TARGET of the Makefile names a .sfc that is not there"); return CLI_REFUSED; }
    const char *asked = cli_has(ctx, "emulator") ? cli_str(ctx, "emulator", NULL) : NULL;
    if (!find_emulator(sdk, asked, emu, sizeof emu)) {
        if (asked) { cli_error(ctx, NULL, "emulator not found: %s", asked); return CLI_REFUSED; }
        printf("Built: %s\nNo emulator found. Run scripts/install-luna.sh (luna-gui), or install Mesen, bsnes or snes9x.\nOpen manually: %s\n", leaf_of(rom), rom);
        return CLI_OK;
    }
    printf("Launching: %s %s\n", leaf_of(emu), leaf_of(rom));
    fflush(stdout);
#ifdef _WIN32
    if (_spawnl(_P_NOWAIT, emu, emu, rom, NULL) == -1) { cli_error(ctx, emu, "cannot start"); return CLI_IO; }
#else
    pid_t pid = fork();
    if (pid < 0) { cli_error(ctx, emu, "cannot start"); return CLI_IO; }
    if (pid == 0) {
        if (!freopen(DEVNULL, "w", stdout) || !freopen(DEVNULL, "w", stderr)) _exit(127);
        setsid();
        execl(emu, emu, rom, (char *)NULL);
        _exit(127);
    }
#endif
    return CLI_OK;
}

/* ================================================================ doctor */

/* What a project's build may call (make/common.mk), by need. */
static const char *const REQUIRED_BINS[] = { "cc65816", "cproc-qbe", "qbe", "wla-65816", "wlalink", "opensnes-rom" };
static const char *const ASSET_BINS[] = {
    "opensnes-sprite", "opensnes-tileset", "opensnes-level", "opensnes-text", "opensnes-palette", "opensnes-image",
    "opensnes-sample", "opensnes-music", "opensnes-save", "gfx4snes", "smconv", "wav2brr", "tmx2snes",
    "wla-spc700", "wla-superfx", "sa1_patch",
};

typedef struct { cli_ctx *ctx; int fails, warns; } doctor;

static void report(doctor *d, const char *status, const char *name, const char *detail)
{
    if (!strcmp(status, "fail")) d->fails++;
    if (!strcmp(status, "warn")) d->warns++;
    if (d->ctx->json) {
        cli_json_object(d->ctx, NULL); cli_json_str(d->ctx, "check", name); cli_json_str(d->ctx, "status", status);
        cli_json_str(d->ctx, "detail", detail); cli_json_close(d->ctx);
    } else {
        printf("  %s: %s%s%s\n", !strcmp(status, "ok") ? "OK" : !strcmp(status, "warn") ? "WARN" : "FAIL", name, *detail ? " — " : "", detail);
    }
}

static int count_objects(const char *dir)
{
    int n = 0;
#ifdef _WIN32
    char pat[PATHN];
    struct _finddata_t fd;
    snprintf(pat, sizeof pat, "%s/*.o", dir);
    intptr_t h = _findfirst(pat, &fd);
    if (h == -1) return 0;
    do n++; while (_findnext(h, &fd) == 0);
    _findclose(h);
#else
    DIR *d = opendir(dir);
    if (!d) return 0;
    for (struct dirent *e; (e = readdir(d));) {
        size_t l = strlen(e->d_name);
        if (l > 2 && strcmp(e->d_name + l - 2, ".o") == 0) n++;
    }
    closedir(d);
#endif
    return n;
}

static int run_doctor(cli_ctx *ctx)
{
    doctor d = { ctx, 0, 0 };
    char sdk[PATHN], p[PATHN], found[PATHN], line[512], detail[PATHN + 600];
    int have_sdk = find_sdk(sdk, sizeof sdk);
    if (ctx->json) { cli_json_begin(ctx); cli_json_array(ctx, "checks"); }
    else printf("OpenSNES Doctor\n========================================\n");
    if (!have_sdk) {
        report(&d, "fail", "SDK", "not found: set OPENSNES_HOME, or run the opensnes of the SDK's bin/");
    } else {
        report(&d, "ok", "SDK", sdk);
        /* cc65816 preprocesses with the host's `cc -E` before cproc */
        if (which("cc", found, sizeof found)) { first_line("cc --version", line, sizeof line); report(&d, "ok", "host cc", line); }
        else report(&d, "fail", "host cc", "not found; cc65816 needs it for the preprocessing stage");
        if (which("make", found, sizeof found)) report(&d, "ok", "make", found);
        else report(&d, "fail", "make", "not found on the PATH; the build is make/common.mk");
        int missing = 0;
        for (size_t i = 0; i < sizeof REQUIRED_BINS / sizeof REQUIRED_BINS[0]; i++) {
            snprintf(p, sizeof p, "%s/bin/%s", sdk, REQUIRED_BINS[i]);
            if (!is_exe(p, NULL, 0)) { snprintf(detail, sizeof detail, "not in %s/bin — every build needs it", sdk); report(&d, "fail", REQUIRED_BINS[i], detail); missing++; }
        }
        if (!missing) report(&d, "ok", "compiler, assembler, linker, opensnes-rom", "");
        missing = 0;
        for (size_t i = 0; i < sizeof ASSET_BINS / sizeof ASSET_BINS[0]; i++) {
            snprintf(p, sizeof p, "%s/bin/%s", sdk, ASSET_BINS[i]);
            if (!is_exe(p, NULL, 0)) { snprintf(detail, sizeof detail, "not in %s/bin — a project that uses it will not build", sdk); report(&d, "warn", ASSET_BINS[i], detail); missing++; }
        }
        if (!missing) report(&d, "ok", "asset and chip tools", "");
        if (which("clang", found, sizeof found)) report(&d, "ok", "clang", "the C lint pre-pass of every build");
        else report(&d, "warn", "clang", "not found — no C lint pre-pass (optional)");
        snprintf(p, sizeof p, "%s/lib/build/lorom", sdk);
        int objs = count_objects(p);
        if (objs) { snprintf(detail, sizeof detail, "%d objects (lorom)", objs); report(&d, "ok", "library", detail); }
        else report(&d, "fail", "library", "not built — the release zip ships it; in the repository, `make lib`");
        if (find_luna(sdk, found, sizeof found)) {
            char cmd[PATHN + 32];
            snprintf(cmd, sizeof cmd, "\"%s\" --version", found);
            first_line(cmd, line, sizeof line);
            snprintf(detail, sizeof detail, "%s (%s)", found, line);
            report(&d, "ok", "luna", detail);
        } else report(&d, "warn", "luna", "not found — scripts/install-luna.sh fetches it (`opensnes test` and the debugger need it)");
        if (find_emulator(sdk, NULL, found, sizeof found)) report(&d, "ok", "emulator", leaf_of(found));
        else report(&d, "warn", "emulator", "none found for `opensnes run` — luna-gui (scripts/install-luna.sh), Mesen, bsnes or snes9x");
        char proj[PATHN];
        if (walk_up(is_project, proj, sizeof proj)) report(&d, "ok", "project", proj);
        else if (!ctx->json) printf("  (no project in the current directory)\n");
    }
    if (ctx->json) { cli_json_close(ctx); cli_json_int(ctx, "failures", d.fails); cli_json_int(ctx, "warnings", d.warns); cli_json_end(ctx); }
    return d.fails ? CLI_REFUSED : CLI_OK;
}

/* =============================================================== upgrade */

#define MAX_NAMES 256
static struct { char name[64]; char what[320]; } names[MAX_NAMES];
static int nnames;

static int load_names(cli_ctx *ctx, const char *path, int changed)
{
    FILE *f = fopen(path, "r");
    if (!f) { cli_error(ctx, path, "the name list is missing from the SDK"); return 0; }
    char line[1024];
    while (fgets(line, sizeof line, f)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (!*line || *line == '#' || nnames >= MAX_NAMES) continue;
        char *what = strchr(line, '\t'), *header = NULL;
        if (!what) continue;
        *what++ = '\0';
        if ((header = strchr(what, '\t'))) *header++ = '\0';
        snprintf(names[nnames].name, sizeof names[0].name, "%s", line);
        if (changed) snprintf(names[nnames].what, sizeof names[0].what, "changes meaning: %s", what);
        else snprintf(names[nnames].what, sizeof names[0].what, "removed: use %s (removed from %s)", what, header ? header : "");
        nnames++;
    }
    fclose(f);
    return 1;
}

static int is_word(int c) { return isalnum(c) || c == '_'; }

static int scan_file(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    int hits = 0, lineno = 0;
    size_t cap = 4096, len;
    char *line = malloc(cap);
    while (line && fgets(line, (int)cap, f)) {
        len = strlen(line);
        while (len == cap - 1 && line[len - 1] != '\n') {       /* a long line: grow */
            char *g = realloc(line, cap * 2);
            if (!g) break;
            line = g;
            if (!fgets(line + len, (int)cap + 1, f)) break;
            cap *= 2; len = strlen(line);
        }
        lineno++;
        char *cut = strstr(line, "//");
        if (cut) *cut = '\0';
        for (char *p = line; *p;) {
            if (!is_word((unsigned char)*p)) { p++; continue; }
            char *e = p;
            while (is_word((unsigned char)*e)) e++;
            size_t wl = (size_t)(e - p);
            for (int k = 0; k < nnames; k++)
                if (strlen(names[k].name) == wl && strncmp(names[k].name, p, wl) == 0) {
                    printf("%s:%d: %s \xe2\x80\x94 %s\n", path, lineno, names[k].name, names[k].what);
                    hits++;
                    break;
                }
            p = e;
        }
    }
    free(line);
    fclose(f);
    return hits;
}

static char **files;
static int nfiles, capfiles;

static void add_file(const char *p)
{
    if (nfiles == capfiles) { capfiles = capfiles ? capfiles * 2 : 256; files = realloc(files, (size_t)capfiles * sizeof *files); }
    if (files) { files[nfiles] = malloc(strlen(p) + 1); if (files[nfiles]) strcpy(files[nfiles++], p); }
}

static int source_suffix(const char *name)
{
    const char *dot = strrchr(name, '.');
    return dot && (!strcmp(dot, ".c") || !strcmp(dot, ".h") || !strcmp(dot, ".asm") || !strcmp(dot, ".s") || !strcmp(dot, ".inc"));
}

static void collect(const char *dir)
{
    char p[PATHN];
#ifdef _WIN32
    struct _finddata_t fd;
    snprintf(p, sizeof p, "%s/*", dir);
    intptr_t h = _findfirst(p, &fd);
    if (h == -1) return;
    do {
        if (!strcmp(fd.name, ".") || !strcmp(fd.name, "..")) continue;
        snprintf(p, sizeof p, "%s/%s", dir, fd.name);
        if (fd.attrib & _A_SUBDIR) collect(p);
        else if (source_suffix(fd.name)) add_file(p);
    } while (_findnext(h, &fd) == 0);
    _findclose(h);
#else
    DIR *d = opendir(dir);
    if (!d) return;
    for (struct dirent *e; (e = readdir(d));) {
        if (!strcmp(e->d_name, ".") || !strcmp(e->d_name, "..")) continue;
        snprintf(p, sizeof p, "%s/%s", dir, e->d_name);
        if (is_dir(p)) collect(p);
        else if (source_suffix(e->d_name) && is_file(p)) add_file(p);
    }
    closedir(d);
#endif
}

static int by_name(const void *a, const void *b) { return strcmp(*(char *const *)a, *(char *const *)b); }

static const cli_opt upgrade_opts[] = {
    { "removed-only", 0, CLI_FLAG, NULL, "leave out the calls that kept their name and changed meaning (they are current API)", NULL },
};

static int run_upgrade(cli_ctx *ctx)
{
    char sdk[PATHN], p[PATHN], headers[PATHN];
    if (!require_sdk(ctx, sdk, sizeof sdk)) return CLI_REFUSED;
    nnames = 0;
    snprintf(p, sizeof p, "%s/make/removed_api.txt", sdk);
    if (!load_names(ctx, p, 0)) return CLI_IO;
    snprintf(p, sizeof p, "%s/make/changed_api.txt", sdk);
    if (!cli_has(ctx, "removed-only") && !load_names(ctx, p, 1)) return CLI_IO;
    snprintf(headers, sizeof headers, "%s/lib/include/snes/", sdk);

    for (int i = 0; i < ctx->nargs; i++) {
        char arg[PATHN];
        snprintf(arg, sizeof arg, "%s", ctx->args[i]);
        size_t l = strlen(arg);
        while (l > 1 && (arg[l - 1] == '/' || arg[l - 1] == '\\')) arg[--l] = '\0';
        if (is_file(arg)) add_file(arg);
        else if (is_dir(arg)) {
            int from = nfiles;
            collect(arg);
            if (files && nfiles > from) qsort(files + from, (size_t)(nfiles - from), sizeof *files, by_name);
        } else { cli_error(ctx, arg, "no such file or folder"); return CLI_REFUSED; }
    }
    int hits = 0, scanned = 0;
    for (int i = 0; i < nfiles; i++) {
        /* the SDK's own headers name the removed names on purpose */
        char full[PATHN];
        if (abs_path(files[i], full, sizeof full) && strncmp(full, headers, strlen(headers)) == 0) continue;
        hits += scan_file(files[i]);
        scanned++;
    }
    if (!ctx->quiet)
        printf("\nopensnes upgrade: %d hit(s) in %d file(s) (the names of make/removed_api.txt and make/changed_api.txt; docs/UPGRADING.md)\n", hits, scanned);
    return hits ? CLI_REFUSED : CLI_OK;
}

/* ================================================================== main */

static const cli_cmd cmds[] = {
    { "init", "create a project folder: main.c, Makefile, res/ (and two luna tests with --template game)", init_opts, CLI_N(init_opts),
      "init my-game --template game", 1, run_init },
    { "build", "build the project of the current folder (make)", build_opts, CLI_N(build_opts), "build --clean", 0, run_build },
    { "clean", "remove what the build made", NULL, 0, "clean", 0, run_clean },
    { "run", "build, then open the ROM in an emulator", run_opts, CLI_N(run_opts), "run --emulator mesen", 0, run_run },
    { "test", "run the project's tests (test/*.toml) in luna", test_opts, CLI_N(test_opts), "test --update", 0, run_test },
    { "doctor", "check the installation: SDK, host compiler, make, the binaries, the library, luna, an emulator (exit 1 on a failure)", NULL, 0,
      "doctor", 0, run_doctor },
    { "upgrade", "in your sources, the names OpenSNES 1.0 removed and the calls whose meaning changed, with what to use (exit 1 on a hit)",
      upgrade_opts, CLI_N(upgrade_opts), "upgrade src/", 1, run_upgrade },
};

int main(int argc, char **argv)
{
    static const cli_tool tool = {
        "opensnes", TOOL_VERSION, "the project tool of the OpenSNES SDK: create, build, run and test a game", cmds, CLI_N(cmds),
    };
    g_argv0 = argc ? argv[0] : NULL;
    return cli_main(&tool, argc, argv);
}
