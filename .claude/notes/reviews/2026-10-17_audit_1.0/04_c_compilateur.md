# 04 — C et compilateurs (cproc, QBE w65816, WLA-DX, ABI)

Expert 04 (C et compilateurs) · 2026-10-17 · HEAD audité `develop` @ `73a3fdfe` (worktree `/home/kobenairb/workspace/opensnes-audit`, sous-modules cproc `8c97293`, qbe `d1ad402`, wla-dx `7382e4c8`) · luna v1.38.0.

Sondes de l'audit : `/tmp/claude-1000/a04/` (`p1`…`p5` = ROM de sonde construites contre le worktree et lues par `luna state --assert` ; `iso/` = compilations isolées ; `metrics.py` = balayage des `.c.asm`). Le C minimal de chaque constat est recopié ci-dessous pour qu'il reste rejouable une fois `/tmp` vidé.

## Périmètre couvert

- **Lu** : `CLAUDE.md`, `PHILOSOPHY.md`, les règles chargées, `compiler/PINS.md`, `compiler/ABI.md` en entier sur les points d'ABI, `docs/STABILITY.md`, la porte 1.0 de `ROADMAP.md`, `docs/BENCHMARK.md`, les entrées compilateur de `KNOWN_LIMITATIONS.md`, `status/silent_defects_log.md`, `devtools/compiler-tests/README.md`, le rapport A du 03/10 et `2026-10-04_reste_a_traiter.md`. Code lu : `cproc/qbe.c` (`funccopy`, `calcvla`/`funcalloc`), `qbe/copy.c` (`defwidthle`), `qbe/w65816/emit.c` (`Oalloc*`), `cc65816` (préprocesseur), `templates/crt0.asm` (page directe du NMI, IRQ).
- **Lancé** : `python3 testing/difftest.py --seeds 1-500` (500/500, 12 000 expressions), `difftest_stmt.py --seeds 1-500` (500/500, 5 000 programmes), une fenêtre de chasse jamais parcourue, `--seeds 500001-501000` (1 000/1 000, 10 000 programmes), deux passes avec les interrupteurs d'optimisation coupés (150 graines chacune), `make test-compiler` (127/127, 16/16 refus), `devtools/toolchain_suites.py`, `make bench`, `make lint-docs`, `verify_toolchain.py`, `check_cproc_widths.py`.
- **Sondé sur luna** : 5 ROM, 35 assertions. Elles couvrent les VLA, `_Bool`, les copies de struct (3×`u8`, `u32`, depuis une donnée `const` nommée ou désignée par pointeur, `volatile`), `FAR ++`, les champs de bits `FAR`, `= {0}` sur un `u32[]`, les index bornés repliés (`reassoc`), le pointeur far en boucle (`r9loop`) avec appel dans le corps, les champs de bits `unsigned long`, les littéraux composés, les membres flexibles, les initialiseurs désignés imbriqués et le modulo signé.
- **Mesuré** : la qualité du code sur les 95 `.c.asm` des exemples (35 830 instructions), `boardGetCell` (tetris), les frames, les appels au runtime, et la distance à l'amont des trois forks (refs `upstream/*` de l'arbre principal, lecture seule).
- **Non lancé** : `make test-sanitizers`, parce qu'il remplace `bin/` par des binaires instrumentés pendant que d'autres experts construisent dans le même worktree. `make coverage-host` non plus : `llvm-profdata`/`llvm-cov` sont absents (`make -n coverage-host` → « not found »).

## Points forts

