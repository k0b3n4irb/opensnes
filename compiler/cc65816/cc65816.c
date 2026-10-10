/*
 * cc65816 - C compiler driver for the 65816 / SNES
 *
 * Runs the three stages of the pipeline and hands back WLA-DX assembly:
 *
 *   1. the host C preprocessor (cc -E -undef -nostdinc)
 *   2. cproc-qbe   C11 -> QBE IR
 *   3. qbe         QBE IR -> 65816 assembly (-t w65816)
 *
 * cproc-qbe and qbe are looked up beside this program. A bash script until
 * 2026-10-06 (compiler/scripts/cc65816): a compiled driver starts faster on
 * every translation unit and does not need a shell on the build machine.
 *
 * Usage: cc65816 input.c [-o output.asm] [-I dir] [-D name[=value]]
 *
 * Environment:
 *   CC65816_G         non-empty: pass -g to qbe (C locals stay in memory) and
 *                     ask cproc for the <output>.dbg aggregate-layout sidecar
 *   CC65816_KEEP_IR   directory that receives a copy of each unit's QBE IR
 *   CC65816_CPP       the preprocessor to run instead of cc / clang / gcc
 */

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <direct.h>
#include <process.h>
#include <windows.h>
#define PATH_SEP '\\'
#define EXE ".exe"
#else
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
extern char **environ;
#define PATH_SEP '/'
#define EXE ""
#endif

#define PATH_LEN 4096
#define SPAWN_FAILED (-1)

static char tmp_pp[PATH_LEN]; /* preprocessed source */
static char tmp_ir[PATH_LEN]; /* QBE IR */

static void cleanup(void)
{
    if (tmp_pp[0])
        remove(tmp_pp);
    if (tmp_ir[0])
        remove(tmp_ir);
}

static void on_signal(int sig)
{
    cleanup();
    _exit(128 + sig);
}

static void die(const char *msg, const char *arg)
{
    fprintf(stderr, "cc65816: %s%s%s\n", msg, arg ? ": " : "", arg ? arg : "");
    exit(1);
}

static void usage(void)
{
    puts("Usage: cc65816 input.c [-o output.asm]");
    puts("  Compiles C source to 65816 assembly for SNES");
    puts("");
    puts("Options:");
    puts("  -o FILE    Output assembly to FILE (default: stdout)");
    puts("  -I DIR     Add include directory");
    puts("  -D NAME    Define preprocessor macro");
    puts("  -h         Show this help");
    exit(1);
}

static int is_sep(char c)
{
#ifdef _WIN32
    return c == '/' || c == '\\';
#else
    return c == '/';
#endif
}

static const char *base_name(const char *path)
{
    const char *base = path;

    for (; *path; path++)
        if (is_sep(*path))
            base = path + 1;
    return base;
}

/* Directory holding this executable, without a trailing separator. */
static void self_dir(const char *argv0, char *out, size_t n)
{
    char buf[PATH_LEN];
    char *end;

    buf[0] = '\0';
#if defined(_WIN32)
    {
        DWORD r = GetModuleFileNameA(NULL, buf, (DWORD)sizeof buf);

        if (r == 0 || r >= sizeof buf)
            buf[0] = '\0';
    }
#elif defined(__APPLE__)
    {
        char raw[PATH_LEN];
        uint32_t sz = sizeof raw;

        if (_NSGetExecutablePath(raw, &sz) != 0 || !realpath(raw, buf))
            buf[0] = '\0';
    }
#else
    if (!realpath("/proc/self/exe", buf))
        buf[0] = '\0';
#endif
    if (!buf[0])
        snprintf(buf, sizeof buf, "%s", argv0);

    end = buf + strlen(buf);
    while (end > buf && !is_sep(end[-1]))
        end--;
    if (end == buf)
        snprintf(out, n, ".");
    else {
        end[-1] = '\0';
        snprintf(out, n, "%s", buf);
    }
}

static void set_env(const char *name, const char *value)
{
#ifdef _WIN32
    _putenv_s(name, value);
#else
    setenv(name, value, 1);
#endif
}

