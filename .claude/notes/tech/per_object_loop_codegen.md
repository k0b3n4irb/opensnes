# The per-object loop: what cc65816 generates, what is too much (2026-10-30)

A game built on the SDK sent (2026-10-30) the reduced, autonomous case below:
the loop that places N sprites in the OAM shadow every frame (a structure of
word arrays, N = 64; 25 to 70 entries per frame in play). It is the first
item of that game's critical path. Nothing of the game is in this file: the
case is its own reduction, in SDK terms, and the figures below are measured
on the case, not in the game.

## The case

```c
/* Cas réduit autonome (rien d'un jeu) : la boucle par entrée la plus coûteuse mesurée dans un portage, qui pose
 * N sprites dans l'ombre de l'OAM à chaque image. Structure de tableaux de mots nommés, N = 64 (40 à 100 en pratique).
 * Par entrée : le mot d'attribut (une priorité ajoutée quand ses bits sont nuls), la taille comparée à celle déjà
 * posée, la place (x − caméra, y), un test de sortie d'écran sur chaque axe, deux mots écrits, et le neuvième bit
 * de x comparé à celui déjà posé. Les deux appels rares (taille, neuvième bit) sont remplacés par des compteurs.
 * Compiler : cc65816 boucle_objets.c -o boucle_objets.asm */
typedef unsigned char u8;
typedef unsigned short u16;
typedef short s16;

#define N 64
u16 n = N;                              /* le nombre d'entrées de ce tour */
s16 px[N], py[N];                       /* la place de chaque entrée */
u16 ta[N];                              /* son mot d'attribut */
u16 big[N], slot_big[N];                /* sa taille, celle déjà posée */
u16 x9[N];                              /* le neuvième bit de x déjà posé */
u8 shadow[4 * N + 4];                   /* l'ombre de l'OAM */
u16 cam_x, prio;
u16 size_calls, x9_calls;               /* à la place des appels rares */

void place_all(void) {
    u16 i, id, w;
    s16 sx, sy;
    for (i = 0, id = 0; i < n; i++, id += 4) {
        w = ta[i];
        if (!(w & 0x3000))
            w |= prio;
        *(u16 *)(shadow + id + 2) = w;
        if ((w = big[i]) != slot_big[i]) {
            slot_big[i] = w;
            size_calls++;
        }
        sx = px[i] - cam_x;
        sy = py[i];
        if ((u16)(sx + 31) >= 287 || (u16)(sy + 31) >= 255) {  /* hors de l'écran */
            *(u16 *)(shadow + id) = 224 << 8;
        } else {
            *(u16 *)(shadow + id) = (u8)sx | (sy << 8);
            if ((sx & 0x100) != x9[i]) {
                x9[i] = sx & 0x100;
                x9_calls++;
            }
        }
    }
}
```

## The count (compiler at `f7f11dfe`)

Common path (visible, priority added, no rare call): `for_cond` 5, body
7 + 3 + 10, position 15 + 5, write and ninth bit 18 + 1, `for_cont` 6 =
**70 instructions** per iteration (in play, the iterations where a rare
call runs add to that).

### What the C can do today: 64 (−6)

`*(u16 *)(shadow + id)` is a store through a `u16` pointer: it may alias any
`u16` global, so `n`, `cam_x` and `prio` are reloaded after every iteration,
rightly. Copied to locals before the loop (`u16 cnt = n, cx = cam_x, pr =
prio;`): the end test goes from `lda.w n / sta lf / lda i / cmp lf / bcc /
jmp` to `lda i / cmp.b lf / bcs`, the subtraction from eight instructions to
`lda.w px,x / sec / sbc.b lf / sta`, and the loop is rotated (no `jmp
@for_cond`). Two byte stores (`shadow[id] = (u8)sx; shadow[id + 1] =
(u8)sy;`) gain nothing: 12 instructions (`sep / sta / rep` twice, the index
recomputed; `try_fuse_bytepair_store` fuses the two bytes of ONE value, not
two values) against 10 for `(u8)sx | (sy << 8)`.

### Delivered (2026-10-31)

Measured with `luna profile` on the case with its locals, 64 entries, 60
calls: **64.2 to 55.2 instructions an entry, 1,940 to 1,656 master cycles**
(-14.7 %). Without the locals: 70.1 to 61.1, 2,167 to 1,883. Waste 1 and 2 went
together: the copy of `id` took the slot `w`'s phi needed. Waste 4 went by
moving the load past the first test (`sinkload`), which also saves it on the
way off screen; 3 and 5 by `peephole_slotdead` and the reload rule. Mechanisms:
`.claude/rules/compiler.md`. Row 6 below is what is left, and A11 holds it.

What it found: `world_sprites` boots one frame earlier and its picture
baseline moved (A/B 60/60 at +-8); three audio hashes moved by phase (MATCH, at
most 0.62 %).

### What was the compiler's (the lot "per-object loop")

| # | Waste | Instructions |
|---|---|---|
| 1 | the conditional `w` (`if (!(w & 0x3000)) w |= pr`) lives in two temporaries, each branch copies (`lda lf+4 / sta lf+6 / jmp` on the nothing-to-do path): phi coalescing | 3 |
| 2 | `id` copied to a second temporary before `tax`, reloaded from there later (`lda lf+2 / sta lf+6 / tax … ldx lf+6`) | 2 |
| 3 | `sta lf+8` of `w = big[i]` is dead: the `sta slot_big,x` uses A | 1 |
| 4 | the load of `py[i]` is scheduled between the subtraction and its use, so `sx` is stored and reloaded | 2 |
| 5 | `sta lf / ldx lf+0 / lda lf` around an `ldx` that leaves A intact | 1 |

About 9: **≈ 55** with the locals. All five are copy coalescing and
reloads after a fusion, in `compiler/qbe/w65816/emit.c` (`try_fuse_*`, the
slot allocation) — a bounded lot: worktree, `difftest` and `difftest_stmt`
hunts beyond the gate, corpus A/B (`diff_corpus.py`), the measure on this
loop compiled alone (instructions on the common path, luna cycles over 64
entries) before and after, given to the game at delivery.

| 6 | three induction variables (`i`, `i*2`, `id`: `asl / sta / tax` at the top, six instructions in `for_cont`) where a hand-written loop keeps X = i*2 and **Y = id** | ≈ 10 |

The compiler never uses Y as an index register: 0 indexed `,y` accesses
against 429 `,x` in the examples' `main.c.asm` (Y serves only the
`[tcc__fp],y` far-frame accesses). This is the ceiling of every loop with
two indices, and a multi-week backend chantier, not a lot:
`STRUCTURAL_DEFECTS.md` A11, undated. The 35 instructions of a hand-written
body are that floor.

## How to measure

```sh
bin/cc65816 boucle_objets.c -o boucle_objets.asm    # the piece in the thread
# count the common path by hand on the blocks for_body → if_true.7 → if_false.8
# → if_false.10 → logic_right.11 → if_false.14 → if_false.17 → for_cont.5
```

A luna measurement of the loop alone is a small ROM calling `place_all` once
a frame with `n = 64`: `luna profile --from-frame 10 --until-frame 70 --sym
m.sym m.sfc`, the rows `place_all*` summed, over 60 calls of 64 entries.