1. **Le compilateur passe sa propre batterie et une chasse fraîche sans défaut.** `make test-compiler` donne « 127 passed, 0 failed … 0/0 unchecked ratchet; 16/16 refusals pinned » (6 refus épinglés au 03/10, 77 cas). `difftest` : 500/500 graines. `difftest_stmt` : 500/500 graines, puis 1 000/1 000 sur la fenêtre 500001-501000, que personne n'avait parcourue.
2. **Les quatre 🔴 du 03/10 sont corrigés et épinglés à l'exécution.** Ils sont dans la ROM `testing/fixtures/compiler/d_quals`. Ma sonde `p1` les rejoue indépendamment sur luna : `FAR ++` → 12, champs de bits `FAR` → 82, `u32 arr[4] = {0}` après une pile salie à `0xAA` → 5, `lb = la` avec un `u32` → `0x11223344`/7. Tout est correct.
3. **Les refus sont devenus bruyants et précis.** `float` est refusé (« uses floating-point arithmetic … use the fixed-point types »), `long long` aussi (« the widest integer is long, 32 bits »), de même que les structs en paramètre ou en retour, les variadiques, `__asm__`, `_Complex` et la copie d'un objet `FAR` entier. Les 16 refus sont épinglés dans `cases/negative/`.
4. **Un garde-fou existe désormais pour la classe « largeur codée en dur ».** `devtools/check_cproc_widths.py` est appelé par `make lint` (`Makefile:128`) et répond « cproc-widths: OK (19 known site(s)) ».
5. **L'invariant Kl fait son travail.** Le VLA qui contient une boucle (`iso/vla.c`) s'arrête sur « internal compiler error: the high word … is read … but no instruction wrote it » au lieu de produire du code faux.
6. **Les optimisations récentes résistent à la sonde ciblée et à leurs propres interrupteurs.** Sur `p4`, 6/6 valeurs sont justes sur luna : pointeur `FAR` parcouru avec un `jsl` dans le corps, `*p++` avec appel, `d[k] |= s[k]` de ROM vers `FAR`, `b[i + 8]` sur `u8` borné. `difftest_stmt --seeds 1-150` passe avec `QBE_NO_PEEPHOLE=1`, puis avec `QBE_NO_AUTO_INLINE=1 QBE_NO_REASSOC=1 QBE_NO_IVAR=1 QBE_NO_DP_TEMPS=1` : 150/150 les deux fois. J'ai vérifié au préalable que ces variables changent bien le code émis : 41 et 249 lignes de diff sur `p1/main.c`.
7. **Le générateur différentiel couvre la mémoire qui compte ici** : tableaux `FAR` tirés une fois sur deux (`difftest_stmt.py:692`, `:1014-1015`), tables `static const` en banque d'assets (`:1018`), pointeurs vers `FAR` (`:1063`).
8. **Les pins sont honnêtes.** `verify_toolchain.py` → « OK: toolchain pins match (3 submodule(s) verified) ». Les en-têtes de `PINS.md` correspondent à `git rev-list --count` : QBE 134 (`77fe846..HEAD`), cproc 46 (`7051114..HEAD`), wla-dx 5 (`--not upstream/master`).
9. **Les suites amont sont tenues par un ratchet.** cproc donne 56 pass et 115 échecs attendus, QBE 55/0, wla-dx 30 et 2 attendus, avec 0 régression et 0 XPASS.
10. **Les frames de pile directe ne craignent pas le NMI.** Le NMI travaille sur sa propre page, qui couvre `tcc__lf` (`crt0.asm:131-137`, `tcc__nmi_registers dsb 160`). Les IRQ sont réservées à l'assembleur et c'est documenté (`crt0.asm:2063-2071`).
11. **Le benchmark publié correspond à la mesure.** `make bench` → « 34 functions, 1238 total cycles (baseline 1238), no regression », soit −37,5 % contre PVSnesLib + 816-opt (1981).
12. **Plus aucune paire `sta N,s / lda N,s` dans les exemples** (0 sur 95 fichiers ; 167 au 03/10, sur un ensemble plus large). Les frames des 108 fonctions qui en ont une : médiane 10 octets, p90 26, max 262.

## Points faibles

**04-1 🔴 Un tableau à longueur variable (VLA) compile sans un mot et produit un programme faux.** Effort : S.
- Preuve d'exécution. Sonde `p1`, sur luna v1.38.0 :
  ```c
  static u16 fill(u16 *p, u16 n) { u16 k, s = 0; for (k = 0; k < n; k++) p[k] = k + 1;
                                   for (k = 0; k < n; k++) s += p[k]; return s; }
  u16 vla(u16 n) { u16 a[n]; return fill(a, n); }   /* vla(6) doit valoir 21 */
  ```
  `luna state -n 2000000 --assert r_vla=…` donne **`8586`** (0x8685) au lieu de `1500`.