/* An empty file with a unique name in the temporary directory. */
static void make_temp(char *out, size_t n, const char *tag)
{
#ifdef _WIN32
    static int serial;
    const char *dir = getenv("TEMP");
    FILE *f;

    if (!dir || !*dir)
        dir = getenv("TMP");
    if (!dir || !*dir)
        dir = ".";
    snprintf(out, n, "%s\\cc65816_%s_%lu_%d.tmp", dir, tag,
             (unsigned long)GetCurrentProcessId(), serial++);
    f = fopen(out, "wb");
    if (!f) {
        out[0] = '\0';
        die("cannot create a temporary file in", dir);
    }
    fclose(f);
#else
    const char *dir = getenv("TMPDIR");
    int fd;

    if (!dir || !*dir)
        dir = "/tmp";
    snprintf(out, n, "%s/cc65816_%s.XXXXXX", dir, tag);
    fd = mkstemp(out);
    if (fd < 0) {
        out[0] = '\0';
        die("cannot create a temporary file in", dir);
    }
    close(fd);
#endif
}

#ifdef _WIN32
/* One argument of a Windows command line, quoted as the C runtime parses it. */
static char *quote_arg(const char *arg)
{
    size_t len = strlen(arg);
    char *out = malloc(2 * len + 3);
    char *p = out;
    size_t i, slashes = 0;

    if (!out)
        die("out of memory", NULL);
    if (len && !strpbrk(arg, " \t\"")) {
        memcpy(out, arg, len + 1);
        return out;
    }
    *p++ = '"';
    for (i = 0; i < len; i++) {
        if (arg[i] == '\\') {
            slashes++;
        } else if (arg[i] == '"') {
            for (; slashes; slashes--)
                *p++ = '\\';
            *p++ = '\\';
        } else {
            slashes = 0;
        }
        *p++ = arg[i];
    }
    for (; slashes; slashes--)
        *p++ = '\\';
    *p++ = '"';
    *p = '\0';
    return out;
}
#endif

/*
 * Run argv[0] (searched in PATH when it has no directory) and wait for it.
 * Returns its exit status, 128 + N when signal N killed it, or SPAWN_FAILED
 * when it could not be started.
 */
static int run(char *const argv[])
{
#ifdef _WIN32
    char **quoted;
    intptr_t rc;
    int argc = 0, i;

    while (argv[argc])
        argc++;
    quoted = calloc((size_t)argc + 1, sizeof *quoted);
    if (!quoted)
        die("out of memory", NULL);
    for (i = 0; i < argc; i++)
        quoted[i] = quote_arg(argv[i]);
    fflush(NULL);
    rc = _spawnvp(_P_WAIT, argv[0], (const char *const *)quoted);
    for (i = 0; i < argc; i++)
        free(quoted[i]);
    free(quoted);
    if (rc == -1)
        return SPAWN_FAILED;
    if ((DWORD)rc == 0xC0000005u) /* access violation */
        return 128 + SIGSEGV;
    if (rc == 0)
        return 0;
    return ((int)rc & 0xFF) ? ((int)rc & 0xFF) : 1;
#else
    pid_t pid;
    int status;

    fflush(NULL);
    if (posix_spawnp(&pid, argv[0], NULL, NULL, argv, environ) != 0)
        return SPAWN_FAILED;
    while (waitpid(pid, &status, 0) < 0)
        if (errno != EINTR)
            return SPAWN_FAILED;
    if (WIFSIGNALED(status))
        return 128 + WTERMSIG(status);
    return WEXITSTATUS(status);
#endif
}

static void make_dirs(const char *path)
{
    char buf[PATH_LEN];
    char *p;

    snprintf(buf, sizeof buf, "%s", path);
    for (p = buf + 1; *p; p++) {
        if (is_sep(*p)) {
            char c = *p;

            *p = '\0';
#ifdef _WIN32
            _mkdir(buf);
#else
            mkdir(buf, 0777);
#endif
            *p = c;
        }
    }
#ifdef _WIN32
    _mkdir(buf);
#else
    mkdir(buf, 0777);
#endif
}

static int copy_file(const char *from, const char *to)
{
    FILE *in = fopen(from, "rb");
    FILE *out;
    char buf[65536];
    size_t n;
    int ok = 1;

    if (!in)
        return 0;
    out = fopen(to, "wb");
    if (!out) {
        fclose(in);
        return 0;
    }
    while ((n = fread(buf, 1, sizeof buf, in)) > 0)
        if (fwrite(buf, 1, n, out) != n)
            ok = 0;
    if (ferror(in))
        ok = 0;
    fclose(in);
    if (fclose(out) != 0)
        ok = 0;
    return ok;
}

