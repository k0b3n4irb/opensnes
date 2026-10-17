# 08 — Benchmark et performance

Expert : benchmark et performance · date : 2026-10-17 · HEAD audité : `develop` @ `73a3fdfe` (worktree `opensnes-audit`, sous-modules au même SHA, luna v1.38.0, PVSnesLib `fa758c9b 2025-12-28` dans `~/workspace/pvsneslib`)

## Périmètre couvert

- Lu : `docs/BENCHMARK.md`, `docs/PERF.md`, `ROADMAP.md:24-26`, `devtools/cyclecount/{bench.py,cyclecount.py,README.md,bench_functions.c,bench_baseline.json}`, `devtools/sdkbench/{run.py,workloads.c,README.md,*.json}`, `devtools/libbench/{run.py,scene.c,README.md}`, `devtools/twinbench/run.py`, `testing/nmi_budget.py`, `lib/include/snes/vramqueue.h`, `hdma.h`, le handler NMI (`templates/crt0.asm`), l'étape CI `opensnes_build.yml:435-452`, `.claude/notes/chantiers/beat_pvsneslib.md` (troisième bench), les rapports du 03/10 (A, B) et `2026-10-04_reste_a_traiter.md`.
- Lancé dans le worktree :
  - `make bench` → `OK: 34 functions, 1238 total cycles (baseline 1238)`.
  - Côté PVSnesLib, à la main (le script de la doc n'existe pas, voir 08-3) : `816-tcc -Wall -c bench_functions.c` + `816-opt`, puis `cyclecount.py --json --pvsneslib` → 2284 brut / 1981 opt, soit **−37,5 % reproduit à l'unité**, fonction par fonction.
  - `PVSNESLIB_HOME=~/workspace/pvsneslib python3 devtools/sdkbench/run.py` (26 s) : PVSnesLib remesuré = référence commitée **à l'unité près** ; OpenSNES **122 719 436** mclk contre 125 832 590 publiés → **−62,3 %** aujourd'hui (−61,4 % publiés). `--check` vert.
  - `PVSNESLIB_HOME=… python3 devtools/libbench/run.py` (16 s) : les 14 lignes de `PERF.md` reproduites à l'unité, sauf `vramq` 10 791 (publié 10 738). `--check` vert (14/14 dans les 10 %).
  - `python3 testing/nmi_budget.py` : 12/12 verts.
  - `PVSNESLIB_HOME=… python3 devtools/twinbench/run.py` (après `make -C` des deux fixtures `retired_examples/{tiled,metasprite}`, sans quoi le script s'arrête : « no ROM in …/retired_examples/tiled ») : 22 paires.
  - `luna profile` de `sprites/dynamic_sprite` (frames 60-252) et `games/likemario` (frames 120-420, RIGHT tenu dès 130, la recette de `PERF.md`).
  - Temps de compilation : `cc65816` sur `workloads.c` 0,18 s ; `816-tcc` + `816-opt` 1,66 s ; les 15 `.c` de `examples/games/*` en 0,5 s.
  - `bin/opensnes-rom check` sur `text/print_string`.
- Corpus Cartouche (sources OpenSNES exclues) : durée du VBlank NTSC — snesdev-wiki *Timing* : « Begins on vertical line V=225 … Ends after vertical line V=261 (NTSC) … 37 (NTSC) … lines », 1364 mclk par ligne (anomie, fullsnes). `PERF.md` (50 468 mclk) est juste.

## Points forts

1. **Les chiffres publiés se reproduisent, à l'unité.** Le tableau statique (−37,5 %), la colonne PVSnesLib de `sdkbench` et de `libbench` sont retrouvés à l'identique sur cette machine. luna est déterministe : la variance d'une exécution à l'autre est nulle. Ce qui bouge vient de la disposition du code : `vramq` est passé de 10 738 à 10 791 sans changement de la lib, et la doc le dit pour `oamxy` et `dma` (« read these two as ties »).
2. **La méthode mesurée est saine et honnête.** On compile le même fichier C (`workloads.c`, aucun `#if` par SDK hormis `FAR` défini à vide chez PVSnesLib), sur le squelette `hello_world` de PVSnesLib, avec ses règles (816-tcc + 816-opt). Les deux ROM sont en LoROM SlowROM (`hdr.asm:21` chez PVSnesLib). Un checksum vérifie que les deux calculent la même chose, et `stepseq`, que 816-tcc compile faux, est **retiré des totaux** au lieu d'être compté. Le coût est la différence avec une ROM vide, sans boot ni NMI. La taille de code et la profondeur de pile sont publiées à côté des cycles, ce que peu de bancs font.
3. **Le tableau mesuré ne peut pas dériver de son JSON** : `write_doc()` le régénère entre deux marqueurs. Le tableau statique est ancré par la sentinelle (ancre 13). Le constat du 03/10 (« −32 % » contre « −21 % » réel) est fermé.
4. **Trois portes en CI** : cycles statiques, 24 charges luna et 14 lignes de lib (`opensnes_build.yml:435-452`), plus le budget NMI dans `make tests`.
5. **Le coût est dit là où il est contre-intuitif** : `vramqueue.h:27-28` écrit que la file coûte *plus* au total que les appels qu'elle remplace (21 700 contre 16 600 mclk) et moins en VBlank. `hdma.h:623-628` donne le coût d'`hdmaIrisWipe` (≈ 2,15 M mclk) et son `WaitForVBlank`.
6. **La compilation est rapide** : 9 fois plus rapide que la chaîne PVSnesLib sur le même fichier. Aucun coût caché de ce côté.

## Points faibles

**08-1 🟠 — Le seul banc « jeu entier » montre OpenSNES plus lent sur 4 exemples sur 22, et aucune page publique ne le dit.** `twinbench` (aujourd'hui, mclk de travail par image, moyenne des frames 200-500) : `mapandobjects` 64 057 → 73 670 (**+15,0 %**), `slopemario` 58 282 → 65 008 (**+11,5 %**), `dynamicmetasprite` 44 332 → 54 844 (**+23,7 %**), `mapscroll` +3,3 %. Ce sont le moteur d'objets, les pentes et le métasprite dynamique, c'est-à-dire le cœur d'un jeu de plateforme. `BENCHMARK.md:13` et `ROADMAP.md:24-26` annoncent « 61 % faster ». `BENCHMARK.md:258` dit « What this does not measure: … a whole game », alors que le dépôt le mesure (`make bench-twins`) et que les causes sont connues (`beat_pvsneslib.md`, troisième bench : copies du moteur d'objets, `oamMetaDrawDyn` qui relit chaque entrée). Le 61 % est vrai dans son périmètre (du C sans bibliothèque), mais un studio qui choisit le SDK pour faire un plateformer avec `object` + `map` sera plus lent que sous PVSnesLib, sans en avoir été averti. Effort S pour publier le tableau avec ses lectures, M-L pour combler le retard (API de callbacks en place, routines d'entrée de `sprite_dynamic.asm`).

**08-2 🟠 — La porte VBlank des hooks mesure des entrées de 300 mclk et laisse le travail réel hors de vue.** `nmi_budget.py` (ligne `EXTRA`, ajoutée pour fermer B PF2) mesure `oamVramQueueUpdate` à **312 mclk** et `oamDynamicNmiFlush` à 296. Les étiquettes de ces routines assembleur ne sont pas de la forme `sym@…` mais `_vqu_*` et `_endframe_*`, que `folded_sym()` ne replie pas. Le profil de `dynamic_sprite` montre le vrai coût : `_vqu_16x16` **7 904 mclk** au pire (frame 60), `_endframe_done` 540, `_vqu_done` etc. La porte vise donc un talon 25 fois plus petit que le travail. Même angle mort que celui que le docstring du script décrit pour `NmiHandler@…`. Rejouer : `testing/bin/luna profile examples/sprites/dynamic_sprite/dynamic_sprite.sfc --from-frame 60 --until-frame 252 --out x.json --top 0`, puis lire les entrées `_vqu_*`. De plus, aucune ligne ne mesure le **VBlank cumulé** (handler + `tilemapFlush` + hooks + `nmi_callback`). Chaque morceau a sa propre référence, et le seul plafond absolu (12 000) ne s'applique qu'à l'étiquette `NmiHandler`. Effort S (préfixes `_vqu_`/`_endframe_` dans le repli, ou demande à luna d'un `--budget` inclusif par plage d'adresses).

**08-3 🟠 — `PERF.md` sous-estime d'un facteur 3 à 4 les routines assembleur du moteur dynamique, et ses chiffres datent de la 0.49.** Le tableau ne compte que l'étiquette de la fonction (`PERF.md:76-80` le dit). Pour le code assembleur dont le corps porte des étiquettes `_o16d_*`, cela laisse de côté la plus grande partie du coût. Dans likemario aujourd'hui, `oamDynamicDraw` vaut 896 mclk à son étiquette, mais `oamDynamicDraw` + `oamDynamic16Draw` + `_o16d_*` font **≈ 4 080 mclk** par image. Le tableau publie 1 351. Les valeurs ont aussi bougé depuis la 0.49 sans que la page suive (`animTick` 3 387 publié contre 2 478 mesuré, `animPlay` 2 125 contre 1 546). Aucune ancre de sentinelle ne garde cette table, contrairement à celle de `libbench`. Un studio qui budgète son image avec cette page sous-estime le moteur de sprites. Effort S (sommer par plage de fonction, régénérer la table par script comme `libbench`).

**08-4 🟡 — Le chiffre de titre « 38 % plus rapide » ne mesure pas une vitesse.** `cyclecount.py` additionne une fois chaque instruction du corps (branches comptées non prises, boucles comptées une seule fois, `jsl __mul16` sans le coût de la routine). `loop_sum` « 71 cycles » est le poids de son code, pas le temps de `n` itérations. `call_add` « −90 % » vient de la comparaison d'un `jml` à `jsl`+`rtl`, alors que l'appelé s'exécute pareil. C'est un bon garde-fou de régression du codegen, pas une mesure à mettre en titre de `ROADMAP.md:24` à côté du 61 % mesuré. Effort S.

**08-5 🟡 — `BENCHMARK.md` se contredit et renvoie vers un script qui n'existe pas.** `:31-32` : « It loses on code that goes through pointers » ; le tableau, lui, donne `array_read`, `array_write`, `array2d_read` et `struct_sum` gagnants. `:98` et `:100` : `compare_and_branch` « −53,5 % » et `clamp` « −46,8 % », contre −62,0 % et −51,6 % dans le tableau au-dessus. La colonne « Impact » des phases (`:136-150`, « ~10 % ») n'a aucune source. La section Reproducing (`:156`) cite `devtools/benchmark/compare_compilers.sh`, absent de l'arbre ; le README de `cyclecount` le cite deux fois. Aucun script du dépôt ne régénère la colonne PVSnesLib du banc statique : j'ai dû la reconstruire à la main. Effort S.

**08-6 🟡 — Les références ne suivent pas les gains, ce qui rend les portes plus lâches à chaque optimisation.** Le `baseline.json` de `sdkbench` date du 13/10. Cinq commits `perf(compiler)` du 16/10 l'ont battu : `bytes` 3 697 616 → 3 157 202 (−14,6 %), `list` 1 698 250 → 1 323 922 (−22 %), `depot` 13,9 M → 12,9 M. La page publie donc −61,4 % au lieu de −62,3 %. Surtout, `--check` tolère +25 % par charge et +5 % au total *par rapport à cette référence ancienne* : `list` pourrait reperdre 37 % sans que la CI ne bronche. Sur un émulateur déterministe, un seuil de 25 % n'a pas de justification statistique. La taille et la pile sont publiées mais pas contrôlées. Effort S (seuil serré par charge, re-capture à chaque `perf(compiler)`, contrôle de la taille).

**08-7 🟡 — Deux lignes de `libbench` comparent des choses différentes et comptent pourtant dans « 14 lignes sur 14 ».** `world` et `worldo` comparent la routine assembleur `oamPlaceWorld` à une boucle C chez PVSnesLib. La ligne équitable est `worldc` (même source, −58 %). `vramq` ne compte que le `vramQueueFlush` d'OpenSNES et laisse de côté les `vramQueuePush`, alors que le coût total est supérieur (`scene.c:303-307` le dit, la table non). La légende annonce « OpenSNES costs no more than PVSnesLib on 14 of the 14 rows ». Les lignes sont étiquetées, mais la phrase de synthèse les additionne. Effort S.

**08-8 🟡 — `twinbench` est cassé sur un arbre fraîchement construit.** Le script vise `testing/fixtures/retired_examples/{tiled,metasprite}` (déplacés le 15/10), que `make examples` ne construit pas. Il faut un `make -C` à la main. Aucune CI ne lance `twinbench` : la régression est passée inaperçue. Effort S.

**08-9 🟡 — Petites incohérences d'unités.** `nmi_budget.py:3-4` donne « about 51 800 master cycles » de VBlank (38 lignes) ; l'arbitre en compte 37, soit 50 468, et `PERF.md` est juste. `vramqueue.h:33` dit « about 49 000 », ce qui correspond à la part hors rafraîchissement (48 988, cohérent). `libbench/README.md:27` renvoie au « CHANGELOG of 1.0.0 », qui n'existe pas. Effort S.

**08-10 🟡 — Ce qu'un studio voudrait savoir et que rien ne publie.**
- La **pire image** d'une scène de jeu, pas seulement la moyenne : `twinbench` et `PERF` donnent des moyennes, et seul le handler a un maximum.
- Le **VBlank restant** pour le jeu, une fois handler, flush et hooks payés (voir 08-2).
- L'**empreinte fixe** du SDK : un `print_string` occupe 3 029 octets de la bande RAM C de 8 Ko (19 sections) et environ 5,2 Ko de code en banque $00 (`opensnes-rom check`). Aucune page ne le chiffre. → expert lib/mémoire.
- Le gain **FastROM** : « about a quarter » à `PERF.md`, sans mesure.
- Le temps de **boot**.
- Le débit réel **SA-1** et Super FX sur une charge C ou assembleur type (→ expert 07/chips).
- 32 en-têtes sur 39 ne citent aucun coût (`grep -c 'mclk\|master cycle\|cycles'`) : `map.h`, `snesmod.h`, `object.h`, `hdma.h` (hors iris), `background.h`. Le principe 5 de `PHILOSOPHY.md` n'est tenu que par `PERF.md`, avec les réserves de 08-3.

## Risques

- **Communication 1.0** : publier « 61 % plus rapide » sans le tableau des jumeaux expose le projet à un démenti public facile. Le premier studio qui porte un plateformer PVSnesLib (le cas d'usage le plus probable, vu le corpus de ports) mesurera +11 à +24 % sur le moteur d'objets et le métasprite.
- **Fausse assurance en VBlank** : la porte NMI est verte pendant que le travail réel du moteur dynamique (≈ 8 000 mclk au pire) et le cumul des hooks ne sont gardés par rien. Une régression là donnerait des tuiles manquantes sans aucune erreur, chez un utilisateur.
- **Portes qui se relâchent seules** : chaque gain non re-capturé élargit la marge de régression silencieuse (08-6).
- **Bancs de jeu dépendants d'arbres hors CI** (PVSnesLib local, fixtures non construites) : ils pourrissent sans bruit (08-8).

## Améliorations recommandées

| # | Action | Constat | Sév. | Effort |
|---|---|---|---|---|
| 1 | Publier le tableau `twinbench` (les 22 paires, les 4 en retard avec leur cause) dans `BENCHMARK.md`, nuancer `ROADMAP.md:24-26` (« sur du C sans bibliothèque ; plus lent sur map+objets et métasprite dynamique ») ; générer la table entre marqueurs comme `sdkbench` | 08-1 | 🟠 | S |
| 2 | Combler le retard du moteur d'objets (callbacks en place, sans les deux copies restantes) et de `oamMetaDrawDyn` (entrées de `sprite_dynamic.asm` avec offset en X), avant le gel de l'API | 08-1 | 🟠 | M-L |
| 3 | `nmi_budget.py` : replier aussi les étiquettes `_vqu_*`/`_endframe_*` (ou des plages d'adresses issues de `[sections]`) ; ajouter une ligne « VBlank cumulé » (handler + hooks) avec un plafond absolu ; à moyen terme, demander à luna un `--budget` inclusif par plage (direct, `exchanges.md`) | 08-2 | 🟠 | S |
| 4 | Régénérer `PERF.md` par script (sommes par section `.text.*` ou par plage de fonction), ancrer la table dans la sentinelle, retirer « OpenSNES 0.49 » | 08-3 | 🟠 | S |
| 5 | Retirer le −38 % statique des titres (`ROADMAP.md:24`, `BENCHMARK.md:13`) ou le présenter pour ce qu'il est (poids du codegen) ; corriger `:31`, `:98`, `:100`, `:156` ; ajouter un `--pvsneslib` à `bench.py` qui recalcule la colonne PVSnesLib | 08-4, 08-5 | 🟡 | S |
| 6 | Seuils de `sdkbench --check` serrés (+2 % par charge sur luna déterministe), re-capture obligatoire à chaque `perf(compiler)`, contrôle de taille et de pile | 08-6 | 🟡 | S |
| 7 | Phrase de synthèse de `libbench` limitée aux lignes de même source ; colonne « total » pour `vramq` (push + flush) | 08-7 | 🟡 | S |
| 8 | `twinbench` : construire les fixtures qu'il lit (dépendance `make`) ; un job hebdomadaire côté OpenSNES seul (la colonne PVSnesLib figée en JSON comme `sdkbench`) | 08-8 | 🟡 | S |
| 9 | Une page « budget d'un jeu » : pire image, VBlank restant, empreinte fixe RAM/ROM/VBlank du SDK, FastROM mesuré | 08-10 | 🟡 | M |

## Verdict

Les chiffres publiés se reproduisent exactement, la méthode mesurée (même C, checksum, différence avec une ROM vide, taille et pile publiées) est d'un niveau professionnel, et le −61 % sur du C pur est réel (−62,3 % aujourd'hui). La présentation est en revanche plus flatteuse que les mesures du dépôt lui-même : le banc des jumeaux, qui mesure des jeux entiers, place OpenSNES derrière PVSnesLib de 11 à 24 % sur le moteur d'objets et le métasprite dynamique, sans que ce soit publié. Dans le même temps, la porte VBlank et `PERF.md` ne voient qu'une fraction du coût des routines assembleur du moteur dynamique. Avant la 1.0 il faut publier les jumeaux et corriger ces deux angles morts de mesure (une journée en tout). Le rattrapage du moteur d'objets est le vrai chantier de performance qui reste.

## Suivi de l'audit du 03/10

| Constat (2026-10-04_reste_a_traiter.md) | État aujourd'hui | Preuve |
|---|---|---|
| Thème 4 / A n°2 : `BENCHMARK.md` « −32 % » contre −21 % du baseline, `ROADMAP` « 30 % » | **corrigé** | `make bench` : 1238 = baseline ; PVSnesLib recompilé : 1981 → −37,5 % identique au tableau ; ancre 13 de la sentinelle. Restent des contradictions de prose (08-5) |
| B PF2 : `nmi_budget.py` ne mesure que l'étiquette `NmiHandler` ; `tilemapFlush`, `oamDynamicNmiFlush`, `oamVramQueueUpdate` invisibles | **partiel** | Les trois sont dans `EXTRA` (12/12 verts) ; mais `oamVramQueueUpdate` mesure 312 mclk contre 7 904 pour `_vqu_16x16` (08-2), et il n'y a ni cumul ni `--budget` inclusif chez luna |
| B PF5 : coût d'`hdmaIrisWipe` et `WaitForVBlank` non documentés | **corrigé (doc)** | `hdma.h:623-628` donne ≈ 2,15 M mclk et l'attente de la première invocation ; le coût lui-même n'a pas changé (non remesuré) |
| D T8 : `nmi_budget.py`, 9 lignes sur 7 ROM, sans script d'entrée | **partiel** | 12 lignes sur 8 ROM ; les lignes `EXTRA` rejouent leur manifeste (`manifest_runs`), pas les lignes `NmiHandler` |
| Constats sans lettre (nouveaux) dans mon domaine | aucun | — |