- Preuve dans le code émis (`iso/vla2.c`, `void f(int n){int a[n]; sink(a);}`) : la taille `n*2` est calculée dans `tcc__lf+0/+6` puis abandonnée, et le « tableau » devient `tsa / clc / adc.w #2`. C'est un pointeur dans la frame fixe de 4 octets, et `sink` écrit par-dessus l'adresse de retour dès que `n > 1`.
- Cause : `compiler/qbe/w65816/emit.c:6058-6075` traite `Oalloc4/8/16` comme une allocation de taille fixe (`getallocslot`) sans regarder l'opérande taille. Côté cproc, `calcvla()` (`cproc/qbe.c:443`, appelé à `:586`) produit volontiers l'allocation dynamique. `__STDC_NO_VLA__` n'est pas défini (voir 04-5). Rien n'en parle dans `KNOWN_LIMITATIONS.md` ni dans `ABI.md` : `ABI.md:496` dit « `alloca` … not supported », et `alloca` n'est pas du C.
- Conséquences : corruption de pile silencieuse sur une construction C11 ordinaire, d'autant plus tentante sur une machine sans tas. C'est le premier défaut silencieux du compilateur depuis le 2026-10-11 : il relance la quinzaine du critère 7 (`silent_defects_log.md`). Surtout, `docs/STABILITY.md:50-52` promet que le C accepté « grows … never the other way ». Refuser le VLA après la 1.0 violerait cette promesse, il faut donc le refuser **avant** le gel.

**04-2 🟠 Affecter une struct depuis un objet `const` nommé la lit en banque $00.** Effort : S.
- Preuve (`iso/kc.c`) :
  ```c
  struct L { unsigned long v; unsigned k; unsigned char c; };
  static const struct L kl = { 0x11223344, 0x5566, 0x77 };
  struct L r; unsigned s;
  void a(void){ s = kl.k; }   /* lda.l kl.kc+4   : juste */
  void b(void){ r = kl; }     /* lda.w kl.kc ... : faux, la donnée est en banque 7 */
  ```
  Même chose pour `r_b = ktab[i]` et pour l'union `ru = ku`. Sur luna (`p2`) : `r_a` attendu `4433221166557700`, lu `008db1014dbb012d`.
- Cause : `funccopy` (`compiler/cproc/qbe.c:666-697`) émet ses `load`/`store` sans aucun drapeau d'accès : ni `farram`, ni `const`, ni `volat`. C'est la classe « qualificatif perdu » du 03/10 (PF3), dans une forme que le correctif `a1474c4` n'a pas couverte. La copie depuis un *pointeur* `const` est juste (`p3`, `[tcc__r9]`). La copie d'une struct `volatile` perd `volat` ; c'est latent aujourd'hui, les 4 stores de `vs = ps; vs = ps;` sont conservés.
- Ce qui le rend 🟠 et non 🔴 : `opensnes-rom check` arrête l'édition de liens (« C code reads bank $01+ data with bank-$00 addressing … `kl.main` »). Mais son conseil, « Keep C-read data in bank $00 or RAM », est inapplicable depuis #127.3 : une donnée `const` ne peut plus être en banque $00. Et le garde-fou ne tient qu'un seul `make` (04-3). `d_quals` n'a pas de cellule « copie depuis un `const` nommé ».

**04-3 🟠 (→ expert build) Le filet de sécurité du compilateur ne tient qu'un `make`.** Effort : S.
- Preuve : `cd p2 && rm -f p2.sfc && make` → « make: *** [.../make/common.mk:720: p2.sfc] Error 1 ». Un second `make` affiche « Built: p2.sfc … Size: 262144 bytes », code de sortie 0. La ROM fausse du premier passage reste sur le disque et devient la sortie officielle.
- Cause : la règle `$(TARGET)` (`make/common.mk:704-720`) écrit le `.sfc` avant les contrôles post-lien, et aucun `.DELETE_ON_ERROR` n'existe (`grep -n DELETE_ON_ERROR make/common.mk` : vide).
- Conséquence : tout ce que le compilateur délègue au contrôle de lien redevient silencieux au second `make`, qu'il s'agisse d'une lecture bank-blind (04-2, `check_bank_reads`), du budget RAM ou de la sentinelle data-init. C'est exactement la classe de défaut que la chaîne prétend fermer.