/*
 * Audit aid: CC65816_KEEP_IR=<dir> keeps every unit's QBE IR as
 * <dir>/<cwd and input path, separators as "__">.ssa, so corpus-wide IR
 * properties can be counted without a second front-end run.
 */
static void keep_ir(const char *dir, const char *input)
{
    char cwd[PATH_LEN], name[2 * PATH_LEN], flat[4 * PATH_LEN], dest[5 * PATH_LEN];
    const char *s;
    size_t len, o = 0;

#ifdef _WIN32
    if (!_getcwd(cwd, sizeof cwd))
#else
    if (!getcwd(cwd, sizeof cwd))
#endif
        die("cannot read the working directory", NULL);
    snprintf(name, sizeof name, "%s/%s", cwd, input);
    s = name;
    if (*s == '/')
        s++;
    for (; *s && o + 3 < sizeof flat; s++) {
        if (is_sep(*s) || *s == ' ') {
            flat[o++] = '_';
            flat[o++] = '_';
        } else {
            flat[o++] = *s;
        }
    }
    flat[o] = '\0';
    len = strlen(flat);
    if (len >= 2 && strcmp(flat + len - 2, ".c") == 0)
        flat[len - 2] = '\0';
    make_dirs(dir);
    snprintf(dest, sizeof dest, "%s/%s.ssa", dir, flat);
    if (!copy_file(tmp_ir, dest))
        die("cannot write", dest);
}

