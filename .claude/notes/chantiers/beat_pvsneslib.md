# Chantier — ahead of PVSnesLib on every counter

**Opened 2026-10-08, plan approved by the owner the same day.** The plan
below is the one he approved (in French, as written); this header is the
live part.

## Status

| Stage | State | Commit | Figures after it |
|---|---|---|---|
| S0 measurement | done; the library bench came on 2026-10-09 with S6 | `d53f8605` | faster on 15 of 18, no larger on 13, no deeper in stack on 0 |
| S1 link only what is referenced | done 2026-10-08. Minimal ROM 9.8 KB -> 2.4 KB in bank $00 (PVSnesLib 2.0 KB): the 0.6 KB left is the mouse / Super Scope / multitap readers the NMI handler always calls — make them linked only when their init is called, with the NMI work of S6 | `87e85fe7` | same three counts (S1 does not touch codegen) |
| S2 dead code | done 2026-10-08 (qbe `f832c8e`: `sinkref` recurses on the emitted copy, `sweepdead` after `sink`). sort +63 % -> -7.5 %, collide +13 % -> -4.5 %, physics -4 % -> -20 %. 28 ROMs change, 86/86 MATCH, audio unchanged, WRAM re-captured (17 streams: stack, scratch, one code pointer) | `313df292` | faster on 17 of 18 (-27.0 %), no larger on 15 (-15.9 %), no deeper in stack on 0 |
| S3 branches | done 2026-10-08 (qbe `7eed8b3`: `threadjnz` before gvn with trivial-phi removal, `simpljnz`, `cmplast`, sign test against 0, 32-bit equality fused). Exit criteria missed by a little on two lines — sort -19.2 % for -20, collide -11.6 % for -15 — what is left in both loops is address arithmetic (S4) and the phi copies (S5), no materialised condition remains. Found on the way: an upstream gvn inference bug (`phicopyref` on a dead edge, fixed), a pre-existing slot-ownership ICE on seed 19645 (fixed in the commit after, qbe `b2a7a26`), and a boot race in `gsuDmaFullFrame` (lib, fixed in the next commit) | `494a199a` | faster on 18 of 18 (-30.4 %), no larger on 15 (-18.2 %), no deeper in stack on 0 |
| S4 addressing | part 1 done 2026-10-08: near `sym[index]` through the far path's indexed-long form (`near_indexed`, `is_far()`), 16-bit x small constant as an inline 32-bit product. Exit lines met: list -19.9 % (for -10), collide -26.9 % (for -25), no `jsl tcc_mul32` left in `list`. Not met: `array2d_read` in the static table (the form is (sym + i) + j, not sym + i). Left for part 2: the 2D form, `p->field` as `tax; lda.l N,x`, the X cache (X is reloaded before each access), the index's dead store and high half | `fb672b2b` | faster on 18 of 18 (-38.4 %), no larger on 18 (-26.1 %), no deeper in stack on 0 |
| S4 part 2 | done 2026-10-08: `p->field` and every 8/16-bit access through an address temp are X-indexed, X is kept between accesses; a 32-bit load through a pointer keeps `[tcc__r9]` (changing it would read bank $00 where it reads the pointer's bank today: a decision for S5, with the corpus as proof — it is what holds `list` at -20 % instead of -38 %). With it, the shift-width fix in `copy.c` (seed 54084). Static table 1637 -> 1593, `struct_sum` won; `array_read`, `array_write`, `array2d_read` still lose there (frames: S5; the 2D form: not done). `grid` went 8 bytes deeper in stack (84 -> 92) | `4d6624b9` | faster on 18 of 18 (-40.4 %), no larger on 18 (-27.1 %), no deeper in stack on 0 |
| S5 frames and ABI | step 1 done 2026-10-08 (qbe `e24db85`): an alloc that `promote` removed no longer keeps its words of frame. 86/86 MATCH at equal frame, audio unchanged, 2 WRAM streams. Stack: deeper by 1 to 72 bytes on 17 workloads (1 to 106 on 18 before), `long` 9 bytes shallower than PVSnesLib. Next, in this order: the parameter copy of a non-leaf function (`lda 14,s / sta 4,s` in `fib`: aliasing is tied to `leaf_opt`, 25 sites), the dead store before a return, then (b) `rep #$20` to the caller and (c) direct-page temporaries for leaves. Also to decide here: the 32-bit load through a near pointer (`list`) | `c53cd5ab` | faster on 18 of 18 (-40.4 %), no larger on 18 (-27.1 %), no deeper in stack on 1 |
| S5 step 2 | done 2026-10-08 (qbe `c88fc56`): parameters read in place in every function (`alias_opt`), slots assigned after the alias and dead-store analyses (`temp_noslot`), `framesize` 0 when no slot. `calls` 231 -> 193 bytes of stack and -37 % cycles. 86/86 MATCH at equal frame. Caught in validation, never shipped: parameters read 2 bytes too high in a function with calls and no slot (25 examples without text; pinned as p11). Found by the new check: the Omul double load (silent, old, fixed). Left for the stack column: the dead store before a return in non-leaf functions, `grid` (+45) and `strings` (+28) to read function by function, then (b) `rep #$20` and (c) direct-page temporaries | `f700f8f6` | faster on 18 of 18 (-40.8 %), no larger on 18 (-27.5 %), no deeper in stack on 1 (deeper by 1 to 45 on the others) |
| S5 step 3 | done 2026-10-09 (qbe `db935a1`): the value a block returns gets no slot in any function (`mark_dead_stores` case R). `calls` 193 -> 159 bytes, level with PVSnesLib. 86/86 MATCH at equal frame, 7 ROMs change, WRAM and audio baselines untouched. With it a cproc fix (seed 103247: a bit-field chosen by a constant condition). Left for the stack column: `grid` (+45) and `strings` (+28), then (b) and (c) | `54f2bab3` | faster on 18 of 18 (-41.0 %), no larger on 18 (-27.7 %), no deeper in stack on 2 (deeper by 1 to 45 on the others) |
| S5 step 4 | done 2026-10-09 (qbe `efc6996`, the plan's item (a)): an address-only Kl temp takes one word (`temp_narrow`, `narrow_ok`), Oshl Kl has a low-half-only path. Stack level or shallower on 7 of 18 (2 before). 86/86 MATCH at equal frame. With it a gvn fix (seed 124152: phis of different classes merged; 0 ROMs change). Left: `grid` (+29), `strings` (+28), `tilemap` (+8), `mul` (+7), then (b) `rep #$20` and (c) direct-page temporaries | `477259cd` | faster on 18 of 18 (-41.1 %), no larger on 18 (-27.8 %), no deeper in stack on 7 (deeper by 1 to 29 on the others) |
| S5 step 5 | done 2026-10-09 (qbe `576aa1d`). **Departs from the plan's item (b)**: `rep #$20` does NOT move to the caller. Measured, it is 3 cycles a call, under 1 % of the workloads, for a new silent failure (assembly calling C in 8-bit mode). Instead a static function whose address is never used leaves it out — only compiled C can enter it. No ABI change, no asm audit, no lint extension. The two static-table rows it was meant to win (`pea_constant_args`, `mod_const_10`) stay 1 to 4 cycles behind: exported functions | `dab43b39` | faster on 18 of 18 (-41.2 %), no larger on 18 (-28.3 %), no deeper in stack on 7 |
| S5 step 6 | done 2026-10-09 (qbe `c794f42` + crt0, the plan's item (c)): the frame of a leaf function is in the direct page (`tcc__lf`, 16 words at `$0080`; the NMI page grows to 160 bytes). Static table 1593 -> 1380 (-30.3 %), 29 wins of 34. Stack level or shallower on 16 of 18. 85/86 MATCH at equal frame (`extbg` 4 frames sooner). New fixture `leaf_frame`. First attempt put the block at `$34` and collided with SNESMOD's `$40-$7F` (link error, three examples). Left for S5: `grid` (+24) and `strings` (+10) — single functions over 16 words; the 32-bit load through a near pointer (`list`) | `705b91df` | faster on 18 of 18 (-43.8 %), no larger on 18 (-30.4 %), no deeper in stack on 16 |
| S6 library hot paths | started 2026-10-09: the device readers are optional (pointers set by `mouseInit()` / `scopeInit()`): minimal ROM 2.4 KB -> 1.9 KB of bank $00, under PVSnesLib's 2.0 KB (the remainder S1 had left). 86/86 MATCH, audio unchanged. Decided for S5 the same day: a 32-bit load through a plain pointer keeps `lda [tcc__r9]`. It reads the pointer's bank where the 8- and 16-bit loads read bank $00; aligning it on them made `list` faster when tried on 2026-10-08 (-38 % against -20 % that day) but would break any code that reads a long through a pointer to another bank, which works today — not worth one workload. Then the library bench (`devtools/libbench`, `make bench-lib`, CI gate at +10 % a row), same day: its first run had us behind on 7 rows of 8 (pad x3.5, `oamSetXY` x3.6, text +83 %, size +41 %, scroll +13 %, `oamSet` +9 %). Rewritten: pads as macros + `padReleased` in assembly, the five sprite setters and the three scroll setters in assembly over a mask table, `textPrint` by runs, `dmaCopyVram` without php/plp. Now 9 rows of 9 at or under PVSnesLib (`oamxy` -1.2 % and `dma` -0.1 % are ties; whole frame -15.9 %). One example a frame earlier at boot (scroll_message, MATCH at offset -1), two SNESMOD onsets 530 samples earlier, aim_target moves 30 px where it moved 20 (its redraw is faster). Not done, and not needed for the criterion: the 3089-mclk NMI block (idle is already -19.5 %). Seen on the way, for a later compiler pass: a computed argument is stored in its slot and loaded again to be pushed (about 20 cycles on a three-argument call); a `u8` compared with a constant uses the signed sequence | `59be2a72`, `9ef1faeb` | minimal ROM 1.9 KB (PVSnesLib 2.0 KB); library bench 9/9 |
| S7 parity (string module, small gaps) | started 2026-10-09: module `string` (`memcpy`, `memmove`, `memset`, `strlen`, `strcmp`, `strcpy`, `strncpy`, assembly, 24-bit on both sides, one section per function, 24 assertions in libtests). Found with it: octal escapes in string literals (silent, old, fixed in the commit before). Then `oamGetX` / `oamGetY` (`735592d2`); `setPaletteColor` was never a gap, `setColor` does it and the guide said otherwise. No row of the migration guide says "Gap" any more outside the non-goals. Still to do: the example twins | `439526a8` | the migration guide's `memcpy` exists |

Update a row when its stage merges: the commit, and the three counts
printed by `make bench-sdk`. The scoreboard is `devtools/sdkbench`.

One finding of the first measurement that the plan did not have: the
stack column. OpenSNES goes deeper than PVSnesLib on all eighteen
workloads (14 to 106 bytes), because every temporary owns a stack slot
where PVSnesLib uses direct-page pseudo-registers. It is the S5 stage's
exit criterion and the reason S5(c) is in tier 1.

---

# Plan — Passer devant PVSnesLib sur tous les compteurs (2026-10-08)

## Contexte

Le benchmark du jour (`docs/BENCHMARK.md`, `make bench`, `make bench-sdk`)
donne OpenSNES 17 à 18 % plus rapide que PVSnesLib au total, mais perdant
sur tout ce qui indexe un tableau ou suit un pointeur : tri +63 %,
collisions +13 %, liste chaînée +7 %, et quatre fonctions de la table
statique. Le propriétaire veut un SDK de 2026 devant sur **tous** les
compteurs, et plus largement sur tous les axes où PVSnesLib est encore
devant ou à égalité.

Trois explorations et une relecture de conception ont établi ceci.

**Le diagnostic de mai était faux.** La perte ne vient pas des pointeurs
sur 4 octets : nous déréférençons en 16 bits, PVSnesLib fait un vrai accès
24 bits. Les causes réelles, mesurées instruction par instruction :

| Cause | Où ça pèse |
|---|---|
| Code mort : des calculs 32 bits d'indice et d'adresse rangés puis jamais relus | 258 des 496 cycles par tour du tri, 84 des 456 des collisions |
| Booléens matérialisés pour `&&`, `\|\|` et les comparaisons, puis retestés | 66 cycles (tri), ~125 (collisions) |
| Valeurs rangées en pile puis relues aussitôt ; un cadre de pile pour de simples temporaires | 22 à 60 cycles par fonction de la table statique |
| Adresse construite dans A puis `tax ; lda.l $0000,x` au lieu d'un mode indexé | ~95 (tri), ~130 (collisions) |
| Multiplication 32 bits appelée pour `&nodes[k]` (taille 6) | toute la perte de la liste : 480 000 cycles |
| `rep #$20` à l'entrée de chaque fonction | 3 cycles : à lui seul il fait perdre `pea_constant_args` et `mod_const_10` |

PVSnesLib lui-même est loin d'être bon : il n'utilise jamais X ni Y comme
indice. Le battre partout coûte peu ; viser le code qu'écrirait un humain
(tri : 27 cycles par tour, nous 496, eux 255) est un second étage.

**Hors compilateur**, PVSnesLib est devant sur trois points : une ROM
minimale embarque ~9,8 Ko de notre bibliothèque contre ~2,0 Ko (nous lions
les modules entiers, eux passent `-d` à l'éditeur de liens) ; nous n'avons
ni `memcpy`, ni `memset`, ni `strlen` alors que notre guide de migration
conseille `memcpy` ; quelques appels de bibliothèque sont lourds
(`padHeld` ~1 000 cycles par trame là où PVSnesLib a une macro).

## Choix de périmètre (à valider en approuvant le plan)

- **L'étage 1 passe avant l'étiquette 1.0.** Il contient un changement
  d'ABI (`rep #$20` à la charge de l'appelant) qui doit précéder le gel,
  et le benchmark est public. Coût : environ cinq semaines.
- **L'étage 2 est planifié mais ne démarre pas avant la 1.0.** C'est le
  seul qui ouvre une classe nouvelle de défauts silencieux (durée de vie
  de X).
- **Restent des non-buts** (PHILOSOPHY.md) : `printf`, `malloc`,
  flottants, scores BCD, mode pixel. Ils ne sont pas dans ce plan.
- La fenêtre des deux semaines repart à la fusion de la dernière étape
  de compilateur (S5), comme accepté.

## Étage 1 — devant PVSnesLib partout (~25 jours)

Ordre imposé : mesurer d'abord (S0) ; le code mort (S2) avant les
branches et l'adressage, parce qu'il bloque les deux ; l'ABI et les
cadres (S5) en dernier côté compilateur (plus grand rayon d'effet) ; la
bibliothèque (S6) après, car ses fonctions C rétrécissent toutes seules.

| # | Étape | Jours | Critère de sortie |
|---|---|---|---|
| S0 | Mesure | 3 | 18 charges × 3 colonnes (cycles, taille, pile) pour les deux SDK ; banc de la bibliothèque ; garde-fou CI |
| S1 | Ne lier que ce qui est référencé | 1,5 | bibliothèque d'une ROM minimale ≤ 2,0 Ko ; corpus identique à l'image |
| S2 | Code mort | 3 | tri ≤ 0 %, collisions ≤ 0 % ; tailles tri/collisions ≤ PVSnesLib |
| S3 | Branches | 3 | tri ≤ −20 %, collisions ≤ −15 %, physique ≤ −10 % |
| S4 | Adressage | 4 | liste ≤ −10 %, collisions ≤ −25 %, `array2d_read` ≤ 112 ; plus aucun `jsl tcc_mul32` dans les charges |
| S5 | Cadres de pile et ABI | 5 | **34 sur 34 dans la table statique** ; colonne pile ≤ PVSnesLib |
| S6 | Points chauds de la bibliothèque | 3 | chaque ligne du banc bibliothèque ≤ PVSnesLib |
| S7 | Parité : `string`, petits manques | 3 | plus aucune ligne « Gap » dans le guide de migration hors non-buts |

### S0 — Mesure (classe D)

- `devtools/sdkbench/workloads.c` : six charges de plus — `tilemap`,
  `grid2d`, `entities` (mise à jour à travers un pointeur), `memcopy`,
  `strings`, `statemachine` (switch + table de pointeurs de fonction).
- `devtools/sdkbench/run.py` : colonnes **taille** (sections `.text.w_*`
  de notre `.sym` ; distance entre étiquettes côté PVSnesLib, `wla_sym()`
  les lit déjà) et **pile** (`luna profile --stack-floor`, déjà utilisé
  par `testing/rom_coverage.py`).
- Nouveau `devtools/libbench/` : la même scène avec les deux SDK (lecture
  manette, `bgSetScroll` ×3, `oamSet` ×32, DMA de 2 Ko, 20 caractères de
  texte, trame vide), mesurée comme `docs/PERF.md`.
- Garde-fou : PVSnesLib n'est pas dans la CI. On versionne
  `devtools/sdkbench/baseline.json` (nos chiffres) avec un job CI qui
  construit nos ROM, lance luna et refuse +5 % au total ou +25 % par
  charge ; et `pvsneslib_reference.json` (daté, avec le commit de
  PVSnesLib) pour que la page s'imprime sans leur arbre.

### S1 — Ne lier que ce qui est référencé

- `make/common.mk:670` : `$(LD) -S linkfile` devient `$(LD) -d -S`,
  derrière `LD_DISCARD ?= 1`. `wlalink` a l'option (`main.c:1204`),
  PVSnesLib l'utilise (`snes_rules:172`), et notre bibliothèque émet déjà
  une section par fonction.
- Audit avant bascule (`-v1` liste ce qui est écarté) : tout ce qui est
  atteint par adresse et non par symbole reçoit `KEEP` — chaînes
  `.data_init` et `.ram_code` (fusionnées avant l'écartement, à
  confirmer), banque de sons SNESMOD, sections de `profile.asm` /
  `debug.asm`.
- `devtools/link_modules.py` doit lier **sans** `-d`, sinon une
  dépendance non déclarée entre modules est masquée.
- `baselines/never_executed.txt` rétrécit (une fonction absente de la
  ROM n'est plus « jamais exécutée ») : une recapture, expliquée.

### S2 — Code mort (milieu de chaîne QBE, indépendant de la cible)

- `compiler/qbe/gcm.c`, `sinkref()` (384-410) : la copie « coulée » près
  de son usage référence encore les opérandes d'origine, et les copies
  d'opérandes ne sont référencées par personne. Faire référencer les
  opérandes coulés par l'instruction émise (attention : l'émission se
  fait à rebours). Comparer d'abord avec `gcm.c` amont.
- Même fichier, après `sink()` : un balayage qui retire toute instruction
  dont le résultat n'a plus d'usage, avec le prédicat que `gcmmove()`
  emploie déjà (`pinned` / `canelim` : appels, écritures, lectures
  `volatile` restent), jusqu'au point fixe. **Exclure `Oalloc*`** (le
  backend a déjà photographié les tailles).
- Pourquoi pas dans l'émetteur : il faudrait y recopier les règles des
  deux moitiés 32 bits, et les analyses en amont de l'émission
  continueraient de compter les consommateurs morts.

### S3 — Branches

- Nouvelle passe QBE (après `gcm()`, avant `abi1`) : un bloc qui ne
  contient qu'un phi de constantes 0/1 et de résultats de comparaison,
  suivi d'un `jnz` sur ce phi, est court-circuité — chaque prédécesseur
  saute directement, ou branche sur sa comparaison (qui redevient la
  dernière instruction du bloc, donc fusionnable par `emit.c:6149`).
  cproc n'est pas touché.
- `emit.c`, chemin fusionné de `emitjmp` : une comparaison signée contre
  la constante 0 devient `lda ; bmi/bpl` au lieu de
  `sec ; sbc #0 ; bvc ; eor #$8000 ; bmi`.

### S4 — Adressage (`compiler/qbe/w65816/emit.c`)

- `mark_far_decomp` (654-729) étendu aux accès proches :
  `add $sym, %idx` → `lda.l sym,x` ; `add %p, N` → `tax ; lda.l N,x`.
  L'adressage long porte l'adresse 24 bits de l'éditeur de liens : aucune
  preuve de banque à fournir. `far_decomp_all` supprime alors l'addition.
- Un cache « X contient %p » d'une entrée, invalidé par **un seul**
  helper appelé à chaque écriture de X (une quarantaine de sites).
- `is_high_dead_propagating` (947-958) : ajouter `Omul` / `Oshl` par
  constante, ce qui supprime le `lda #0 ; sta` après chaque `extuh`
  d'indice.
- Multiplication 32 bits par une petite constante en ligne quand la
  moitié haute est vivante (avant `emit.c:3260`) : ~60 cycles au lieu de
  ~250 pour `jsl tcc_mul32`.

### S5 — Cadres de pile et ABI (trois commits)

- **(a)** Un temporaire 32 bits « adresse seulement » se comporte comme
  un 16 bits : `is_narrow_kl()` relâche les tests `cls != Kl` dans
  `emitstore`, `mark_dead_stores`, `can_be_frameless` et supprime
  l'invalidation du cache A en fin d'addition. L'invariant des moitiés
  hautes reste le filet : une lecture haute d'un tel temporaire arrête
  la compilation.
- **(b)** `rep #$20` passe à la charge de l'appelant : le prologue ne
  l'émet plus (`emit.c:6092`). Audit des appels assembleur → C (`crt0`,
  `lib/source/*.asm`, `lib/contrib`), `devtools/check_asm_abi.py` étendu
  pour refuser un `jsl` vers un symbole C sans `rep #$20` dans le bloc,
  `compiler/ABI.md` et `KNOWN_LIMITATIONS.md` mis à jour.
- **(c)** Temporaires en page directe pour les fonctions **feuilles**
  (≤ 12 mots, `tcc__r0` à `tcc__r5h`) : plus de cadre, `lda dp` au lieu
  de `lda n,s`. Audit préalable : l'IRQ doit tourner sur sa propre page
  directe comme le fait la NMI ; le côté SA-1 aussi.

### S6 — Bibliothèque (classe B, après remesure)

- `padHeld` / `padPressed` / `padReleased` : la NMI met à zéro un port
  débranché une fois par trame, et les trois deviennent des lectures
  `static inline` dans `lib/include/snes/input.h`.
- `bgSetScroll`, `oamSetSize` : remesurer après S4 et S5 ; `static
  inline` ou assembleur seulement si encore derrière.
- Le bloc de 3 089 cycles de la NMI : le profiler, corriger ;
  `testing/nmi_budget.py` est la porte.

### S7 — Parité

- Module `string` en assembleur (`lib/source/string.asm`,
  `<snes/string.h>`) : `memcpy`, `memmove`, `memset`, `strlen`, `strcmp`,
  `strcpy`, `strncpy`, respectant les pointeurs lointains ; ligne
  `_DEP_string` dans `common.mk` ; le guide de migration cite une
  fonction qui existe.
- `setPaletteColor`, `oamGetX`, `oamGetY` ; jumeaux des exemples
  PVSnesLib sans équivalent (skill `port-example`).

## Étage 2 — viser le code d'un humain (après la 1.0)

Après l'étage 1, un backend sans allocateur atteint environ 100 à 120
cycles par tour de tri (PVSnesLib 255, humain 27). Le reste demande de
garder des valeurs dans X ou en page directe d'une instruction à l'autre.

| # | Étape | Jours |
|---|---|---|
| T1 | Écriture d'octet sans `pha/pla` | 1,5 |
| T2 | Réduction de force des variables d'induction (QBE) | 4 |
| T3 | Indice de boucle gardé dans X | 10 à 15 |
| T4 | Page directe comme banc de registres pour les fonctions non feuilles | 8 à 10 |

Un allocateur de registres classique sur A/X/Y n'est pas à planifier :
un seul accumulateur, deux registres d'adresse aux modes asymétriques.
T4 est l'« allocateur » qui convient à ce processeur.

## Vérification

Chaque étape de compilateur est de classe A (`.claude/rules/testing.md`),
sur une branche `wip/<étape>`, le commit de sous-module **ajouté à
l'index du dépôt principal avant tout `make`** :

1. Copier les ROM d'avant (`rsync` vers `/tmp/examples_before/`).
2. `make clean && make`, `make tests`.
3. `python3 testing/diff_corpus.py --ref /tmp/examples_before` : tout en
   MATCH ; un DIFF s'explique avant toute recapture.
4. `testing/difftest.py` et `testing/difftest_stmt.py --seeds 1-2000` ;
   1-10000 pour S2, S3 et S5.
5. `devtools/compiler-tests/run.py` avec les nouveaux `.checks` ;
   `make bench`, `make bench-sdk`, banc bibliothèque.
6. `make test-sanitizers`, `make test-toolchain-suites` (la suite QBE est
   l'oracle indépendant pour les passes génériques de S2 et S3).
7. Quelques exemples triés pour une vérification visuelle, puis fusion
   en un commit, `compiler/PINS.md` à jour.

**Références sensibles au temps** (oracle WRAM, empreintes audio) : elles
bougeront sur presque tout le corpus. Une recapture **par étape, à la
fin, dans un commit à part** ; le commit audio cite
`luna diff --audio` par exemple (MATCH à 2 %), le commit WRAM dit quelles
pages ont bougé et pourquoi.

**On s'arrête** si : un écart du test différentiel ne se réduit pas à un
motif nommé en une journée ; un DIFF du corpus reste inexpliqué ;
l'invariant des moitiés hautes ou le contrôle de propriété des
emplacements se déclenche (l'analyse est fausse, on ne contourne pas) ;
la suite amont de QBE régresse ; une étape dépasse le double de son
estimation.

## Fichiers principaux

- `compiler/qbe/gcm.c` — `sinkref`, balayage après `sink()`
- `compiler/qbe/w65816/emit.c` — `mark_far_decomp`, `mark_addr_only_kl`,
  `mark_dead_stores`, `can_be_frameless`, `emitstore`, `Oadd`/`Omul` 32
  bits, sites de lecture et d'écriture proches, `emitjmp`, prologue
- `make/common.mk` — ligne d'édition de liens, `_DEP_string`
- `devtools/sdkbench/`, nouveau `devtools/libbench/`
- `templates/crt0.asm` — page directe, modes d'entrée NMI/IRQ
- `lib/include/snes/input.h`, `lib/source/input.c`, nouveau
  `lib/source/string.asm`
- `docs/BENCHMARK.md`, `docs/PERF.md`, `compiler/ABI.md`,
  `docs/MIGRATING_FROM_PVSNESLIB.md`

## Corrections to commit messages

- `4d6624b9` (S4 part 2) says `superfx_game_skeleton` reports its audio
  DIFF "on the partial last window alone". Wrong: read window by window
  after the push, two complete windows are over the 2 % tolerance as well
  (2500 ms: 3.00 %, 4500 ms: 2.22 %), the others under 1.9 %. The cause is
  the one stated — the music starts one frame later (first sample above 64
  at 33344 -> 33872) — but the last window is not the only one that shows
  it. History is not rewritten; this line is the correction.
- `54f2bab3` (S5 step 3) says recursive `fib(17)` used 265 bytes of stack
  "two days ago". It was the day before, the morning of 2026-10-08.
- `477259cd` (S5 step 4) counts "180 000 programs clean on new seeds".
  The first 120 000 had one failure, seed 124152 — the gvn defect the same
  commit fixes; the 60 000 run after the fix were clean, and so is that seed.

## S7: PVSnesLib's examples against ours (mapped 2026-10-09)

PVSnesLib `fa758c9b` has 62 example directories; ours has 86. Read by name
and by what each demonstrates, not built side by side.

**Covered** (theirs -> ours): hello_world -> text/print_string; the
Backgrounds (Mode0, Mode1, Mode1BG3HighPriority, Mode1ContinuosScroll,
Mode1LZ77, Mode1MixedScroll, Mode3, Mode5, Mode7, Mode7Perspective); the
Effects (Fading, GradientColors, HDMAGradient, MosaicShading,
ParallaxScrolling, Transparency, TransparentWindow, Waves, Window); the
seven Sprites; input controller / mouse / superscope; maps DynamicMap,
mapscroll, slopemario, tiled; objects/mapandobjects; games breakout and
likemario; audio effects, music, musicGreaterThan32k; memory_mapping ->
memory/hirom_demo; sram -> memory/save_game; timer -> basics/timer.

**No twin yet, and one is owed** (to port with the `port-example` skill,
each with README, screenshot and a manifest):

| PVSnesLib | What it shows | Note |
|---|---|---|
| `random` | the generator | we have `rngNext()`; no example calls it for its own sake |
| `testregion` | NTSC / PAL detection | `getRegion()` / `isPAL()` exist, tested by `make test-pal` only |
| `typeconsole` | the text console's modes | to read before deciding what it maps to |
| `debug`, `breakpoints` | emulator messages and breakpoints | `debug.h` has both, no example |
| `objects/moveobjects`, `objects/nogravityobject` | the object engine without a map | ours only shows it inside `games/mapandobjects` |
| `maps/mapbuffer` | a map drawn from a RAM buffer | to read first |
| `audio/music2`, `audio/musicHiROM`, `audio/effectsandmusic`, `audio/tada` | SNESMOD variants | check what `soundboard`, `sfx_from_wav` and `hirom_demo` already cover |
| `input/mouse-data-test` | raw mouse data | probably covered by input/mouse; to check |
| `Mode1Png`, `Mode1Scroll` | a PNG background, a scrolled one | probably covered by backgrounds/mode1 and maps/map_scroll; to check |

**No twin, by decision**: `scoring` (BCD scores: non-goal),
`Palette/GetColors` (reading CGRAM back: not provided), `input/multiplay5`
(the multitap path cannot be armed, KNOWN_LIMITATIONS.md), the three
`logo/*` (two of them reproduce Capcom's and Konami's boot logos: not
ours to ship).

## Third bench: the twins, side by side (started 2026-10-09)

Owner's question, same day: port the missing examples first, to have a
one-to-one benchmark? Decided no: the missing ones do almost nothing per
frame, and about forty examples already have their twin. `devtools/twinbench`
(`make bench-twins`) runs each pair on luna with the same input script and
reports the master cycles of work per frame. The twins are ports, not the
same source: a pair is quoted only after its two sources were read side by
side.

First pass, 26 pairs: ahead on 17, behind on 9. Read so far:

| Pair | First pass | Verdict |
|---|---|---|
| mapandobjects, slopemario | +40.9 %, +39.6 % | real: the object engine copied each object six times a frame between its pool and the workspace. Workspace moved behind the pool, the four collision routines work on it in place: +17.4 %, +13.9 %. The two copies around each update callback remain; removing them needs callbacks that work in the slot (the compiler folds `o->field` on `&objbuffers[idx]` into `lda.l objbuffers+N,x`, tried) — an API addition, about a day |
| dynamicmetasprite | +75.2 % | real twins: `oamMetaDrawDyn` is compiled C here, assembly there. Next |
| dynamicsprite, metasprite, animatedsprite | +148 %, +977 %, +38 % | NOT twins (4 sprites against 1; redraw every frame against once; the anim module against a counter). Left out of the bench. The anim module costs about 5,000 mclk a frame for one sprite: to look at |
| dynamicmetasprite (2) | +75.2 % -> +24.9 % | `oamMetaDrawDyn` rewritten in assembly (77,656 -> 55,364; theirs 44,332). What is left: ours fills an oambuffer entry per item, then calls the draw routine of its size, which reads it back (about 2,700 mclk an item in the callee, 1,900 in the iterator); theirs draws each item in the iterator. Closing it means entry points in sprite_dynamic.asm that take the entry offset in X with the data bank already set, or a fused loop |
| mapscroll | +5.9 % | read: same work (`main` 3,100 against 3,015). The difference is our NMI handler on a frame where a sprite moved (7,670 against their 6,726 every frame) and the map's column upload (`_pvb1` 3,164 against 2,501). The handler is paid by every game: next after the engines |
| breakout | +2.4 % | says nothing yet: under the bench's input script theirs waits and ours plays. Left out until the script is right |
| mode7 | +0.9 % | to read |

## Issue #165: a real game on the SDK (2026-10-09, owner: this comes first)

A port in progress (`~/workspace/speedball2-snes`, its own repository and
its own clone of OpenSNES; read, never modified from here) reported that
placing 19 world-space sprites cost a third of its frame, and on the way
that a sprite and its background did not line up. Owner's decision the
same day: a real game finding things is worth more than our benches, so
this goes ahead of the twin bench's leftovers.

| Part of the issue | State |
|---|---|
| Sprites one line above the background | fixed, `caa73986`: the library stored y - 1 for sprites; the arbiter says only the background needs it |
| `oamPlaceWorld` (batch placement, camera, culling) | done, this commit: `lib/source/sprite_world.asm`, 12 words asserted in libtests, two `libbench` rows. 31,379 master cycles a frame against 63,783 for the same loop in C (19 sprites, 13 on screen, slow ROM): 1,650 a sprite, not the "few hundred" hoped for |
| A VRAM upload queue drained by one assembly routine | done, this commit: `<snes/vramqueue.h>`, `vramQueuePush` / `vramQueueFlush`, both assembly. Measured, and said so: it costs MORE in total than the calls it replaces (21,700 against 16,600 master cycles for six 128-byte transfers) and a third LESS in VBlank (10,700). The project's own version pushes with a C macro; compiled by us that macro costs three times the call (five indexed stores, the count reloaded each time) — a compiler finding to keep: an index used by several consecutive stores is rebuilt for each |
| Answer on the issue | not written: published under the owner's name, needs the owner's go |

The project could not be built here (its assets come from an original ROM),
so the figures are from a reconstruction of its match screen in `libbench`;
its own measurement, once it calls the routine, is the one that counts.

## Issue #166: the compiler on per-entity loops (2026-10-09, next in line)

From the same project. A loop over N entities reading parallel `u16` tables
compiles to 76 instructions per iteration (reproduced on qbe `58449ce` with
the issue's `place()`); about 35-50 would do. Six patterns, all visible in
the output; these were T-stage items ("after 1.0") and move up because a
game in C pays them on every entity, every frame. Plan posted on the issue
(comment of 2026-10-09), in this order, one commit each, full Class A
protocol each:

0. `place()` as a bench row first (instructions and master cycles per
   iteration, gated).
1. pattern 3: `lda.w sym,x` for a plain (non-FAR, non-const) object.
2. pattern 5: an inverted short branch when a safe upper bound of the
   distance fits (a wrong bound is refused by the assembler, never
   miscompiled).
3. pattern 2: X kept across every instruction that does not write X (today
   any non-indexed instruction drops it).
4. pattern 1: the index's dead high half and the reload of what A holds.
5. pattern 4: the store nobody reads on the loop increment.
6. pattern 6 (loop shape, scaled index kept in X) only after the five are
   measured: induction-variable work, days, the largest risk.

Also from it: "prefer u16 in hot loops" belongs in the docs.

Both issues were answered in writing on 2026-10-09 (owner's go), under the
owner's name: what is fixed, what is measured, what is not promised.

### What the project said back (2026-10-09, comments on #165 and #166)

It adopted all three pieces of #165 the same day (`41603464`); its game has
no assembly file of its own any more. Measured there, FastROM:
`oamPlaceWorld` 1,320 master cycles a sprite (ours: 1,652 in SlowROM); the
push function took its `spritesUpdate` from 78,112 to 64,285 against its
macro. Whole frame with 18 players walking: 70.2 % -> 65.4 %. Modest,
because the rest is game logic in C: 95,000 a tick for 18 players, 28,000
for a depth sort.

Asked for, and accepted (answered on the issues):

| Request | Where | State |
|---|---|---|
| `opensnes-sprite sheet --size 32 --metasprite 32 32`: every `METASPR_ITEM` has tile 0 | tools | fixed, `ed7903ed`: wrong on any sheet of several rows of metasprites, not only 32x32 |
| `vramQueuePushSprite(src, addr, size_px)`: the strips of one streamed frame in one call | lib | done, this commit; its gain is theirs to measure |
| `opensnes-sprite sheet --size 32 --flip`: mirrored 32x32 blocks not deduplicated | tools | done, this commit, as the author's second message specified it: `--compact` writes each distinct block once and `<stem>_blocks.inc` says where each block of the sheet went (stored block + flip bits). `--flip` alone only ever marked mirrors in the metasprite table |
| optional `const u8 *order` in `OamWorldBatch` (depth sort without a second copy of x and y) | lib | to do; its own path in the routine, cost of the indirection to be measured |
| pattern 7: `T tab[N][M]` const table, 59 instructions a read against 17 flattened | compiler | with `array2d_read`, same root |
| pattern 8: absolute difference and signed compare, 37 instructions (compare materialised as 0/1, `cmp #0` after an `sbc`) | compiler | to do |
| pattern 9: inline a `static` function with one call site | compiler | to size: the back end has no inliner; the largest lever and the largest piece |
| its insertion sort as a benchmark (97 instructions, 3,300 mclk an element when already sorted) | devtools | to add as written |

Withdrawn by its author, and corrected in our docs: "prefer u16 tables".
What costs is 8-bit arithmetic, comparison and `s8`; a `u8` table is no
slower to index.

Step 1 of #166 (pattern 3) on `place`: 7,417,820 -> 7,383,730 (-0.5 %).
The first version was wrong (FAR and const objects got the short form too:
17 pictures, two checks); it exposed that the link-time bank guard saw no
`static` symbol at all, in both its implementations — fixed. And that the
differential tests index no const or FAR table: to close.

Step 2 of #166 (pattern 5, short branches): `place` 7,383,730 -> 7,325,978,
523 -> 502 bytes; 76 -> 74 instructions an iteration (two of its three
conditionals; the loop exit is out of reach). Mostly a size gain: -33.9 %
against PVSnesLib over the 19 workloads. Its first version undercounted a
line that carries a label and an instruction, and difftest showed it as
link failures ("too large distance", 129 bytes), never as wrong results.
Two examples boot one frame sooner (mode4, mode7/extbg: MATCH at tolerance
2). play_noise's audio differs by 6 % in its first window with an onset two
samples earlier: noise, whose windows are not phase-stable.

Step 3 of #166 (patterns 1, 2, 4: a peephole over the emitted text):
`place` 7,325,978 -> 6,460,002 (-12.9 % since the issue opened), 74 -> 66
instructions an iteration; `near` 37 -> 32, `sort` 95 -> 80, 2D lookup
59 -> 51. 19 workloads: -47.5 % cycles, -35.8 % size against PVSnesLib.
Corpus 86/86 at tolerance 0; hunt 66001-70000 and 167001-173000 clean.
The hunt found a defect of step 2 (a backward bound that did not count the
branches already shortened: link failure at 129 bytes), fixed with this
step. What the listing still shows, for the next steps: the load of a
table then an operation on it (`lda.w gx,x / sta / lda.w xs,x / sta / lda /
sbc`) where `sbc.w xs,x` would do; `cmp.w #0` after an operation that set
the flags; the comparison result materialised as 0 or 1 (pattern 8); the
loop counter in memory with the test at the top (pattern 6).

### Second round of answers (2026-10-10 06:55, both issues)

On the real game at `a95512f6`, source unchanged: 18 players walking
65.4 % -> 57.9 % of the frame, walking + scrolling 71.1 % -> 63.8 %.
Per function, none a leaf: `movePlayer` -21.5 %, `sortPlayers` -28.4 %,
`animate` -16.4 %, `stepPlayer` (three calls) -7.9 %, `spritesUpdate`
-6.6 %. So step 3 gives more on non-leaf code than on `place` (-12.9 %)
where the body is array accesses, and little where it is calls: pattern 9
seen from the measurement side. No build failure, the stricter bank guard
says OK, same frame hashes.

`vramQueuePushSprite` adopted: its `sendFrame` 12,973 -> 3,901 master
cycles a frame. Logic of the tick, for the choice of what comes next:
`movePlayer` 26,304, `animate` 14,734, `stepPlayer` 12,841, `control`
12,154 (runs one tick in eight: mostly entry and exit), `moving` 8,220.

Step 4 of #166 (two peephole rules from the real game's output: `cmp.w #0`
after the flags are set; a direct-page slot nobody reads). Two silent
mistakes on the way, both found by the differential tests before anything
was pushed: the compare's carry read two lines on (six programs of the
gate), then the reload rule removing the load the compare rule had relied
on (one program in 100,000; seed 179180, now in the gate). Final hunt:
76001-84000 and 173001-193000 clean. `place` unchanged; real functions
`movePlayer` 184 -> 175, `animate` 79 -> 69. `dist` added as the twentieth
workload: 3,665 master cycles a distance.

The project's own framing of "done" for #166 (2026-10-10): kick-off scene
under 40 % of the frame (56 % at `d235abe9`). Its 1,750-per-distance
threshold is withdrawn (it found an exact pre-reject: 1,100 a pair). #165
is closed by its author; `order` in the batch is #167, not urgent, three
design points answered there (visible[] by sprite; an out-of-range entry
hides the slot; one routine only if the identity path costs nothing).
A hand-made working copy of the player's fields did NOT pay there (+3.5
points): the cost left is the index per block and the calls.

### State at `a66a4f53` (2026-10-10, CI green on every leg)

The project's figures with step 4: kick-off 50.9 % of the frame (52.4 %
before the step; its own source work took it from 56 to 52.4), one player
running with collisions 33.0 %. `movePlayer` -7 %, `animate` -13 %,
`stepPlayer` -4 %. The AI is transcribed and nothing reads the 81 distances
every tick: the matrix is off the list. One threshold left for #166:
**kick-off under 40 %**, the per-player path.

What that path is made of, counted on a copy of its `match.c` (dummy
tables, instructions): `movePlayer` 175 with 19 `tax` (9 of them after an
`asl a`), `head` 196 with 20, `control` 148 with 13 and 3 calls, `animate`
69 with 9, `moving` 115 with 4 calls, `matchTick` 62 with 7 calls. Read on
the output:

- the IR has `extuh id / mul 2` once per block (the front end reloads `id`
  from its alloc in each block; after mem2reg and gvn, `sink` puts the
  cheap computation back next to each use). After the peephole that is
  `lda id / asl a / [sta slot] / tax` per block: 3 or 4 instructions, not
  the 9 of before. Keeping it in a slot would save one instruction a block;
  keeping it in X ACROSS blocks would save all of them, and X is also
  wanted for the other indices (`anim_data[...]`). `lda.w sym,y` exists for
  plain RAM (not `long,y`): a loop-invariant index in Y is the option to
  size;
- `sta slot` of the scaled index in each block, never read again;
- `jmp` to a block that is only a `jmp`;
- a value reloaded right after the conditional branch that tested it, when
  the fall-through label has no other way in.

Candidates for the next step, to be chosen by what they do to `movePlayer`
and `dist`: (a) a forward dataflow over the function's text for "X holds
slot S" across labels (join = equal, loops to a fixed point), the natural
extension of the peephole; (b) Y for an index that does not change in the
function; (c) inlining a single-call-site static (`stepPlayer`, `control`,
`moving`, `matchTick` are calls).

### Owed to others

- #164 (DiscoC, external contributor): answered on 2026-10-10. Said in the
  owner's name, without a date: the layout of bank $70 will be written down
  and posted on that issue. Also named as ours: a helper that copies a GSU
  program to Game Pak RAM, and a way to pin a GSU binary at a fixed ROM
  address. After the port's issues.
- #167: `order` in `OamWorldBatch`; design answered there; after #166's
  next step.

### Step 5 of #166: whole-function inlining (2026-10-10), and what the real game said

Delivered (`inline.c`, `splice_fn`): a static function with one call site
is inlined whole and not emitted; `static inline` with several sites is
copied at each (160 IR instructions). Rules and declines in
`.claude/rules/compiler.md`.

The owner built the game project for us (`~/workspace/speedball2-snes`,
read only; we build copies of `game/` in the scratchpad with
`make OPENSNES=<our tree> [CC=<another bin>/cc65816]`). First time a step
is measured on the game itself and not on a copy of one file. Method: its
own `test/play.toml` input script, `luna profile --from-frame 0
--until-frame 760 --top 3000`, sum of master cycles of the symbols of
`match.c` (`*.match`, `match*`). NOT the busy share of a window: the game
catches up lost ticks, so a faster build does more work in a given window
(frames 320-430 read +4 points on a build that is faster overall).

| Build | `match.c` | all busy | ticks run (of the script) |
|---|---|---|---|
| inliner off | 99.00 M | 167.39 M | 347 |
| every single-site static absorbed | 99.26 M | 171.16 M | 344 |
| + leaf rule (final) | 98.61 M | 166.79 M | 348 |
| final, `inline` on `vectorLength` and `octantTo` | 94.78 M | 163.56 M | 349 |

What it taught, against what I had announced ("(c) is the only lever on
the functions made of calls"):

- the time is in the bodies, not in the calls: -0.4 % as written;
- pouring a leaf into a non-leaf LOSES (the direct-page frame and the
  peephole's rules on its slots are worth more than a call): hence the
  leaf rule, by size, and its refinement (a caller whose every call can be
  absorbed becomes a leaf: that is where -4.3 % comes from, `selectActive`,
  the heaviest non-leaf, whose only calls are two `vectorLength`);
- **59 % of the logic's time is in non-leaf functions, on stack frames**:
  `selectActive` has 145 stack-relative instructions of 250, `collide` 110
  of 241, `localInteraction` 151 of 364. That is the next step, and it is
  T4 of the plan in a small form: a temporary that is NOT live across a
  call can live in the direct page (`tcc__lf`) even in a function that
  calls — a leaf callee clobbers that page, which does not matter to a
  value dead at the call. Needs a liveness of slots across call sites in
  `emit.c`; the stack keeps what crosses a call.

Hunt for this step: programs 193001-217400, expressions 84001-89000, on
new ranges, clean. The generator now makes odd seeds static (4k+1) or
static inline (4k+3). Audio: unchanged with the final rule. WRAM: 11
examples re-captured (their inlined functions use other temporaries;
pictures identical on 86).

### Step 6 of #166: direct-page temps in functions that call (2026-10-10)

Delivered (`color_slots` in `emit.c`): a temp no call crosses gets a slot
in `tcc__lf` (16 words, first fit), the others the stack. Rules in
`.claude/rules/compiler.md`.

- the game (method in `.claude/notes/projects/speedball2.md`): 98.6 M ->
  93.6 M in its logic (-5.4 % against `a66a4f53`), 92.1 M with its two
  `inline`; three lost ticks fewer on its script; 370 bytes less code;
- sdkbench: -50.8 % -> -52.9 % cycles, -37.3 % -> -38.4 % size, stack no
  deeper on 19 of 20; libbench 13 rows of 13 (`oamxy`, `dma` ahead again,
  `text` -6.8 %); static table unchanged (1270: its functions are leaves);
- hunt: programs 217401-237400, expressions 89001-93000, clean;
- four free-running examples one frame early (boot shorter): pictures and
  three manifests re-captured after `diff_corpus --tolerance 2` and
  `frame_sequence.py` said "same pictures, offset -1".

Tried and not kept: a size limit on what is absorbed into a caller that
keeps calling (16, 40, 80, 160 IR instructions): the game's logic moved by
+-0.3 % either way, within what its tick catch-up moves. Inlining a
non-leaf into a non-leaf is worth its bytes (about 190 saved), not time.

Found on the way, not a compiler defect: `audioLoadSample` raced on its
end mark (`silent_defects_log.md`, 2026-10-10). The test had passed by
phase since 2026-10-03; the faster loop exposed it. Lesson for the
protocol side of the library: a value one side waits for must be HELD by
the other until answered; and a wait counted in loop turns is a wait whose
length the compiler decides.

What is left in the game's heavy functions after this step: `matchDecide`
still has 222 stack-relative instructions of about 2,300 (values that do
cross a call, and the 16 words taken first-come); `spritesUpdate` and
`movePlayers` are next to read. Candidates: choose which temps get the 16
words by use count instead of definition order; more words (the block is
32 bytes; the NMI page mirrors up to $A0).

### Step 7 of #166: the scaled index computed once (2026-10-10)

Read in the game's `movePlayers` (a loop over eighteen players and a dozen
parallel arrays): `lda i / asl a / tax` in front of EVERY access, twelve
times per iteration. Cause, in the IR after gcm: `sink()` copies the
address of a load or store next to it (wanted: the emitter folds it into
the addressing mode) and, since our fix of 2026-10-08 made its recursion
real, the address's operands too — so each access had its own `mul i, 2`.
Upstream's recursion acts on a local copy and never reaches the emitted
instruction; we had repaired a bug into a pessimisation. `sinkref` no
longer recurses (`QBE_SINK_DEEP=1` restores it).

- the game, two builds of its sources of the day, its `measure.sh`:
  match.c 75.12 M -> 68.28 M master cycles over the same 340 ticks
  (-9.1 %), three-frame ticks 9 -> 4; the game measured the same on its
  working tree (75.43 -> 68.59) with its nine tests and its reference
  model in agreement at every tick;
- sdkbench: -52.9 % -> -53.9 % cycles, -38.4 % -> -39.8 % size; physics,
  collide, sort, entities, dist move, nothing regresses; static table
  unchanged (1270);
- corpus: 86/86 MATCH at offset 0; nine WRAM streams re-captured (direct
  page temps renumbered);
- the work was done in a separate worktree (`~/workspace/opensnes-s8`,
  submodules as worktrees of the main tree's), the game given the binary in
  `~/workspace/opensnes-s8-bin` meanwhile: the first chantier run under
  `exchanges.md`.

The hunt on a new range found a defect that was not this change's
(`silent_defects_log.md`, 2026-10-10: a `(u8)` cast dropped after a signed
shift in a loop, inherited from upstream `copy.c`); fixed in the same lot.

The benchmark gains a workload written by the game for us, `depot`
(eighteen agents, bursts of sixteen decisions, nothing from the game in
it), which reacts to this step like the game's own code (-5.0 % against
-5.5 %).

Next candidates read in the same listing: `lda.b S / tax` before a load is
`ldx.b S` (2 cycles each); a phi copy `lda S / sta T` at each loop turn;
`bne L1 / jmp L2 / L1: jmp L3` (a branch to a jump).

How the separate worktree was made (the recipe, since `git submodule
update` in a new worktree would fetch from the network):

    git worktree add ../opensnes-s8 -b wip/s8-sink develop
    for m in cproc qbe wla-dx; do rmdir ../opensnes-s8/compiler/$m
      git -C compiler/$m worktree add [--detach] ../opensnes-s8/compiler/$m <branch|HEAD>
    done
    cp -a testing/bin/. ../opensnes-s8/testing/bin/     # luna

Each submodule checkout there is a linked worktree of the main tree's
submodule repository, so a commit made there is already in it; landing is
`git merge --ff-only` here, then `git -C compiler/qbe checkout --detach
<sha>`, then the fork push and the rebuild. Two traps met: git printed
"cannot chdir to …/opensnes-s8/compiler/qbe" from the main tree while the
linked worktrees existed, and a `$(git -C … rev-parse HEAD)` came back
empty, so the fork push did nothing — check `git ls-remote` before
pushing develop; and `make test-toolchain-suites` chained right after
`make test-sanitizers` runs on its half-cleaned tree and fails everything
(rebuild first). Removed the same day: the three submodule worktrees, the
worktree, both branches.

### Step 8 of #166: a phi shares the slot of what feeds it (2026-10-10)

Counted in the game's assembly after step 7: 297 copies from one
direct-page slot to another, most of them a loop variable written back at
the end of each turn. The slot colouring gave a phi and each of its
arguments different slots by construction: a phi result interfered with
everything live at the end of each predecessor, its own argument included,
and a result with its own operands.

What changed in `color_slots` (emit.c):
- a phi's own argument on an edge is no longer an interference by itself
  (any other reason still is: the argument live into the block, the old
  value read after the new one exists — `while (j--)`);
- a 16-bit add / sub / and / or / xor / copy may take its operand's slot
  (the emitter computes these in A and stores once);
- an argument asks for its phi's slot, or a sibling argument's when the
  phi is not placed yet; a phi asks for an argument's;
- two 32-bit values share exactly or not at all. The first version let
  first fit put one a word over the other, since nothing separated them
  any more: 233 difftest seeds stopped on the slot-ownership check (loud,
  never a wrong result), which is the net doing its job.
`emitphimoves` already skipped a move between equal slots.

- the game, develop 551787e9 against this: match.c 68.59 M -> 64.16 M
  (-6.5 %), everything but the wait 117.84 M -> 108.72 M (-7.7 %),
  three-frame ticks 4 -> 1; the game replayed it: reference model in
  agreement at every tick (946 + 496 + six provoked states), no internal
  error;
- four free-running examples show the same pictures earlier (mode2,
  fix32_orbit, dsp1_cube one frame; mode7/extbg seven: `diff_corpus
  --tolerance 3` and its sequence pass), three manifests indexed by frame
  moved with them. These three (and the three of step 6) should name a
  moment, not a frame: `at_symbol` (debt, still open);
- audio: nine hashes moved; seven MATCH under `--align-onset`; `echo`
  (21.8 %) and `speech_synth` (2.6 %) read DIFF and are the same signal 2
  and 16 samples apart, measured sample against sample (the limit is noted
  for luna in `OPEN_luna.md`);
- tried and dropped: taking the phi interference from the block's live-in
  only instead of each predecessor's live-out (the moves are on the edge,
  so it would be sound): 3 instructions of 13,000 on the game.

The hunt found one more defect that was not this step's: a constant shift
count narrowed by a cast (`silent_defects_log.md`). Two inherited silent
defects in one day from ranges nobody had run: the hunt on NEW seeds at
each Class A step is worth more than the gate's fixed ones.

The worktree recipe's trap, understood the second time (2026-10-10): a
submodule checkout made with `git -C compiler/qbe worktree add` shares the
submodule's `config`, and as soon as git runs in the new superproject
worktree (`git add compiler/qbe`, `git status`) it rewrites
`core.worktree` in that shared config to point at the NEW checkout — with
a relative path that only resolves from the linked git directory. From
then on every git command in the main tree's `compiler/qbe` and
`compiler/wla-dx` dies with "cannot chdir to …/opensnes-s9/compiler/qbe"
(`cproc` has its git directory elsewhere and is spared). Landing
therefore goes: read the commit with `git --git-dir=.git/modules/compiler/qbe
rev-parse wip/<name>`, remove the linked worktrees, then
`git config -f .git/modules/compiler/<m>/config core.worktree
../../../../compiler/<m>` for qbe and wla-dx, and only then checkout,
push the fork (`git ls-remote` to see it arrived) and rebuild. The game
builds with `bin/` and never runs git here, so it did not see it; a
session that had run `git status` in this tree during the chantier would
have. Next time: a plain clone of the three submodules into the worktree
(`git clone compiler/qbe ../opensnes-<name>/compiler/qbe`) costs a few
seconds and shares nothing.

### Step 9 of #166: a late peephole pass (2026-10-10)

Counted in the game's assembly after step 8: 182 `lda.b S / tax` in front
of a load, 71 stores of 0 through A, 21 counters through `lda / inc a /
sta`. `peephole_late` (emit.c) rewrites them once the ordinary peephole
has settled: `ldx.b S`, `stz`, `inc.b S` / `dec.b S`, and a constant
stored to several places is loaded once. 16-bit accumulator only (the
width is known line by line: an 8-bit section never crosses a label or a
branch — 5,348 sections checked over the corpus), direct-page slots only
for ldx and inc (no stack-relative form).

- the game: match.c 64.16 M -> 62.22 M (-3.0 %), everything but the wait
  108.72 M -> 106.50 M;
- the first version ran the rules INSIDE the peephole loop and the game
  went 2 % slower with fewer instructions: a `stz` to a slot written and
  never read (the high half of a widened index) was not a `sta` any more,
  so neither dead-store rule removed it, and the `sta S / lda S` behind it
  stayed. A new form is put in after the rules that do not know it;
- mode7/extbg one frame earlier again (the fourth time that day): its
  manifest now asserts a range for `ball_x` instead of a value. The other
  frame-indexed manifests are the debt (`at_symbol`, `input_at`).

Session totals on the game, same 340 ticks: 75.43 M (morning) -> 68.59
(step 7) -> 64.16 (step 8) -> 62.22 (step 9), -17.5 %.


The worktree recipe that works (step 9, 2026-10-10): plain clones.

    git worktree add ../opensnes-<name> -b wip/<name> develop
    for m in cproc qbe wla-dx; do rmdir ../opensnes-<name>/compiler/$m
      git clone -q compiler/$m ../opensnes-<name>/compiler/$m
      git -C ../opensnes-<name>/compiler/$m checkout -q --detach $(git -C compiler/$m rev-parse HEAD)
    done
    cp -a testing/bin/. ../opensnes-<name>/testing/bin/

Landing: `git -C compiler/qbe fetch ../opensnes-<name>/compiler/qbe <sha>`,
checkout that sha here, `git merge --ff-only wip/<name>` (rebase the branch
first if `develop` took note commits meanwhile), push the fork, `git
ls-remote` to see it, remove the worktree, rebuild. Nothing in the main
tree's git was disturbed during the chantier.