**04-4 🟠 `compiler/ABI.md`, que la 1.0 gèle (`STABILITY.md:29-32`), affirme encore cinq choses fausses.** Effort : S.
- `:51` et `:381` : « a `u8` is zero-extended … before pushing », « Always zero/sign-extended when passed as argument ». Faux aujourd'hui (`iso/u8arg.c`) : `asmfn((u8)g16)` et `asmfs((s8)g16)` donnent `lda.w g16 / pha`, et `d2(int x){ asmfn(x); }` donne `jml asmfn`. L'octet haut part tel quel. C'est le constat A n°7, inchangé.
- `:195-200` : « Larger structs … Returned via a hidden first pointer argument … the backend handles this transparently ». Faux, c'est refusé (« struct returns by value are not supported on w65816 », `negative/struct_return`). `:503` dit le contraire dans le même fichier.
- `:385` : `long long` « cproc still emits via the host-target lowering ». Faux, c'est refusé.
- `:386` : `float`/`double` « Software floats — extremely slow, used only by printf-style helpers ». Faux, c'est refusé, et il n'existe ni soft-float ni `printf`.
- `make lint-docs` passe (« OK: no doc drift detected ») : l'ancre 4 ne vérifie que les prototypes. Conséquence : un auteur d'assembleur qui suit `ABI.md` lit un `u8` en 16 bits et prend des déchets dans l'octet haut. Et la 1.0 gèlerait un contrat qui se contredit.

**04-5 🟡 Le préprocesseur n'a pas la sémantique de la cible.** Effort : S.
- `cc65816` appelle le `cc -E -undef -nostdinc` de l'hôte (`compiler/cc65816/*.c:374-395`). Sur cette machine (GCC 16.2.1), cela donne `__STDC_VERSION__` = **202311L**, alors que cproc est un frontal C11. La valeur dépend du compilateur de la machine de build. C'est la classe « une ROM qui dépend de la machine de build » du 2026-10-03, qui n'est pas entièrement fermée.
- `__STDC_HOSTED__` = 1 sur une cible freestanding.
- `__STDC_NO_VLA__` est absent (voir 04-1), et `__CHAR_BIT__` aussi.
- Aucun des en-têtes freestanding que C11 §4p6 exige n'est disponible : `stdint.h`, `stddef.h`, `stdbool.h`, `limits.h`, `stdarg.h`, `stdalign.h`, `stdnoreturn.h`, `assert.h`. Les dix `#include` testés échouent.
- C'est voulu (le `<stdint.h>` de l'hôte mentait sur les tailles), mais rien ne le dit dans `KNOWN_LIMITATIONS.md` ni dans `ABI.md`. Le signe de `char` (signé, mesuré : `(char)200 < 0` → 1 sur `p5`) n'est documenté nulle part non plus.

**04-6 🟡 Le lint clang de l'hôte raisonne toujours avec un `int` de 32 bits** (constat A n°8, inchangé). Effort : S-M.
- `CLANG_LINT_FLAGS` (`make/common.mk:564-568`) n'a pas de `--target`.
- `int f(s16 a){ return a == 0xFFFF; }` → « error: result of comparison of constant 65535 … is always false [-Werror] ». C'est faux sur la cible, où la constante est `unsigned int` et la comparaison vraie pour −1. Le build de l'utilisateur est bloqué.
- `u16*u16 > 70000`, toujours faux sur la cible, passe sans un mot.