int main(int argc, char *argv[])
{
    const char *input = NULL, *output = NULL, *env;
    char dir[PATH_LEN], cproc[PATH_LEN + 32], qbe[PATH_LEN + 32];
    static char **cpp; /* static: still reachable at exit, whichever return is taken */
    int ncpp = 0, i, rc, debug;

    /* cpp: [cc -E -undef -nostdinc -D__OPENSNES__=1] flags... input -o tmp */
    cpp = calloc((size_t)argc + 16, sizeof *cpp);
    if (!cpp)
        die("out of memory", NULL);
    cpp[ncpp++] = "cc";
    cpp[ncpp++] = "-E";
    /*
     * -undef and -nostdinc (2026-10-03): the host's predefined macros
     * (__x86_64__, __aarch64__, __linux__...) made the same source give a
     * different ROM on each build machine, and the host's <stdint.h> was
     * read with cproc's sizes (int32_t came out 2 bytes) without a word.
     * A `#include <stdint.h>` now fails at the preprocessor: use the
     * fixed-width types of <snes/types.h>. The standard macros stay.
     */
    cpp[ncpp++] = "-undef";
    cpp[ncpp++] = "-nostdinc";
    /*
     * Lets a header tell this target from the host lint pass of
     * make/common.mk (clang -fsyntax-only, host int and long sizes);
     * snes/types.h uses it to keep sizeof(s32) == 4 on both.
     */
    cpp[ncpp++] = "-D__OPENSNES__=1";

    for (i = 1; i < argc; i++) {
        const char *a = argv[i];

        if (strcmp(a, "-o") == 0) {
            if (++i >= argc)
                usage();
            output = argv[i];
        } else if (strcmp(a, "-I") == 0 || strcmp(a, "-D") == 0) {
            if (i + 1 >= argc)
                usage();
            cpp[ncpp++] = argv[i++];
            cpp[ncpp++] = argv[i];
        } else if (strncmp(a, "-I", 2) == 0 || strncmp(a, "-D", 2) == 0) {
            cpp[ncpp++] = argv[i];
        } else if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0) {
            usage();
        } else if (a[0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", a);
            usage();
        } else if (!input) {
            input = a;
        } else {
            fprintf(stderr, "Multiple input files not supported\n");
            return 1;
        }
    }
    if (!input) {
        fprintf(stderr, "Error: No input file specified\n");
        usage();
    }
    {
        struct stat st;

        if (stat(input, &st) != 0 || !S_ISREG(st.st_mode)) {
            fprintf(stderr, "Error: Input file not found: %s\n", input);
            return 1;
        }
    }

    self_dir(argv[0], dir, sizeof dir);
    snprintf(cproc, sizeof cproc, "%s%ccproc-qbe" EXE, dir, PATH_SEP);
    snprintf(qbe, sizeof qbe, "%s%cqbe" EXE, dir, PATH_SEP);

    atexit(cleanup);
    signal(SIGINT, on_signal);
    signal(SIGTERM, on_signal);
    make_temp(tmp_pp, sizeof tmp_pp, "pp");
    make_temp(tmp_ir, sizeof tmp_ir, "ir");

    /* Stage 1: preprocess with the host compiler. */
    cpp[ncpp++] = (char *)input;
    cpp[ncpp++] = "-o";
    cpp[ncpp++] = tmp_pp;
    cpp[ncpp] = NULL;
    env = getenv("CC65816_CPP");
    if (env && *env) {
        cpp[0] = (char *)env;
        rc = run(cpp);
    } else {
        static const char *const hosts[] = { "cc", "clang", "gcc" };
        size_t h;

        rc = SPAWN_FAILED;
        for (h = 0; h < sizeof hosts / sizeof *hosts && rc == SPAWN_FAILED; h++) {
            cpp[0] = (char *)hosts[h];
            rc = run(cpp);
        }
    }
    if (rc == SPAWN_FAILED)
        die("no host C preprocessor found (tried cc, clang, gcc; set CC65816_CPP)", NULL);
    if (rc != 0)
        return rc;

    /*
     * Cooper source-level debug: CC65816_G keeps C locals in memory (qbe -g,
     * which disables optimisation) and has cproc write the aggregate-layout
     * sidecar the debugger expands Locals from.
     */
    env = getenv("CC65816_G");
    debug = env && *env;
    if (debug && output) {
        char dbg[PATH_LEN];
        size_t len = strlen(output);

        if (len >= 4 && strcmp(output + len - 4, ".asm") == 0)
            len -= 4;
        snprintf(dbg, sizeof dbg, "%.*s.dbg", (int)len, output);
        set_env("CC65816_DBG", dbg);
    }

    /*
     * Per-unit label prefix. cproc numbers anonymous local-linkage globals
     * (string literals, __func__, compound literals) from a counter that
     * restarts at 0 in every translation unit, and the w65816 backend strips
     * QBE's ".L" local marker, so two .c files would ship the same label
     * (string.15) and wlalink would reject the duplicate. The file's stem
     * namespaces them (<tu>_string.15); exported symbols are untouched.
     */
    {
        char tu[PATH_LEN];
        char *p, *dot;

        snprintf(tu, sizeof tu, "%s", base_name(input));
        dot = strrchr(tu, '.');
        if (dot)
            *dot = '\0';
        for (p = tu; *p; p++)
            if (!isalnum((unsigned char)*p) && *p != '_')
                *p = '_';
        set_env("CC65816_TU", tu);
    }

    /*
     * Stage 2: C -> QBE IR, to a file and not a pipe: with a pipe a cproc
     * failure reaches qbe as truncated IR and a misleading parse error.
     */
    {
        char *args[] = { cproc, "-o", tmp_ir, tmp_pp, NULL };

        rc = run(args);
    }
    if (rc == SPAWN_FAILED)
        die("cannot run", cproc);
    if (rc != 0) {
        if (rc == 128 + SIGSEGV)
            fprintf(stderr,
                    "cc65816: cproc crashed (SIGSEGV) on %s — a compiler bug, "
                    "please report it with this file\n",
                    base_name(input));
        return rc;
    }

    env = getenv("CC65816_KEEP_IR");
    if (env && *env)
        keep_ir(env, input);

    /* Stage 3: QBE IR -> 65816 assembly. */
    {
        char *args[8];
        int n = 0;

        args[n++] = qbe;
        if (debug)
            args[n++] = "-g";
        args[n++] = "-t";
        args[n++] = "w65816";
        if (output) {
            args[n++] = "-o";
            args[n++] = (char *)output;
        }
        args[n++] = tmp_ir;
        args[n] = NULL;
        rc = run(args);
    }
    if (rc == SPAWN_FAILED)
        die("cannot run", qbe);
    if (rc != 0)
        return rc;
    if (output)
        printf("Generated: %s\n", output);
    return 0;
}
