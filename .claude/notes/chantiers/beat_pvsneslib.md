# Chantier — ahead of PVSnesLib on every counter

**Opened 2026-10-08, plan approved by the owner the same day.** The plan
below is the one he approved (in French, as written); this header is the
live part.

## Status

| Stage | State | Commit | Figures after it |
|---|---|---|---|
| S0 measurement | done except the library bench (`devtools/libbench`, to do before S6) | `d53f8605` | faster on 15 of 18, no larger on 13, no deeper in stack on 0 |
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
| S6 library hot paths | started 2026-10-09: the device readers are optional (pointers set by `mouseInit()` / `scopeInit()`): minimal ROM 2.4 KB -> 1.9 KB of bank $00, under PVSnesLib's 2.0 KB (the remainder S1 had left). 86/86 MATCH, audio unchanged. Decided for S5 the same day: a 32-bit load through a plain pointer keeps `lda [tcc__r9]`. It reads the pointer's bank where the 8- and 16-bit loads read bank $00; aligning it on them made `list` faster when tried on 2026-10-08 (-38 % against -20 % that day) but would break any code that reads a long through a pointer to another bank, which works today — not worth one workload. Still to do: `devtools/libbench/` (both SDKs, library calls), pads as `static inline`, `bgSetScroll` / `oamSetSize`, the 3089-mclk NMI block | `59be2a72` | minimal ROM 1.9 KB (PVSnesLib 2.0 KB) |
| S7 parity (string module, small gaps) | started 2026-10-09: module `string` (`memcpy`, `memmove`, `memset`, `strlen`, `strcmp`, `strcpy`, `strncpy`, assembly, 24-bit on both sides, one section per function, 24 assertions in libtests). Found with it: octal escapes in string literals (silent, old, fixed in the commit before). Still to do: `setPaletteColor`, `oamGetX`, `oamGetY`; the example twins | this commit | the migration guide's `memcpy` exists |

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