**04-7 🟡 Le code des comparaisons reste lourd** (constat A PF6, partiel). Effort : M.
- `boardGetCell` (`examples/games/tetris/board.c.asm`) consacre 7 instructions à chaque borne : `and.w #$00FF / sta / sec / sbc.w #24 / bvc / eor.w #$8000 / bpl`. Il s'agit d'une comparaison signée sur un `u8` étendu par des zéros, là où `cmp.w #24 / bcs` suffit. Le `u8` est remasqué après son rechargement (`lda.b tcc__lf+0 / and.w #$00FF`). La matérialisation 0/1 relevée le 03/10 a disparu.
- Sur les 95 `.c.asm` des exemples, il y a 342 comparaisons signées par `bvc @scmp`. **144** comparent un `u8` masqué à une constante positive, et sont donc évitables. On compte aussi 943 `and.w #$00FF` et 187 triplets `bxx + / jmp`.
- Une copie de struct passe par la page directe : 8 instructions pour 4 octets (`lda.w kl / sta.b lf+0 / lda.w kl+2 / sta.b lf+2 / lda.b lf+0 / sta.w r_a …`), là où 4 suffisent.
- Le runtime reste modeste : `__mul16` ×20, `__mod16` ×14, `__sdiv16` ×13, `__div16` ×7, `tcc_mul32` ×3 (`metrics.py`).

**04-8 🟠 Fork massif, vélocité forte juste avant le gel, distance à l'amont qui croît.** Effort : L (dette).
- **QBE.** 134 patchs locaux sur une racine écrasée (`77fe846`), donc `git merge-base` ne peut rien dire. `w65816/emit.c` compte **9 919 lignes**. Il y a eu **48** commits QBE depuis le 2026-10-01 et 10 depuis le 2026-10-12, dont cinq règles de peephole *textuelles* le 2026-10-16 (`b6d9571`, `f9c83c0`, `653b143`, `f3bb5bf`, `d1ad402`). Le backend lit **48** variables d'environnement (`getenv`), dont une trentaine d'interrupteurs d'optimisation. L'amont a 41 commits depuis la base `120f316`. `e01207f` (« fix exponential behavior of defwidthle() ») n'est toujours pas absorbé (`merge-base --is-ancestor` → non), et il touchera désormais `79ff312`, la correction locale de la même fonction. Le conflit est donc certain. Mes deux générateurs de chaînes `and/or/xor` et de phis (N = 26) ne l'ont pas déclenché : le risque reste théorique, mais l'absorption coûtera plus cher à chaque mois.
- **cproc.** 46 locaux, 97 commits amont absents.
- **wla-dx.** 5 locaux, 9 commits amont en avance, bloqués par la régression SPAN.
- Conséquence : le critère 7 (« deux semaines sans défaut silencieux ») est mesuré sur une cible qui bouge tous les jours. Chaque règle de peephole ajoutée est une surface que seul le différentiel garde.

**04-9 🟡 Chaque optimisation n'a qu'une preuve de forme ; sa correction repose sur le différentiel global.** Effort : M.
- Chaque optimisation récente a un `.checks` qui épingle sa forme (`reassoc_bounded_index`, `r9_loop_pointer`, `r9_across_index`, `peep_flow`, `peep_late`, `phi_share`, `dp_temps`, `ivar_scaled_counter`, `fuse_indexed*`, `x_across_blocks`, `loop_x_counter_home`, `auto_inline*`), et souvent une variante `_off`. La correction, elle, ne tient qu'à `difftest`/`difftest_stmt` (porte + chasse nocturne) et à l'A/B du corpus. Aucune ROM d'exécution n'est dédiée à une optimisation donnée.
- Les configurations « interrupteur coupé » ne sont pas dans la CI. Je les ai lancées : propres sur 150 graines, mais c'est un instrument de bissection qui n'a pas de garde.
- `reassoc` (`f3bb5bf`) repose sur une « owner decision » : ne jamais exploiter l'indéfini d'un accès hors bornes. C'est la bonne décision, mais aucune fixture négative ne prouve qu'un index non borné reste non replié.

**04-10 🟡 Dérives de documentation dans le périmètre.** Effort : S.
- `docs/BENCHMARK.md:31` dit « It loses on code that goes through pointers ». Le tableau dit l'inverse : `array_read` −21,7 %, `struct_sum` −43,0 %, `swap` −41,5 %.
- `:98` et `:100` donnent `compare_and_branch` −53,5 % et `clamp` −46,8 %. Le tableau mesuré dit −62,0 % et −51,6 %.
- `:118` écrit « pea.w is smaller in code size (2 bytes vs 3) ». PEA fait 3 octets (WDC 65816 manual, chunk `54f0f7fadfea5123` : `F43421 PEA $2134` ; softpixel `c96664aebe366231` : « PEA addr F4 3 5 »). L'explication de la perte de `pea_constant_args` est donc fausse.
- `:129-130` décrit la règle de la feuille « stays a function when its caller still calls something else ». Cette règle est désactivée par défaut depuis `8df59cb` (`PINS.md`).
- `devtools/compiler-tests/README.md` dit « 76/76 » et « struct by value (parameter, return, assignment) » ; l'affectation fonctionne depuis le 03/10.
- `testing/fixtures/compiler/c_features/main.c:19-20` dit « struct … assignment by value … REFUSES ».
- `PINS.md` dit « QBE's `tools/test.sh` is 56/56 … must stay so ». Mesuré aujourd'hui : 55 pass.
- `docs/STABILITY.md:50-52` ne liste que trois refus. Il en manque cinq : `float`, `long long`, `_Complex`, la copie `FAR`, et le VLA quand il sera refusé.
- L'erreur « whole-object copy of a FAR struct » pointe la ligne de *déclaration* (`fs.c:3:22`), pas celle de l'affectation.

**04-11 🟡 La suite amont de cproc ne dit presque plus rien.** Effort : M. Il y a 115 échecs attendus sur 171 tests (`cproc_known_fail.txt`), donc 67 % de la suite est neutralisée. L'affirmation « none is a wrong-code bug » (en-tête du fichier) n'est vérifiée par rien d'automatique : ce sont des diffs d'IR comparés à l'ABI de l'hôte.

## Risques

- **Le gel tombe pendant une rafale d'optimisations.** Dix commits QBE en cinq jours, dont des règles textuelles qui raisonnent sur la vivacité de `tcc__r9`, de X et des slots de page directe : c'est le profil exact du défaut 42461f9 (« stored a stale A », trouvé par `difftest_stmt` seed 105). Le différentiel est solide, mais il ne voit que ce que son générateur sait écrire : ni VLA (04-1), ni copie depuis un `const` nommé (04-2), ni `u8` passé à de l'assembleur (04-4).
- **Les croisements d'axes restent le point aveugle.** Les deux défauts nouveaux de cet audit tombent, comme les quatre du 03/10, à l'intersection de deux fonctions testées séparément : allocation × taille dynamique, et copie d'agrégat × qualificatif d'objet. `check_cproc_widths.py` couvre la largeur ; aucune matrice ne couvre les qualificatifs (`FAR`/`const`/`volatile` × `=`/copie/`++`/champ de bits/`->`/`[]`).
- **La promesse de `STABILITY.md`.** Tout ce qui compile aujourd'hui, même faux, sera difficile à refuser après la 1.0. Il faut fixer la frontière (VLA, `_Thread_local` accepté sans effet, `__STDC_VERSION__`) avant.
- **La resynchronisation amont devient impossible en pratique.** L'owner a décidé le 2026-10-13 de ne rien remonter. Avec 134 patchs sur une racine écrasée, chaque correctif amont devra être porté à la main (`e01207f` en est le premier cas qui entre en conflit). → expert gouvernance pour le critère A5 de `STRUCTURAL_DEFECTS.md:737`, qui dit toujours « 34 current patches ».
- **`difftest_stmt` interprète le C qu'il génère.** Une erreur partagée entre le modèle et le compilateur, une règle de C mal comprise des deux côtés, passerait. Le modèle d'expressions est vérifié par clang (`difftest.py`), mais pas celui des instructions.

## Améliorations recommandées

| # | Action | Sévérité traitée | Effort | Premier pas concret |
|---|---|---|---|---|
| 1 | Refuser le VLA : dans cproc, erreur « variable-length arrays are not supported on w65816 » quand `calcvla` voit un tableau à taille non constante ; définir `-D__STDC_NO_VLA__=1` dans `cc65816` ; en défense, faire refuser par QBE tout `Oalloc*` dont l'argument n'est pas une constante | 🔴 04-1 | S | Fixture `cases/negative/vla.c` + `.expect`, entrée dans `silent_defects_log.md` (2026-10-17), ligne dans `KNOWN_LIMITATIONS.md` et dans la liste des refus de `STABILITY.md` |
| 2 | `funccopy` reçoit la qualification de la source et de la destination (`accessqual`) et la pose sur chaque `load`/`store` (`farram`/`const` → lecture longue, `volat`) ; ou, à défaut, refus « copy of a const object by name » | 🟠 04-2 | S | Cellule `d_quals` : `r = kl;` avec `kl` `static const` (attendu `0x11223344`…), et `.checks` « absent `lda.w kl` » |
| 3 | `.DELETE_ON_ERROR:` dans `make/common.mk`, ou link dans un fichier temporaire renommé seulement après `opensnes-rom check` | 🟠 04-3 | S | `rm p2.sfc; make; make` doit échouer deux fois (→ expert build) |
| 4 | Remettre `ABI.md` en accord avec le code avant le gel : `:51/:381` « octet haut indéfini, l'appelé masque » (ou `extub`/`extsb` côté appelant), supprimer `:195-200`, corriger `:385-386` ; étendre l'ancre 4 de `check_doc_drift.py` à une liste de refus partagée avec `cases/negative/` | 🟠 04-4 | S | Une phrase par ligne fausse ; test : la liste des `.expect` négatifs apparaît dans `ABI.md` |
| 5 | Matrice de qualificatifs : générer {`FAR`,`const`,`volatile`} × {`=`, copie d'agrégat, `op=`, `++`, champ de bits, `->`, `[]`} et vérifier dans l'IR que chaque accès porte son drapeau, comme `check_cproc_widths.py` le fait pour les largeurs | 🟠 04-2, risque croisements | M | Recycler `scan.py` (audit du 03/10) comme cas de `run.py` |
| 6 | Préprocesseur à la sémantique de la cible : `-U__STDC_VERSION__ -D__STDC_VERSION__=201112L -U__STDC_HOSTED__ -D__STDC_HOSTED__=0 -D__CHAR_BIT__=8 -D__SIZEOF_INT__=2 …` ; décider puis documenter les en-têtes freestanding (au minimum `<stdint.h>`, `<stdbool.h>`, `<stddef.h>`, `<limits.h>` aux tailles de la cible, ou leur refus explicite) ; documenter le signe de `char` dans `ABI.md` | 🟡 04-5 | S | `printf '__STDC_VERSION__' \| bin/cc65816 -E` doit dire `201112L` quel que soit l'hôte |
| 7 | Lint hôte 16 bits : `clang --target=msp430-none-elf -D__OPENSNES__` | 🟡 04-6 | S-M | Le cas `s16 == 0xFFFF` ne doit plus bloquer ; `breakout/main.c` relu |
| 8 | Comparaison non signée quand les deux opérandes sont prouvés ≥ 0 (u8/u16 étendus) ; supprimer le second `and.w #$00FF` d'un slot déjà masqué ; copie d'agrégat sans passage par la page directe | 🟡 04-7 | M | `.checks` sur `boardGetCell` : `absent bvc`, `count 1 and.w #\$00FF` |
| 9 | Geler la vélocité du backend pour la fenêtre du critère 7 : ne plus fusionner de nouvelle règle de peephole, seulement des correctifs ; mettre les configurations « interrupteur coupé » dans la chasse nocturne (`compiler-hunt.yml`, une config par nuit) | 🟠 04-8, 🟡 04-9 | S | Une ligne dans `silent_defects_log.md` (« effort pendant la fenêtre ») et une matrice `env` dans le workflow |
| 10 | Absorber `e01207f` en le fusionnant avec `79ff312` (`visit()` + largeur portée par la marque) | 🟠 04-8 | S-M | `git -C compiler/qbe cherry-pick e01207f`, résoudre `copy.c`, `make test-toolchain-suites`, `difftest_stmt` porte + seed 241214 |
| 11 | Corriger `BENCHMARK.md:31,98,100,118,129-130`, le README des tests compilateur, l'en-tête de `c_features`, `PINS.md` « 56/56 », la liste de refus de `STABILITY.md` | 🟡 04-10 | S | Une passe de relecture, plus une ancre doc-drift pour le tableau « Biggest Wins » (qui doit suivre `bench_baseline.json`) |

## Verdict

Le cœur du compilateur est solide : les quatre 🔴 du 03/10 sont corrigés et épinglés, 13 500 programmes et 12 000 expressions aléatoires passent sur luna, et les optimisations récentes tiennent aussi bien sous sonde ciblée qu'avec leurs interrupteurs coupés. La frontière du C accepté n'est pourtant pas finie. Un VLA compile en corruption de pile silencieuse (04-1), une copie de struct depuis un `const` nommé n'est rattrapée que par un contrôle de lien qu'un second `make` contourne (04-2, 04-3), et `ABI.md`, que la 1.0 gèle, affirme encore cinq choses fausses (04-4). Ces quatre points sont à corriger, avec un ralentissement du backend pendant la fenêtre du critère 7, avant de geler une 1.0 qui promet de ne jamais refuser ce qu'elle accepte aujourd'hui.

## Suivi de l'audit du 03/10

| Constat (lettre A) | État au 2026-10-17 | Preuve d'aujourd'hui |
|---|---|---|
| A n°1-4 (🔴 `FAR ++`, copie `u32`, `= {0}`, champs de bits `FAR`/`const`) | **corrigé**, épinglé | `d_quals` dans `make tests` ; sonde indépendante `p1` sur luna : 12, 82, 5, `0x11223344` — tout juste |
| A n°5 (🟠 deux lectures d'un champ de bits `volatile` → une) | **corrigé, non épinglé** | `iso` `twice()` : deux `lda.w vb` ; aucun `.checks` ni cellule `d_quals` ne le garde (`grep volatile` dans `d_quals/main.c` : seulement les entrées) |
| A n°6 (🟠 `++` sur `volatile`, store sans `volat`) | corrigé (par `a1474c4`) | `silent_defects_log.md:50` ; copie d'une struct `volatile` : `volat` perdu par `funccopy`, latent (04-2) |
| A n°7 (🟡 argument `u8`/`s8` non étendu) | **ouvert** | `iso/u8arg.c` : `lda.w g16 / pha` pour `(u8)` et `(s8)`, `jml asmfn` pour l'appel de queue ; `ABI.md:51,381` inchangés (04-4) |
| A n°8 (🟡 lint hôte en `int` 32 bits) | **ouvert** | `make/common.mk:564` sans `--target` ; faux positif `s16 == 0xFFFF` reproduit (04-6) |
| A PF3 (🟠 classe « patch de largeur incomplet » sans garde-fou) | **partiel** | Largeurs : `check_cproc_widths.py` dans `make lint` (19 sites). Qualificatifs : pas de matrice, et 04-2 est une nouvelle occurrence de la classe |
| A PF4 (🟠 doc canonique contraire au code) | **partiel, et aggravé** | Structs et `volatile` réglés ; `u8` toujours faux ; trois nouvelles affirmations fausses (`ABI.md:195-200`, `:385`, `:386`) depuis que `float`, `long long` et les retours de struct sont refusés (04-4) |
| A PF6 (🟡 comparaisons lourdes) | **partiel** | La matérialisation 0/1 a disparu ; la comparaison signée sur `u8` et le double masque restent dans `boardGetCell` ; 144 sites évitables dans les exemples (04-7) |
| A PF7 (🟡 `e01207f` non absorbé) | **ouvert** | `git -C compiler/qbe merge-base --is-ancestor e01207f d1ad402f` → non ; entre maintenant en conflit avec `79ff312` (04-8) |
| H PF16 (critère A5 « 34 current patches ») | ouvert → expert gouvernance | `STRUCTURAL_DEFECTS.md:737` dit toujours 34 ; réalité 134 + 46 + 5 ; décision du 2026-10-13 (pas de contribution amont) non reportée dans le critère |
