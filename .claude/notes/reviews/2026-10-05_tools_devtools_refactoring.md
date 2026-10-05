# `tools/` et `devtools/` — inventaire et plan de refactoring (2026-10-05)

Rapport demandé par le propriétaire (« cela se disperse un peu »). Lu sur
`develop` à `2718b372` (1.0.0 coupée sur develop, étiquette en attente).
Toutes les mesures ci-dessous sont reproductibles avec les commandes citées
en annexe ; rien n'a été modifié dans le dépôt.

## 1. Verdict en trois phrases

Les deux dossiers ne rangent pas par **qui exécute le fichier** mais par
histoire : « C compilé » d'un côté, « Python » de l'autre, puis tout ce qui
est arrivé depuis s'est posé au plus près. Résultat : cinq scripts de
`devtools/` partent dans le zip de release alors que son README dit « non
distribué », vingt projets ROM de fixture sont répartis sur trois racines et
listés à la main trois fois dans le `Makefile`, et les scripts s'importent
dans les deux sens par trente-trois `sys.path.insert`. Le code lui-même est
sain (chaque script a sa raison d'être écrite en tête, les outils C ont tous
une suite golden) ; c'est le **rangement** et la **colle** qui sont à refaire,
et cela se fait en lots courts sans toucher à l'API publique ni au format du
zip.

## 2. Inventaire chiffré

| Zone | Fichiers suivis | Lignes | Contenu réel |
|---|---|---|---|
| `tools/` outils C (9) | 193 | 28 097 l. de C, dont ~15 700 vendues (lodepng 7 254, stb_image 8 010, cmdparser 877) | gfx4snes, smconv, font2snes, img2snes, palplan, aseprite2snes, tmx2snes, wav2brr, sa1-patch |
| `tools/fuzz/` | 21 | 5 harnais libFuzzer | cibles des parseurs vendus |
| `tools/luna-test/` | 298 | 3 116 l. de Python (15 scripts + `probes/lib.py`) | 138 manifestes, 120 baselines, 5 ROM de stress, 2 rapports générés |
| `devtools/` scripts | 38 `.py` + 1 `.sh` | 8 905 l. de Python | 11 `check_*`, 2 `lint_*`, 6 tests unitaires, 7 générateurs, 3 bancs |
| `devtools/` fixtures ROM | 7 `libtests*` + 6 `compiler-tests/runtime` + `benchrom` (×2) | 15 projets `Makefile` | asserts via `test_*.py` (5) ou manifestes luna (2) |
| `devtools/compiler-tests/cases` | 190 fichiers | 11 cas négatifs + checks | vérifs C→ASM sans émulateur |

Activité des 60 derniers jours : 186 commits dans `tools/luna-test/`, 111 dans
`devtools/`, 7 dans chaque outil C. Le mouvement est tout entier dans le
Python ; c'est là que le rangement paie.

## 3. Les sept familles réelles

Le tri honnête n'est pas « C / Python » mais « qui lance le fichier » :

| Famille | Qui l'exécute | Où c'est aujourd'hui |
|---|---|---|
| **A. Outils d'assets livrés** | l'utilisateur, via `make/common.mk` | `tools/<outil>/` (cohérent) |
| **B. Vérifications de la build utilisateur** | `common.mk` à chaque link, donc **livrés dans le zip** | `devtools/symmap/symmap.py`, `check_bank_reads.py`, `check_nmi_wram_race.py`, `asset_budget.py`, `check_upgrade.py` |
| **C. Test d'un projet utilisateur** | `make test` d'un projet, **livré** | `tools/luna-test/project_test.py` + `luna_runner.py` + `probes/lib.py` + `luna.version`, `scripts/install-luna.sh` |
| **D. Harnais de test du SDK** | `make tests`, la CI | `tools/luna-test/` (runner, oracles, manifestes, baselines) |
| **E. Fixtures ROM** | `make tests` et les manifestes | `devtools/libtests*` (7), `devtools/compiler-tests/runtime/*` (6), `tools/luna-test/stress/*` (5), `devtools/benchrom` (2) |
| **F. Sentinelles et lints du mainteneur** | `make lint`, les hooks, la CI | `devtools/check_*.py`, `lint_*.py`, `verify_toolchain.py`, `toolchain_suites.py`, `link_modules.py`, `gen_luna_doc.py`, `check_doc_render.py`, `release_smoke.py`, `check_debug_info.sh` |
| **G. Générateurs ponctuels et provenance d'assets** | personne, ou un README d'exemple | `hicolor64.py`, `hicolor64hires.py`, `m7ptables.py` (cités par deux README d'exemples), `gen_hud_bar/`, `brr2it/`, `font2snes/` (aucun appelant, intacts depuis le 2026-03-03), `vram_layout/` (6 `vram.spec`, dépend d'ortools) |

Plus les **données** posées à côté des scripts : `removed_api.txt`,
`cproc_width_sites.txt`, `toolchain-suites/*.txt`, `cyclecount/bench_baseline.json`,
`luna.version`, `luna.sha256`, et deux rapports générés commis dans le code
(`ROM_COVERAGE.md`, `CORPUS_COVERAGE.md`).

## 4. Constats

Numérotés pour que le plan (§6) puisse les citer.

### C1. La frontière `tools/` / `devtools/` est fausse dans les deux sens

- `devtools/README.md` : « NOT distributed in release packages ». Or
  `Makefile:51` dérive `RELEASE_DEVTOOLS` de `common.mk` et copie cinq scripts
  de `devtools/` dans le zip (famille B). C'est le correctif du 2026-09-26
  (le zip ne liait aucun projet) ; il a réparé le symptôme, pas le rangement :
  ce que `common.mk` exécute devrait vivre à côté de `common.mk`, qui est
  copié en entier (`cp -r make/*`).
- `tools/README.md` : « end-user-facing tools … shipped in the per-OS release
  archives ». Or de `tools/luna-test/` (298 fichiers) seuls quatre partent dans
  le zip ; les 294 autres sont le harnais interne du SDK (famille D).
- `tools/fuzz/` n'est ni livré ni un outil : c'est un banc de test des
  parseurs. Il n'est pas dans la table de `tools/README.md`.

### C2. Les deux README décrivent un autre dépôt

- `devtools/README.md` liste `check_mvn/` et `benchmark/` (n'existent pas) et
  **sept** entrées en tout ; il ignore les 11 `check_*`, les 2 `lint_*`, les 7
  fixtures, `compiler-tests/`, `vram_layout/`, `release_smoke.py`,
  `verify_toolchain.py`, `toolchain_suites.py`, `link_modules.py`…
- Il prescrit `uv sync` et un `pyproject.toml` dont la seule dépendance est
  Pillow (pour `gen_hud_bar`, sans appelant) ; `vram_layout.py` importe
  `ortools` qui n'y figure pas ; `uv` n'apparaît dans aucun workflow ni
  Makefile. Le reste du dépôt (README de `luna-test`) promet « stdlib only ».
- `tools/README.md` omet `aseprite2snes/`, `palplan/`, `fuzz/` ;
  `tools/sa1-patch/` et `tools/fuzz/` n'ont pas de README.

### C3. Dépendances croisées dans les deux sens, par `sys.path`

33 `sys.path.insert` dans 62 fichiers Python. Les sens :

- `tools/luna-test` → `devtools` : `rom_coverage.py:62` importe `symmap`,
  `wram_regress.py:115` importe `check_corpus_fresh`.
- `devtools` → `tools/luna-test` : les 5 `libtests*/test_*.py`, les 6
  `compiler-tests/runtime/*/test_*.py`, `benchrom/bench.py` et
  `b2_deref/bench.py` importent `probes/lib.py` ; `libtests_dsp1` importe
  aussi `luna_runner`.
- à l'intérieur de `luna-test` : `probes/lib.py:28` importe `luna_runner` (le
  module de 541 lignes qui porte `find_luna`, `LUNA_VERSION`, `REPO_ROOT`,
  `discover_example_roms`), et `wram_regress` importe `rom_coverage` pour un
  lecteur de `.sym`.

Aucun paquet, aucun `__init__.py`, chaque script reconstruit le chemin du
dépôt (`HERE.parents[3]`, `HERE.parent.parent / "devtools"`…). Un
déplacement de dossier casse douze fichiers en silence.

### C4. Vingt fixtures ROM sur trois racines, listées à la main trois fois

| Racine | Projets | Asserts |
|---|---|---|
| `devtools/libtests*/` | 7 | `test_*.py` (5) ; `libtests_gsu` et `libtests_snesmod` n'ont **que** des manifestes luna |
| `devtools/compiler-tests/runtime/` | 6 | `test_*.py` |
| `tools/luna-test/stress/` | 5 | manifestes `luna test` |
| `devtools/benchrom/`, `benchrom/b2_deref/` | 2 | `bench.py` (audits C1 et B2, ponctuels) |

Le `Makefile` répète la liste de construction des fixtures dans `tests`
(l. 220-227), `rom-coverage` (l. 371-378) et `test-manifests` (l. 513-521),
et partiellement dans `test-lib` (l. 272-294) et `test-pal` (l. 322). Dix
manifestes pointent sur `../../../devtools/libtests*/…`. Chacun des vingt
projets a son `Makefile` de 7 à 21 lignes qui inclut `common.mk` : ce sont des
exemples sans README, et ils ne sont ni dans `examples/` ni ensemble.

### C5. Logique dupliquée

- **Trois lecteurs de `.sym`** : `symmap.SymbolTable` (984 l., le lecteur
  « officiel »), `rom_coverage.load_labels` + `ram_band_top` (commenté « one
  reader of the .sym » alors qu'il en existe déjà un), et le parseur propre de
  `check_bank_reads.py`. `probes/lib.sym_size` en fait un quatrième, minimal.
- **Deux `find_luna`** : `luna_runner.py:94` (celui que tout le monde importe)
  et `release_smoke.py:57` (copie, pour ne pas dépendre du harnais).
- **Quatre lecteurs de `luna.version`** (`luna_runner`, `project_test`,
  `wram_regress`, `gen_luna_doc`), et `scripts/install-luna.sh` en bash.
- **Quatre énumérateurs du corpus** : `luna_runner.discover_example_roms`,
  `asset_budget.find_examples`, `check_corpus_fresh` (glob `*.sfc`),
  `check_doc_drift` (rglob `main.c`, deux fois).
- **Deux « budget »** : `tools/luna-test/budget.py` (mesuré sur luna) et
  `devtools/asset_budget.py` (statique, exécuté par `common.mk`), sans
  renvoi de l'un à l'autre.

### C6. Les outils C : neuf Makefiles copiés, huit runners golden réécrits

- Les neuf `tools/*/Makefile` (45 à 79 lignes) répètent le même bloc clang /
  `-static` sauf Darwin / `SANITIZE=1` / `install` vers `../../bin/`, avec
  des variantes de nommage (`OBJ` vs `BUILD`, `EXE` vs `TARGET`,
  `DATESTRING` ou non). Un changement de politique (un flag, un OS) se fait
  en neuf endroits.
- Les huit `tests/run_golden.py` (70 à 217 lignes) réécrivent chacun le
  cycle « lancer l'outil sur `fixtures/`, comparer octet par octet avec
  `golden/` » ; `diff` entre deux d'entre eux : 96 à 249 lignes.
- Versions par outil sans règle ni bump : 1.0.0 (×6), 1.0.1 (tmx2snes),
  2.0.0 (gfx4snes), 2.1.0 (smconv).
- Code tiers dispersé : lodepng et cmdparser dans `tools/common/`, stb_image
  dans `font2snes/src/`, cute_tiled à la racine de `tmx2snes/`, un `json.c`
  maison dans `aseprite2snes/src/` ; les harnais de `fuzz/` les atteignent par
  chemins relatifs. L'audit du 09-26 (gouv. 6, rec. 14) demandait déjà de les
  réunir et de les citer dans `ATTRIBUTION.md`.
- `tmx2snes/` est le seul outil sans `src/` (un `.c` à la racine, avec son
  `.o` à côté).

### C7. Reliquats de l'histoire

- `luna_runner.find_luna` teste encore `tools/luna-test/vendor/luna-<v>-linux-<arch>/`
  (l'ère pré-`install-luna.sh`) et le README de `luna-test` le documente.
- Mesen2 / snes9x / opensnes-emu sont nommés dans 16 fichiers de `tools/` et
  `devtools/` ; `stress/README.md` présente encore Mesen2 comme l'oracle de
  validation. L'ancre 11 du sentinel ne regarde que `.claude/agents`, `skills`,
  `hooks`.
- `stress/mcp_sweep.py` est épinglé « luna v1.14.0, 94 outils » (nous sommes
  en v1.32.0) et se décrit « not a standing regression » ; `mcp_probe.py` est
  un client MCP de référence. `luna_tooling.md` affirme « aucun prototype
  transitoire en ce moment » tandis que `stress/README.md` en liste deux.
- `benchrom/` (audit C1) et `benchrom/b2_deref/` (audit B2) sont des mesures
  ponctuelles commises comme des outils ; leurs résultats sont dans les notes.
- `gen_hud_bar/`, `brr2it/`, `font2snes/font2snes.py` : aucun appelant, aucun
  commit depuis le 2026-03-03. Le C `font2snes` a sa propre suite golden ; la
  « référence Python » ne vérifie plus rien.
- `check_vram_layout.py` (lint stdlib, dans `make lint`) et `vram_layout/`
  (générateur CP-SAT) portent le même nom pour deux rôles, plus
  `test_check_vram_layout.py` à la racine.

### C8. Nommage

- Tiret ou souligné au hasard : `luna-test`, `compiler-tests`,
  `toolchain-suites/` lu par `toolchain_suites.py`, `sa1-patch/` qui produit
  `sa1_patch`.
- `tools/luna-test/manifest.toml` (les réglages du runner, 138 lignes) à côté
  de `manifests/` (138 manifestes `luna test`) : deux objets différents, un
  seul mot.
- Trois `bench.py` (`cyclecount/`, `benchrom/`, `benchrom/b2_deref/`) ; deux
  `font2snes` ; `budget.py` et `asset_budget.py`.

### C9. Les tests des outils ne sont pas dans la porte locale

Les six tests unitaires (`test_check_doc_drift`, `test_check_nmi_wram_race`,
`test_symmap`, `test_check_vram_layout`, `test_harness`, `test_asset_budget`)
tournent dans `lint.yml` ; côté `make`, seul `test_harness` est dans
`make lint`. `test_check_doc_drift.py` n'est cité par aucune cible. La règle
`testing.md` dit pourtant « la porte qu'on dit au contributeur de lancer doit
être celle de la CI ». Style mêlé : 28 scripts en `argparse`, 11 en `sys.argv`
à la main.

## 5. Ce qui tient bien, à ne pas défaire

- **La couche d'orchestration luna est mince et bien séparée** : `probes/lib.py`
  (215 l.) est une vraie petite API (`peek`, `assert_mem`, `dump_vram`,
  `cgram_words`) utilisée par 14 fichiers ; `test_harness.py` pine les
  décisions du runner. C'est le noyau autour duquel ranger.
- **Chaque script porte sa raison d'être** en tête (date, revue, numéro
  d'item). Le rapport s'est écrit en lisant ces en-têtes.
- **Les outils C ont tous une suite golden, un fuzz, et un build sanitisé.**
  Rien à reprendre sur le fond.
- **`RELEASE_DEVTOOLS` dérivé de `common.mk`** a tué une classe de bug ; le
  lot B ci-dessous le rend inutile sans perdre la garantie.
- Les règles existent (`luna_tooling.md`, `testing.md`, `abi_lint.md`) ; ce
  qui manque est une règle de **rangement**.

## 6. Cible proposée

Le principe : **un dossier par exécutant**, et ce qu'un fichier de build
appelle vit à côté de ce fichier.

```
tools/                     A. outils d'assets C livrés (inchangé en surface)
  tool.mk                     le bloc clang/static/sanitize/install, inclus par les neuf Makefiles
  third_party/                lodepng, cmdparser, stb_image, cute_tiled (+ ATTRIBUTION)
  tests/golden.py             le runner golden unique ; chaque run_golden.py devient une table
  <outil>/{src,tests}         tmx2snes rejoint le schéma src/

make/
  common.mk
  checks/                    B. ce que common.mk exécute : symmap.py, check_bank_reads.py,
                                check_nmi_wram_race.py, asset_budget.py, check_upgrade.py
                                (le zip copie make/ en entier : RELEASE_DEVTOOLS disparaît)

testing/                   C + D + E. le harnais et ses fixtures
  luna_runner.py, wram_regress.py, rom_coverage.py, … (les 15 scripts)
  lib/                        ex-probes/lib.py + luna.py (find_luna, LUNA_VERSION, firmware_dir)
                                + corpus.py (énumération des exemples) — l'unique endroit importé
  runner.toml                 ex-manifest.toml
  manifests/, baselines/
  fixtures/                   les 20 projets ROM : lib/, lib_fx/, …, compiler/a6_farptr/, …,
                                stress/hwmath/, … ; un fixtures.mk qui liste tout, inclus par le Makefile
  project_test.py             livré (avec lib/ et luna.version, comme aujourd'hui)

devtools/                  F + G. mainteneur seulement, jamais livré
  lint/                       check_doc_drift, check_doc_render, check_asm_abi, check_vram_layout,
                                check_cproc_widths, check_corpus_fresh, lint_asm, lint_commits,
                                verify_toolchain, toolchain_suites, link_modules, gen_luna_doc,
                                check_debug_info.sh, release_smoke
  bench/                      cyclecount/
  assets/                     hicolor64, hicolor64hires, m7ptables, vram_layout (provenance d'assets)
  data/                       removed_api.txt, cproc_width_sites.txt, toolchain-suites/
  README.md                   régénéré depuis l'inventaire, une ligne par fichier
```

Ce qui ne bouge **pas** pour l'utilisateur : `make`, `make test` dans un
projet, `scripts/install-luna.sh`, les binaires de `bin/`, le contenu du
zip (seuls des chemins internes à `common.mk` changent). Rien de l'API
publique n'est concerné : le gel 1.0 porte sur les en-têtes, pas sur le
rangement du dépôt.

Deux options écartées : fusionner `tools/` et `devtools/` en un seul
dossier (on perdrait la seule frontière que les utilisateurs comprennent,
« C livré ») ; faire du Python un paquet `pip` (le dépôt tient à « stdlib,
`python3 chemin/script.py` », et c'est un bon choix pour un SDK).

## 7. Plan en lots

Chaque lot est un commit sur `develop`, validé par `make clean && make`,
`make tests`, `make lint`, `make docs-strict`, et pour les lots qui déplacent
des scripts, `make release` + `release_smoke.py` sur le zip (c'est le test
que le 09-26 n'avait pas). Les lots 1 à 3 ne déplacent rien et peuvent se
faire aujourd'hui ; les lots 4 à 7 déplacent et doivent se faire un par un.

| Lot | Contenu | Constats | Taille | Risque |
|---|---|---|---|---|
| **1. Dire la vérité** | Réécrire `tools/README.md` et `devtools/README.md` depuis l'inventaire (une ligne par fichier, famille, appelant) ; README pour `sa1-patch/` et `fuzz/` ; `pyproject.toml` : retirer `uv`, déclarer `ortools` optionnel ou supprimer le fichier ; corriger « not distributed » | C1, C2 | S | nul |
| **2. Élaguer** | Supprimer `gen_hud_bar/`, `brr2it/`, `font2snes/font2snes.py` (après un mot du propriétaire : aucun appelant depuis sept mois) ; `benchrom/` et `b2_deref/` → une note d'archive avec la mesure, puis suppression ; `mcp_sweep.py` supprimé, `mcp_probe.py` gardé ou supprimé selon `luna_tooling.md` (liste des prototypes mise à jour) ; retirer la branche `vendor/` de `find_luna` et du README ; réécrire `stress/README.md` sans Mesen2 | C7 | S | nul |
| **3. La porte locale = la CI** | Cible `make test-devtools` qui lance les six tests unitaires ; `lint.yml` l'appelle ; `make lint` l'inclut | C9 | S | nul |
| **4. `make/checks/`** | Déplacer les cinq scripts de la famille B ; `common.mk`, `Makefile`, les trois workflows (`symmap --check-overlap`), les règles (`bank0_budget.md`, `nmi_audit.md`, `abi_lint.md`) et les docs qui les citent (14 pages de `docs/`) ; supprimer `RELEASE_DEVTOOLS` ; `release_smoke.py` prouve le zip | C1, C5 (symmap unique) | M | faible, mécanique ; le sentinel (ancre 10/16) et `grep -rn 'devtools/symmap'` listent les sites |
| **5. `testing/lib/`** | Renommer `tools/luna-test` → `testing/` ; `probes/lib.py` + `luna.py` + `corpus.py` dans `testing/lib/` ; `rom_coverage` et `check_bank_reads` lisent le `.sym` via `symmap` ; `release_smoke` importe `find_luna` ; `manifest.toml` → `runner.toml` ; les 33 `sys.path.insert` deviennent un seul en-tête de deux lignes pointant sur `testing/lib` | C3, C5, C8 | M | moyen : `release.yml` et `opensnes_build.yml` nomment `tools/luna-test/bin/luna` et `baselines/` ; `install-luna.sh` ; `luna_tooling.md`, `testing.md`, `partners.md` ; la copie du zip |
| **6. `testing/fixtures/`** | Regrouper les 20 projets ROM ; un `fixtures.mk` (liste unique) inclus par `tests`, `rom-coverage`, `test-manifests`, `test-pal`, `test-lib` ; les 10 manifestes qui pointent sur `devtools/` ; `test_*.py` pour `libtests_gsu` et `libtests_snesmod` ou note expliquant pourquoi les manifestes suffisent | C4 | M | moyen : `rom_coverage.py` et `wram_regress.py` connaissent les chemins des fixtures ; les baselines WRAM sont indexées par nom de ROM (vérifier qu'un renommage ne les invalide pas) |
| **7. Outils C** | `tools/tool.mk` ; `tools/third_party/` + `ATTRIBUTION.md` ; `tools/tests/golden.py` ; `tmx2snes/src/` ; une règle de version (la version du SDK, ou un `VERSION` par outil bumpé par le lint des commits) | C6 | M | faible : les suites golden sont le filet ; `fuzz/Makefile` et le job `sanitizers` suivent les chemins |

Après le lot 7 : une règle `.claude/rules/tooling_layout.md` (le tableau du
§3 et la phrase « ce qu'un fichier de build appelle vit à côté de lui »),
et une ancre 17 dans `check_doc_drift.py` : **tout script de `devtools/` et
`testing/` est cité par une cible `make`, un workflow ou la table d'un
README** — c'est le détecteur d'orphelins qui aurait signalé `gen_hud_bar`
en avril.

Ordre recommandé : 1, 2, 3 aujourd'hui (une demi-journée, aucun risque),
puis 4, 5, 6, 7 à raison d'un lot par séance, le zip testé à chaque fois.
Le lot 5 est le pivot : c'est lui qui donne l'endroit unique à importer ;
le lot 6 s'appuie dessus.

## 8. Annexe — commandes de l'inventaire

```sh
git ls-files tools devtools | awk -F/ '{print $1"/"$2}' | sort | uniq -c | sort -rn
wc -l tools/luna-test/*.py tools/luna-test/probes/*.py devtools/*.py devtools/*/*.py
grep -nE 'devtools/|tools/' Makefile make/common.mk .github/workflows/*.yml scripts/githooks/*
grep -rn 'sys.path.insert' --include=*.py devtools tools/luna-test | wc -l      # 33
grep -rnE '^(from|import) (luna_runner|symmap|lib)\b' --include=*.py devtools tools/luna-test
grep -nE 'MAKE\) -s -C devtools/libtests$' Makefile                                # 3 listes + test-pal
grep -hE '^rom = ' tools/luna-test/manifests/*.toml | grep -v examples/ | sort | uniq -c
for f in $(git ls-files devtools tools/luna-test | grep -E '\.(py|sh)$'); do \
  echo "$(grep -rlF "$(basename $f)" Makefile make .github scripts docs devtools tools/luna-test .claude | grep -v "^$f$" | wc -l) $f"; done | sort -n
diff tools/gfx4snes/tests/run_golden.py tools/palplan/tests/run_golden.py | grep -c '^[<>]'   # 249
grep -HnE '^VERSION' tools/*/Makefile
grep -rniE 'mesen2|opensnes-emu|snes9x' -l tools devtools --include=*.py --include=*.md --include=*.c
git log -1 --format=%as -- devtools/gen_hud_bar devtools/brr2it devtools/font2snes   # 2026-03-03
```

## 9. Complément (même jour) — deux publics, et l'héritage PVSnesLib

Le propriétaire a retenu le plan et ajouté deux directions : séparer
nettement **l'utilisateur du SDK** (qui fait un jeu) du **contributeur**
(qui fait le SDK), avec une release légère pour le premier ; et sortir les
outils de l'héritage PVSnesLib pour proposer au développeur de jeu quelque
chose de plus moderne. Ce qui suit est l'état des lieux de ces deux points et
une proposition ; rien n'est décidé ici.

### 9.1 Ce que l'utilisateur reçoit aujourd'hui

Le zip v0.48.0 pèse 17 à 24 Mo selon l'OS. Dedans, non compressé :

| Morceau | Poids | Pour qui |
|---|---|---|
| `bin/` (17 binaires statiques : wla-dx ×4, cproc, qbe, 9 outils, `opensnes`) | 55 Mo | utilisateur |
| `lib/build` (LoROM, HiROM, SA-1, Super FX) + `lib/include` | 7,3 Mo | utilisateur |
| `make/`, `templates/`, `starter/`, `install-luna.sh`, 4 fichiers de `luna-test`, 5 scripts de `devtools` | < 1 Mo | utilisateur |
| `examples/` (91 projets, sources + assets 6,5 Mo, ROM 23 Mo, objets) | 47 Mo | lecteur, pas constructeur |
| `docs/build/html` | 29 Mo | lecteur |

Les deux tiers du zip sont de la lecture (exemples construits, HTML Doxygen)
que l'utilisateur consulte une fois et que chaque release re-livre quatre
fois. Le SDK proprement dit (ce qu'il faut pour `make` un projet) tient
dans le premier tiers.

### 9.2 L'héritage PVSnesLib, outil par outil

| Outil | Origine | Ce qui en reste dans l'usage |
|---|---|---|
| `gfx4snes` | pcx2snes / gfx2snes d'Alekmaul, réécrit mais même contrat | les drapeaux d'une lettre (`-s 32 -o 16 -u 16 -p -i`, `-m -M 7`, `-c`, `-P`) et leur ordre ; pas de `-h` (audit 09-26) ; 11 formes d'appel différentes dans les exemples |
| `smconv` | SNESMOD de Mukunda Johnson, loader de modlib | l'entrée `.it` reste le bon choix (OpenMPT, Schism) ; le driver SPC700 est l'objet des tickets #6-#10 |
| `tmx2snes` | tmx2snes d'Alekmaul, réécrit sur cute_tiled | Tiled `.tmj`, bon choix |
| `font2snes`, `wav2brr` | contrats PVSnesLib (une PNG 1 bpp, un wav → brr) | simples et sains |
| `img2snes`, `palplan`, `aseprite2snes`, `sa1-patch` | OpenSNES | les trois premiers sont déjà « modernes » : quantification, planification de palettes, Aseprite |
| `cmdparser`, `lodepng` | PVSnesLib (Apache 2.0, zlib) | dans `tools/common/` |

Mais l'héritage n'est pas tant dans les binaires que dans le **geste** que
l'on demande à l'utilisateur, celui des exemples PVSnesLib :

1. écrire à la main, pour chaque PNG, une règle `make` avec les drapeaux de
   `gfx4snes` (le `starter/Makefile` le fait, et dit « copy this block for
   each new .png ») ;
2. écrire à la main un `data.asm` avec les `.incbin` et les sections
   (55 exemples sur 91 en ont un) ;
3. écrire à la main `clean-assets` ;
4. connaître l'emplacement VRAM, le format de palette, la banque, avant
   d'avoir affiché quoi que ce soit.

C'est ce geste qu'il faut remplacer, pas réécrire `gfx4snes` une troisième
fois.

### 9.3 Proposition : le manifeste d'assets comme contrat, le CLI comme porte

**a) Un manifeste d'assets déclaratif** (`assets.toml` à côté de `main.c`),
lu par `common.mk` :

```toml
[[sprite]]
name = "player"
src  = "res/player.png"      # ou res/player.aseprite : tags → AnimClip
size = 32                    # 8 / 16 / 32 / 64
bpp  = 4

[[background]]
name    = "town"
src     = "res/town.tmj"     # Tiled ; le tileset et la palette suivent
bpp     = 4
palette = "shared"           # palplan range la palette avec les autres "shared"

[[sample]]
name = "jump"
src  = "res/jump.wav"
loop = false

[[music]]
name = "theme"
src  = "res/theme.it"
```

Le build en dérive les appels aux neuf outils, le `data.asm` (sections
`ASSET_SECTION`, `.incbin`, labels), un `assets.h` avec les symboles typés
(`extern const u8 player_tiles[]; #define player_tiles_size …`), et le
nettoyage. L'utilisateur ne tape plus jamais `-s 8 -o 16 -u 16 -p -m -i`.
Les 55 `data.asm` du corpus sont la preuve que c'est générable : ils ont
tous la même forme.

Ce manifeste est le **contrat** : une fois qu'il existe, les binaires
derrière deviennent un détail d'implémentation. On peut alors, sans casser
un seul projet, fusionner `gfx4snes` / `img2snes` / `palplan` /
`aseprite2snes` / `font2snes` / `tmx2snes` en un seul `opensnes-assets`
(ou le réécrire), et le vérifier avec les suites golden existantes.

**b) Une seule porte : `opensnes`.** Le CLI existe déjà (`scripts/opensnes`,
bash : `init`, `build`, `run`, `test`, `doctor`). Il devient l'interface de
l'utilisateur et le seul nom à retenir : `opensnes init`, `build`, `run`,
`test`, `doctor`, plus `assets` (montre ce que le manifeste va produire,
avec l'empreinte VRAM / CGRAM : `asset_budget.py` le calcule déjà). Les
Makefiles restent dessous pour qui veut les voir.

**c) Des entrées natives des outils d'aujourd'hui** : Aseprite (fait),
Tiled (fait), trackers IT (fait), WAV (fait). Ce qui manque est de les
**combiner** sans que l'utilisateur orchestre (Aseprite → tuiles + palette +
métasprites + clips en une entrée de manifeste). LDtk pourrait suivre Tiled ;
ce n'est pas urgent.

**d) Deux zips au lieu d'un gros.**

| Archive | Contenu | Poids estimé |
|---|---|---|
| `opensnes_<v>_<os>_<arch>.zip` | `bin/`, `lib/`, `make/`, `templates/`, `starter/`, `opensnes`, `install-luna.sh`, le sous-ensemble `testing/` livré, `README`, `LICENSE`s | ~ 8 Mo compressé |
| `opensnes-examples_<v>.zip` | sources + assets + ROM construites, un par OS inutile | ~ 10 Mo, une fois |
| docs | en ligne (GitHub Pages depuis `docs-strict`), plus dans le zip | 0 |

`release_smoke.py` teste déjà « extraire, construire le starter, scaffolder
un projet » : il reste le juge.

### 9.4 Ce que cela change au plan du §7

Rien n'est retiré ; deux lots s'ajoutent **après** le lot 7, et le lot 4
(`make/checks/`) est précisément ce qui prépare la release légère :

| Lot | Contenu | Taille |
|---|---|---|
| **8. Release légère** | `make release` ne copie plus `examples/` ni `docs/build/html` ; cible `release-examples` ; docs publiées en ligne ; `release.yml` publie les deux archives ; `release_smoke.py` inchangé | S |
| **9. Manifeste d'assets** | `assets.toml` lu par `common.mk` (un `assets.mk` généré, Python stdlib, dans `make/checks/` ou `make/assets/`) ; `starter/` l'utilise ; migration des exemples par catégorie (les 55 `data.asm` disparaissent un à un) ; `opensnes assets` ; les règles à la main restent acceptées | L, par étapes |
| **10. `opensnes-assets`** (optionnel, plus tard) | fusion des outils d'images derrière le manifeste, golden tests comme filet | L |

Le lot 9 est le vrai changement de nature pour l'utilisateur ; il touche la
surface publique de la build (pas l'API C), donc il se fait avant ou après
le tag 1.0 selon que le propriétaire veut que « 1.0 » signifie aussi « le
geste moderne ». Mon avis : le manifeste est **additif** (les règles à la
main continuent de marcher), il peut donc arriver en 1.1 sans rien casser,
et le tag 1.0 n'a pas à l'attendre.

Deux choses à trancher par le propriétaire : le nom et le format du
manifeste (TOML est ce que luna et les manifestes de test utilisent déjà),
et le sort de `img2snes` (quantification RGB → indexé) : l'intégrer au
manifeste (`quantize = true`) ou le garder comme étape d'artiste hors build.

## 10. Complément (même jour) — les besoins d'un studio, et la famille d'outils qui en découle

Le propriétaire précise la cible : penser comme un **studio indépendant qui
produit un jeu commercial** avec ce SDK ; des outils **unifiés** dans leurs
conventions mais **dédiés** chacun à une fonction précise (un seul outil
graphique ne couvre pas tous les besoins). Ce qui suit remplace les §9.3 et
9.4 là où ils parlaient d'un manifeste global et d'un `opensnes-gfx` unique.

### 10.1 Qui est dans le studio, et ce que chacun attend

| Rôle | Ses outils à lui | Ce qu'il attend du SDK |
|---|---|---|
| Programmeur (1 à 3) | C, git, un émulateur, un débogueur | une build reproductible en une commande, des erreurs qui disent quoi faire, des budgets visibles avant d'exploser, un profileur, des tests automatiques |
| Graphiste | Aseprite, Photoshop/Krita, Tiled ou LDtk | importer **son** fichier sans apprendre les drapeaux, voir tout de suite si une palette ou une taille ne passe pas, itérer en secondes |
| Musicien / sound designer | OpenMPT, Schism, Furnace ; un DAW pour les samples | importer `.it` et `.wav`, connaître la place qu'il reste dans les 64 Ko de l'APU, entendre le résultat sur l'émulateur de référence |
| Producteur / QA | un tableau, une console, un flash cart | une ROM finalisée avec un en-tête propre, une liste de ce qui a changé, une campagne de tests rejouable, une preuve console |
| L'éditeur (Limited Run, Retro-bit, un fabricant de cartouches) | ses contraintes de fabrication | un mapper qu'il sait produire, une taille de ROM standard, la pile de sauvegarde dimensionnée, les crédits de licence des composants embarqués |

### 10.2 Les besoins, du premier jour à la mise en boîte

Numérotés pour la table du §10.3.

**Démarrer**
- B1. Installer le SDK en une étape par OS, sans dépendance cachée (Python, make et clang sont déjà demandés : à vérifier par `doctor`).
- B2. Créer un projet qui tourne en une commande, avec un dépôt git et une CI prêts.

**Produire des assets (la boucle quotidienne du graphiste et du musicien)**
- B3. Sprites : feuille PNG ou `.aseprite` → tuiles 4 bpp, palette, table de métasprites, clips d'animation depuis les tags.
- B4. Décors : tileset + tilemap depuis PNG, avec déduplication, flips, priorités, et la contrainte des 8 palettes de 16.
- B5. Niveaux : Tiled (`.tmj`) aujourd'hui, LDtk demain, avec les couches, les objets et les collisions.
- B6. Polices et texte : police bitmap, table de chaînes, **localisation** (plusieurs langues, un texte hors code).
- B7. Palettes : planification entre assets (qui partage quoi), quantification RGB → 15 bits, aperçu du rendu réel.
- B8. Images plein écran et effets : titre, cinématiques, Mode 7, HiColor (nos `hicolor64.py` et `m7ptables.py` sont des besoins de studio déguisés en scripts de mainteneur).
- B9. Samples : WAV → BRR avec boucle, aperçu du poids en APU.
- B10. Musique : `.it` → soundbank, par banque, avec la liste des samples partagés.
- B11. Les réglages d'import **vivent à côté de l'asset**, versionnés, relus par l'outil et par la build ; l'artiste ne touche jamais au Makefile.

**Construire et mesurer**
- B12. Une build incrémentale, reproductible (même sources → même ROM, le `check_corpus_fresh` le garantit déjà pour nous), en CI sur les trois OS.
- B13. Les budgets à chaque build : ROM par banque, RAM plain et FAR, VRAM / CGRAM / OAM, temps VBlank, taille APU. Tout cela existe en morceaux (`symmap`, `asset_budget`, `budget.py`, `nmi_budget`) ; le studio veut **un** rapport.
- B14. Un débogage source : symboles, points d'arrêt, inspection PPU/APU. luna le fait (MCP, `.dbg`) ; le SDK doit l'exposer en une commande.
- B15. Un profileur par fonction et par frame. luna `profile` existe ; même remarque.

**Tester**
- B16. Des tests automatiques : scénarios d'entrée → assertions mémoire (nos manifestes luna), régression visuelle (fbhash), audio (hash WAV). Le studio en veut pour **son** jeu, pas pour notre corpus : `project_test.py` est l'embryon.
- B17. Le passage PAL / NTSC, la RAM aléatoire au démarrage, les deux manettes, la souris, le Super Scope : des campagnes rejouables (`hardware_preflight` est notre version).
- B18. La console réelle : un flash cart, une grille, des photos (`docs/HARDWARE_VERIFICATION.md`).

**Livrer**
- B19. Finaliser la ROM : titre, code jeu et code éditeur, région, mapper, vitesse, taille SRAM, somme de contrôle, taille standard (4 à 32 Mbit), vérification que tout est cohérent. Aujourd'hui dispersé entre `hdr*.asm`, `ROM_REGION`, `ROM_BANKS`, wlalink et `sa1-patch`.
- B20. Les contraintes de fabrication : un éditeur produit du LoROM / HiROM avec ou sans SRAM ; SA-1 et Super FX ne se fabriquent pas en série aujourd'hui. Le SDK doit le dire au moment du choix, pas à la fin.
- B21. Les obligations de licence du code embarqué dans la ROM (bibliothèque MIT, driver SNESMOD, tout ce que `ATTRIBUTION.md` liste) : un fichier de crédits généré par build, prêt pour le manuel.
- B22. Des sauvegardes : dimensionnement SRAM, versionnage du format, somme de contrôle, migration ; un outil pour lire / écrire un `.srm` de test.
- B23. Un changelog de ROM entre deux builds (ce qui a changé : taille, assets, symboles) pour le producteur.

### 10.3 La famille d'outils : une fonction, un outil, des conventions communes

**Les conventions, communes à tous (c'est le sens d'« unifié »)** : un préfixe
unique, des sous-commandes nommées, options longues, `--help` complet,
`--json` pour la sortie machine, le même format de fichier de réglages à
côté de l'asset, les mêmes codes de retour, les mêmes messages (fichier,
ligne, quoi faire), `inspect` partout (que contient ce fichier, combien il
pèse, dans quelle contrainte il ne rentre pas).

| Outil | Fonction précise | Besoins | Ce qu'il absorbe | État |
|---|---|---|---|---|
| `opensnes` | le projet : `init`, `build`, `run`, `test`, `doctor`, `budget`, `release` | B1, B2, B12, B13 | `scripts/opensnes` (bash), `project_test.py`, les cinq `check_*` de la build | existe, à promouvoir |
| `opensnes-sprite` | feuille ou `.aseprite` → tuiles, palette, métasprites, clips | B3, B11 | `gfx4snes -P`, `aseprite2snes` | à fusionner |
| `opensnes-tileset` | PNG → tileset + tilemap, dédup, flips, palettes par tuile | B4, B11 | `gfx4snes -m`, `palplan` pour la répartition | à fusionner |
| `opensnes-level` | Tiled / LDtk → cartes, objets, collisions | B5 | `tmx2snes` | à renommer, LDtk à ajouter |
| `opensnes-text` | police bitmap + table de chaînes + langues | B6 | `font2snes` ; la table de chaînes est **nouvelle** | à créer autour de l'existant |
| `opensnes-palette` | planifier, quantifier, prévisualiser | B7 | `palplan`, `img2snes` | à fusionner |
| `opensnes-image` | plein écran, Mode 7, HiColor, pseudo-hires | B8 | `hicolor64*.py`, `m7ptables.py`, le mode 7 de `gfx4snes` | à créer, en promouvant les scripts |
| `opensnes-sample` | WAV → BRR | B9 | `wav2brr` | à renommer |
| `opensnes-music` | `.it` → soundbank, samples partagés, poids APU | B10 | `smconv` | à renommer, `inspect` à ajouter |
| `opensnes-rom` | finaliser et vérifier l'en-tête, la taille, le mapper, la somme ; avertir sur B20 ; générer les crédits B21 | B19, B20, B21, B23 | `sa1-patch`, les `hdr*.asm`, la logique de `ROM_REGION` / `ROM_BANKS` | à créer |
| `opensnes-save` | lire, écrire, vérifier un `.srm` | B22 | rien (le module `sram` côté lib) | à créer, petit |
| luna | déboguer, profiler, tester, préflight | B14 à B18 | déjà le partenaire | exposer via `opensnes debug`, `profile`, `test` |

Douze noms, un par fonction ; le studio en utilise trois par jour
(`opensnes`, son outil d'asset, luna) et les autres une fois par projet.

### 10.4 Ce qui est nouveau par rapport aux lots 8 à 10

- Le pivot n'est plus un manifeste global mais **un fichier de réglages à
  côté de chaque asset** (`player.png.import` ou `.toml`, le format est
  secondaire), écrit par l'outil la première fois, lu par la build. La règle
  `make` générique remplace les 55 `data.asm` et les blocs copiés du
  `starter/`.
- Trois outils n'ont pas d'ancêtre : `opensnes-rom`, `opensnes-text` (la
  table de chaînes), `opensnes-save`. Ce sont les besoins « commercial » que
  l'héritage PVSnesLib n'a jamais couverts.
- `hicolor64*.py` et `m7ptables.py` changent de statut : de scripts de
  mainteneur à embryons d'`opensnes-image`.
- Le contrat de fusion : chaque outil absorbé garde sa suite golden, qui
  devient la suite de l'outil nouveau (`opensnes-sprite` doit produire les
  mêmes octets que `gfx4snes -P` sur les fixtures). Pas de réécriture
  à l'aveugle.

### 10.5 Ordre proposé

1. **Conventions d'abord** : une page `docs/tools/CONVENTIONS.md` (préfixe,
   sous-commandes, `--json`, fichier de réglages, messages) et un squelette
   commun en C (`tools/common/cli.c` : parsing, aide, erreurs, JSON) que
   `cmdparser` ne fournit pas. Un seul outil le valide : `opensnes-sample`
   (le plus petit).
2. **Le son** : `opensnes-sample`, `opensnes-music` avec `inspect`.
3. **Le graphisme** dans l'ordre d'usage : `opensnes-sprite`,
   `opensnes-tileset`, `opensnes-palette`, puis `opensnes-text`,
   `opensnes-image`.
4. **`opensnes-level`**, puis LDtk.
5. **`opensnes-rom`** et `opensnes-save` : courts, et ce sont eux qui
   parlent à l'éditeur.
6. **`opensnes` en vrai programme**, quand les outils existent : `build`
   lit les fichiers de réglages, `budget` agrège, `release` appelle
   `opensnes-rom`.

Les lots 1 à 7 du §7 (le rangement du dépôt) restent préalables : on ne
construit pas douze outils sur trois racines de fixtures et deux README
faux. Le lot 8 (release légère) peut se faire dès maintenant.

Questions au propriétaire : le préfixe (`opensnes-` est long mais sans
ambiguïté ; `osn-` est court) ; le format du fichier de réglages ; si
`opensnes-text` avec localisation est dans la cible 1.x ou plus loin ; et
si la 1.0 attend cette famille ou si elle arrive en 1.x, additive (mon
avis : 1.x, les anciens outils restant livrés le temps d'une version avec
un message de renvoi).

## 11. Décisions du propriétaire (2026-10-05) et leur conséquence : un chemin utilisateur sans Python

Décidé : préfixe **`opensnes-`**, fichiers de réglages en **TOML**, la
famille d'outils arrive en **1.x** (additive, la 1.0 ne l'attend pas). Et la
règle qui gouverne tout le reste, écrite dans `.claude/rules/two_audiences.md` :

> Deux populations. Le **développeur de jeu** reçoit des outils compilés,
> prêts à l'usage, sans Python ni autre runtime à installer. Le
> **contributeur** est dans le dur et accepte Python, uv, ortools, ce qu'il
> faut. Rien de ce que la build d'un projet utilisateur exécute n'est un
> script interprété.

### 11.1 Où le chemin utilisateur dépend de Python aujourd'hui

Mesuré sur `make/common.mk` et les scripts livrés (`grep -n python3`) :

| Site | Rôle | Remplacement | Taille |
|---|---|---|---|
| `common.mk:130` `ROMSIZE` | log2 de la taille ROM pour l'en-tête | table `ifeq` en make pur (cinq valeurs : 256 Ko → `$08` … 4 Mo → `$0C`) | S, faisable aujourd'hui |
| `common.mk:134` `GSU_RAM_SIZE_VAL` | log2 de la RAM GSU | même table | S |
| `common.mk:523` `check_upgrade.py -q` | sur une erreur de compilation, nomme un identifiant 0.x retiré | garder, mais derrière `command -v python3` : c'est une aide de migration 0.x → 1.0, pas une étape de build | S |
| `common.mk:649,667,684` `symmap.py` (bank $00, budget RAM, terminateur data-init) | les trois ratchets post-link, obligatoires | **`opensnes-rom check <sym>`** en C : un lecteur de `.sym` (sections, labels, ramsections) et les trois règles | M |
| `common.mk:694` `asset_budget.py --oneline` | ligne d'information VRAM / CGRAM (`\|\| true`) | `opensnes budget` en C, ou retiré du link et gardé comme commande | S |
| `common.mk:703` `check_bank_reads.py` | lecture C sans banque d'une donnée en banque $01+ (silencieux) | `opensnes-rom check`, même lecteur de `.sym` + les `.asm` générés | M |
| `common.mk:724` `check_nmi_wram_race.py` | port WRAM $2180 dans un callback NMI (silencieux) | `opensnes-rom check` (graphe d'appels sur les `.asm` générés) | M |
| `common.mk:749` `project_test.py` | `make test` d'un projet utilisateur | **`luna test`** directement : les manifestes sont natifs ; `opensnes test` (C) ne fait que les lister et appeler luna | S, luna fait déjà le travail |
| `bin/cc65816`, `bin/opensnes`, `scripts/install-luna.sh` | bash | toléré pour l'instant : `make` sur Windows signifie MSYS2, qui a bash. La ligne rouge est Python. `cc65816` en vrai binaire C est souhaitable (lot à part) | M |
| `scripts/opensnes doctor:450` | exige python3 et le dit | disparaît avec les lignes ci-dessus | — |

Dix appels, un seul outil pour les absorber : **`opensnes-rom`** devient
aussi le vérificateur post-link, ce qui est cohérent avec sa fonction
(« la ROM est-elle bonne ? »). Les scripts Python d'origine restent dans
`devtools/` pour le contributeur tant que l'équivalent C n'a pas prouvé, sur
le corpus, qu'il rend le même verdict ; puis ils sont supprimés (le même
contrat de fusion que pour les outils d'assets : la suite existante juge).

### 11.2 Ce que cela change au plan

- **Lot 4 (`make/checks/`) est remplacé** : déplacer cinq scripts Python
  pour les livrer n'a plus de sens si l'on ne livre plus de Python. Le lot
  devient « **`opensnes-rom check`** » et passe après le squelette CLI
  commun (§10.5, étape 1). En attendant, `RELEASE_DEVTOOLS` reste tel quel.
- **Lot 8 (release légère)** se précise : le zip utilisateur contient
  `bin/` (binaires), `lib/`, `make/`, `templates/`, `starter/`,
  `install-luna.sh`, les licences — et, le temps de la transition, les
  scripts Python que `common.mk` appelle encore, marqués comme tels dans
  le README du zip. Exemples et HTML partent ailleurs dès maintenant.
- **Lot nouveau, immédiat** : `ROMSIZE` et `GSU_RAM_SIZE_VAL` en make pur
  (deux lignes), `check_upgrade` derrière `command -v`. Trois des dix
  appels tombent sans rien construire.
- **Le test utilisateur** passe sur `luna test` natif : `project_test.py`
  ne sert plus qu'au corpus du SDK, où il peut rester Python.
- **Le gate contributeur** ne change pas : `make tests`, `make lint`, les
  sentinelles, tout reste Python et libre d'évoluer. La frontière est le
  zip, pas le dépôt.

### 11.3 Mesure du succès

Trois conditions, vérifiables mécaniquement et à inscrire au sentinel
quand elles seront vraies : `grep -c python3 make/common.mk` rend 0 (hors
la branche `command -v`) ; `release_smoke.py` construit le starter et un
projet scaffoldé dans un conteneur **sans** `python3` ; `opensnes doctor`
ne nomme plus Python.

## 12. Fait le 2026-10-05 (même séance)

| Lot | Commit | Ce qui a changé |
|---|---|---|
| Décisions et règle | `2ff2090a` | ce rapport, `.claude/rules/two_audiences.md` |
| 2. Élaguer | `85bae8bd` | `gen_hud_bar/`, `brr2it/`, `devtools/font2snes/`, `pyproject.toml`, `stress/mcp_probe.py`, `stress/mcp_sweep.py` supprimés ; branche `vendor/` retirée de `find_luna` ; `stress/README.md` réécrit sans second émulateur ; `ROADMAP.md` ne coche plus `check_mvn` ni `benchmark`. **Retrait du plan** : `benchrom/` et `b2_deref/` restent, `lib/ARCHITECTURE.md` les désigne comme l'instrument de mesure à relancer par module |
| 1. Dire la vérité | `5590d8bd` | `tools/README.md`, `devtools/README.md` réécrits depuis l'inventaire ; README pour `sa1-patch/` et `fuzz/` ; `hicolor64hires.py` supprimé (son exemple est archivé, plus rien ne le nommait) |
| 3. Porte locale = CI | `1cb2e3a9` | `make test-devtools` (six tests unitaires), dans `make lint` et appelé tel quel par `lint.yml` |
| `ROMSIZE` en shell | `98a1fcdb` | `ROMSIZE` et `GSU_RAM_SIZE_VAL` en arithmétique shell ; `check_upgrade` derrière `command -v python3` ; 91 ROM identiques octet pour octet. Reste **sept** appels `python3` sur le `make` d'un utilisateur, tous post-link (`symmap` ×3, `asset_budget`, `check_bank_reads`, `check_nmi_wram_race`, `project_test`) |
| 8. Release légère | `20a02de4` | le zip SDK ne contient plus ni exemples ni HTML ; `make release-examples` produit `opensnes-examples_<v>.zip` (4 Mo, une fois par version, attaché par `release.yml` depuis la jambe linux x86_64) ; `GETTING_STARTED.md` et les notes de release renvoient à l'archive et au site ; `release-smoke` vert sur le zip |

Correction d'une estimation du §9.1 : en **compressé**, exemples et HTML
pesaient 14 Mo d'un zip de 40 Mo (v0.46.0 arm64), pas les deux tiers ;
ce qui reste, 23 Mo, ce sont les **17 binaires statiques** de `bin/`
(1,5 à 2 Mo chacun compressés : quatre wla, cproc, qbe, neuf outils,
`opensnes`). Le prochain levier sur la taille n'est donc pas le contenu
mais le nombre de binaires — ce que la fusion en `opensnes-*` (douze
outils dont plusieurs fusionnent gfx4snes, img2snes, palplan,
aseprite2snes, font2snes) réduit mécaniquement, et la question de la
liaison statique (chaque outil embarque sa libc) à poser pour 1.x.

Restent du §7 : lots 5 (`testing/lib/`), 6 (`testing/fixtures/`), 7
(outils C : `tool.mk`, `third_party/`, runner golden commun) ; puis la
famille du §10 dans l'ordre du §10.5, `opensnes-rom check` en tête pour
retirer les sept derniers appels Python.

### Suite de la séance (lots 5 à 7)

| Lot | Commit | Ce qui a changé |
|---|---|---|
| 5a. `testing/` | `75d0d942` | `tools/luna-test` → `testing/` ; 384 fichiers suivent (Makefile, common.mk, workflows, `install-luna.sh`, CLI, sentinel, règles, skills, agents, docs, README d'exemples) ; les 137 manifestes et les Makefiles des ROM de stress perdent un niveau de `../` ; scope de commit `testing` ajouté |
| 5b. `testing/lib/` | `d9632561` | `lib/luna.py` (`find_luna`, `LUNA_VERSION`, `REPO_ROOT`, `firmware_dir`), `lib/corpus.py`, `lib/probes.py` (ex `probes/lib.py`) ; chaque importeur fait un seul `sys.path.insert` de `testing/` puis `from lib import …` ; le zip copie `testing/lib/`. **Retrait** : les trois lecteurs de `.sym` restent : `SymbolTable` avale aussi les 218 lignes `[definitions]` d'un `.sym`, ce que `rom_coverage.load_labels` ne doit pas faire ; `check_bank_reads` est livré et doit rester autonome (il passe en C de toute façon) ; `release_smoke.find_luna` cherche dans l'arbre extrait du zip, pas dans le dépôt, ce n'est pas un doublon |
| 6. `testing/fixtures/` | `41bedec5` | les 20 projets ROM sous une racine (`libtests*`, `compiler/<a6_farptr…>`, `stress/<hwmath…>`, `benchrom`) ; `FIXTURES_LIB` / `FIXTURES_COMPILER` / `FIXTURES_STRESS` et `make fixtures` dans le Makefile, `test-lib` en boucle sur `FIXTURE_TESTS` ; `testing/fixtures/README.md` dit ce que chaque fixture épingle ; `rom_coverage` 325/325 inchangé |
| 7a. `tool.mk` | `89ebbbaa` | les neuf Makefiles d'outils deviennent cinq à huit lignes sur `tools/tool.mk` ; une macro `TOOL_VERSION` / `TOOL_BUILD_DATE` dans les sources (quatre outils lisaient `VERSION`, cinq `__BUILD_VERSION`) ; `tools/third_party/` (lodepng, cmdparser, stb_image, cute_tiled) ; `tmx2snes/src/` ; gfx4snes et img2snes gardent deux classes `-Wextra` héritées tues ; 8/8 suites golden sur les nouveaux binaires |
| 7b. `tools/tests/golden.py` | *(ce commit)* | le runner golden unique ; les huit `run_golden.py` sont des tables de cas (`expect_outputs`, `expect_refused`, `expect_stdout`, `check`), 520 lignes au lieu de 900, mêmes verdicts |

Trouvé en chemin et retiré : `testing/stress/roms/` (huit ROM malformées
non suivies, aucun lecteur dans le dépôt). Le plan du §7 est fait ; la
suite est le §10.5 (conventions, `opensnes-sample`, `opensnes-rom check`).

### Ouverture de la famille (même séance, suite)

| Quoi | Commit | Détail |
|---|---|---|
| Conventions | `e862dafa` | `docs/tools/CONVENTIONS.md` : la famille (onze outils plus luna), l'invocation (sous-commandes, options longues, `--help`, `--json`, quatre codes de sortie), la forme des messages, le fichier de réglages TOML à côté de l'asset (`<asset>.toml`, première clé `tool`), les sorties déterministes avec en-tête généré, les règles d'implémentation |
| Correctifs CI du lot 7 | `c772e55b`, `217de1ae`, `c26a6b59`, `6a77e6d1` | cppcheck lisait désormais `third_party/` et `tmx2snes/src/` : macros `TOOL_*` fournies, `cute_tiled.h` supprimé comme les autres décodeurs vendus, deux `%d` d'unsigned corrigés dans tmx2snes ; le corpus de fuzz `tiled` lisait ses fixtures via la variable qui nomme désormais `third_party/` ; le binaire `tmx2snes` était entré dans git avec la disparition de son `.gitignore` local (reste dans l'historique de `89ebbbaa`, ignoré depuis) |
| `tools/common/cli.c` | `7117a55c` | l'implémentation des conventions, partagée : parsing (sous-commandes, options longues et courtes, `--`), aide générée, messages, chemins de sortie (`--out` crée son dossier), lecture et écriture du fichier de réglages (chaînes, entiers, booléens, `[a, b]`, clé inconnue refusée, outil différent refusé), écrivain JSON |
| `opensnes-sample` | `7117a55c` | `encode` et `inspect` ; mêmes octets que `wav2brr` (suite golden comparée aux goldens de wav2brr, 13 cas) ; `wav.c` fuzzé ; page `docs/tools/opensnes-sample.md` ; `wav2brr` reste livré une version |

| `opensnes-music` | `5e624ed9` | `bank`, `spc`, `inspect` sur le convertisseur de smconv (itloader, it2spc, brr) ; mêmes octets que `smconv -s -n -p` (suite golden comparée aux goldens de smconv, 7 cas) ; `inspect` donne les 57 957 octets qu'un module peut prendre, le chiffre de `smconv -V` ; page `docs/tools/opensnes-music.md`. Reste à faire : les diagnostics propres de la bibliothèque smconv parlent encore avec son nom ; une sortie de messages par callback les ramènerait à la forme de la famille |
| cppcheck sur `tmx2snes/src` | `c26a6b59`, `d231fbc6` | deux `%d` d'unsigned, deux boucles qui lisaient l'octet avant de tester l'indice — des remarques que le lint ne voyait pas tant que le fichier était hors de `src/` |

| `opensnes-rom check` | `b24ccda6`, `12894c64`, `99d1fd76` | les cinq vérifications post-link en C : lecteur de `.sym` (labels avec repli de banque, définitions, sections, ramsections), ratchet banque $00, bande RAM C et bande FAR, sentinelle data-init, lectures sans banque dans les `.c.asm`, graphe d'appels NMI, inventaire d'assets ; comparé aux scripts Python sur les 99 ROM construites : mêmes verdicts, mêmes chiffres ; `common.mk` l'appelle à la place des cinq ; il reste dans `common.mk` deux `python3` : l'aide 0.x derrière `command -v`, et `project_test.py` sous `make test` d'un projet |

| `opensnes-sprite` | `aa21d513` | le gros morceau : `sheet` (feuille → tuiles en ordre VRAM OBJ, palette, `.inc`/`_data.as`, table `_meta.inc` avec `--metasprite W H`, `--flip`), `anim` (export Aseprite → `_anim.h`), `inspect`. Les modules de gfx4snes sont liés comme bibliothèque derrière un `diag.c` qui parle dans la forme de la famille ; le convertisseur d'aseprite2snes est devenu `anim.c` (chemin d'erreur par `longjmp`, crochet d'avertissement), que l'outil 0.x appelle lui-même — son golden est inchangé. Suite : chaque sortie `sheet` comparée aux goldens de gfx4snes octet pour octet, l'en-tête `anim` à celui d'aseprite2snes à partir de sa deuxième ligne (la première nomme le générateur), 13 cas. Hors périmètre, pour `opensnes-tileset` : le chemin « carte » de gfx4snes (`-m`, modes 1/5/6/7, `-a`, `-y`) |

| `opensnes-tileset` | `1c6ca57c` | le chemin « carte » de gfx4snes : `convert` (tuiles dédupliquées, carte modes 1/5/6/7, pages 32×32, décalage, priorité, réarrangement de palette, palette imposée), `inspect` (borne avant déduplication, taille de carte, banques touchées). Suite : le golden `bg` de gfx4snes octet pour octet, son oracle de pixels sur `banks.png` avec et sans `--rearrange`, les trois refus matériels. À noter : certains messages de la bibliothèque citent encore les drapeaux de gfx4snes (« deduplicate with -F ») ; à reprendre quand gfx4snes sera retiré |

| La règle générique de la build | `700d5580`, `26d47d03` | `common.mk` : `ASSET_TOML` ramasse chaque `*.toml` / `res/*.toml` qui nomme un outil `opensnes-*`, un tampon `.done` par fichier lance `opensnes-<outil> <table> -q <asset>` avant tout objet C ou ASM, et `assets_gen.asm` rassemble les fragments `_data.as` dans une `ASSET_SECTION` (`opensnes-sample` écrit le sien désormais). Le `starter` tient sur `res/player.png.toml` seul, l'exemple Aseprite sur deux fichiers de réglages ; plus de `data.asm` ni de règle de conversion. Mêmes baselines visuelles et WRAM. Trois pièges rencontrés : `.DEFAULT_GOAL` (mes règles précédaient `all:`), le double `$$` d'une règle hors `define`, et deux `.toml` de même racine qui incluaient deux fois le fragment. Et une faute de procédure : j'ai poussé le commit avec « release-smoke green » alors que la fumée venait d'échouer sur un zip construit avant le commit du `.toml` du starter ; relancée après, elle est verte. La fumée se lance sur l'arbre commité, pas avant |

Deux fautes de ma main dans cette séance, à retenir : un enchaînement
`&&` interrompu par une commande qui échouait a fait sauter en silence les
éditions qui suivaient (le câblage d'`opensnes-rom` dans `tools/Makefile`,
le Makefile racine et le `.gitignore`), et le binaire de l'outil est entré
dans le commit `b24ccda6` faute de cette ligne d'ignore — comme `tmx2snes`
au lot 7a. Les deux binaires restent dans l'historique (pas de force-push)
et sont ignorés depuis. Règle pour la suite : la ligne `.gitignore` d'un
nouvel outil se commite **avant** son premier `make`, et les éditions de
câblage se vérifient par `git status` avant le commit, pas après le push.

Trouvé en chemin : `asset_budget.py --oneline .` avec un chemin relatif
mesurait le dépôt entier (il préfixait le chemin par la racine du dépôt) ;
`common.mk` lui passait `$(CURDIR)` absolu, donc la build n'était pas
touchée. Le C ne reproduit pas ce comportement.

Ce que le premier outil a appris pour les suivants : le fichier de réglages
se lit **par entrée** (une `cli_ctx` par asset), un `--out` doit créer son
dossier, le JSON d'un échec doit nommer l'entrée et le code, et la suite
golden d'un outil fusionné pointe sur les goldens de l'outil absorbé plutôt
que d'en copier les octets. Suivant dans l'ordre du §10.5 : `opensnes-rom check`, qui retire les
sept derniers appels Python de la build d'un utilisateur ; puis
`opensnes-sprite` (gfx4snes -P + aseprite2snes), le plus gros.

### Journal (suite, 2026-10-05, migration des exemples)

- `tools/common/incfile.c` : les deux outils graphiques écrivent la colle
  dans la langue d'`asset.h` (`<nom>_tiles/_pal/_map` + `DECLARE_*_ASSET`),
  un fragment `_data.as` autoporteur (une `ASSET_SECTION` par bloc, parts de
  32 Ko au-dessus d'une banque), `extern` nus pour LZ77 et les blocs coupés.
  `assets_gen.asm` n'est plus qu'une liste d'`.include`.
- `cli_path()` : une option `FILE` lue dans un fichier de réglages est
  relative à ce fichier (le `palette = "town_fixed.pal"` du RPG), et
  `--save` l'écrit ainsi. La convention était écrite, pas appliquée.
- Un défaut de compilateur révélé par la colle incluse depuis deux fichiers
  (statiques de portée fichier en labels globaux) : corrigé dans cproc,
  voir le journal de la note sur les exemples.
- Des 50 `data.asm` des exemples, 28 ont disparu, 13 ne portent plus que ce
  qu'aucun outil ne convertit (cartes tmx2snes, tables HDMA et de sinus,
  helpers asm, polices binaires, sections RAM) et 9 n'avaient rien à
  convertir (images SPC700, `.brr` sans source, `.dat`, `.pic` sans PNG).
  `opensnes-level` est la prochaine marche.
- `opensnes-level` livré (6/11 outils de la famille) : le convertisseur de
  tmx2snes extrait en `tools/tmx2snes/src/level.c` (erreurs en buffer par
  `longjmp`, progression par callback), la colle `.inc` / `_data.as`, un
  `inspect` sans écriture, les goldens de tmx2snes repris octet pour octet.
  La règle générique convertit un niveau après les autres assets (il lit le
  `.map` du tileset). `map_scroll` et `tiled` perdent leur dernier
  `data.asm` ; `mapandobjects`, sans source Tiled, garde ses binaires.
- **Erreur de méthode, 2026-10-05 soir** : le workflow « Build & Release » était
  rouge sur macOS et Windows depuis l'arrivée d'`opensnes-rom` (`strset`
  heurte la libc de mingw ; `strncasecmp` sans `<strings.h>` sur macOS), et je
  n'ai regardé que « Lint » pendant quatre pushes. Règle : après un push, lire
  les trois workflows, et pour « Build & Release » les trois OS — un outil C
  nouveau se compile ici sous gcc 16 et là-bas sous clang et mingw.
- **Même soir, deux pushes partis sans vérification** : `make release-smoke
  2>&1 | tail -1 && git commit && git push` — le code de sortie est celui de
  `tail`, et la chaîne a continué sur un échec. Règle : `set -o pipefail`, ou
  séparer la vérification du commit ; et `make release-smoke` juge le zip
  *existant* — reconstruire le zip (`make release`) après tout changement
  de `common.mk` ou du `Makefile`, sur un `release/` vide.

### Journal (suite, 2026-10-05, nuit : `opensnes-text`, `-palette`, `-image`)

- Trois outils de plus (9/11). `opensnes-text font` : une image des 96
  glyphes → tuiles en ordre de glyphe + rampe de gris + colle ; source
  indexée (l'index tel quel), grise ou RVB (rang de luminosité). Les tuiles
  et palettes de font2snes reproduites octet pour octet, à 2 et 4 bpp, par
  les deux chemins (gris et indexé) ; l'en-tête C de `-c` n'a pas de
  successeur (la famille livre de l'`.incbin`). La table de chaînes reste à
  écrire : question au propriétaire toujours ouverte (1.x ou plus tard).
- `opensnes-palette plan` : asset composé (`palettes.toml`, `bg = [...]`,
  `sprite = [...]`), mêmes macros `PAL_<NOM>_CGRAM/_SLOT/_COLORS` et même image
  CGRAM que palplan (le nom = la racine du fichier `.pal`, ce qui était déjà
  le cas de tous les manifestes) ; `quantize` = img2snes (`--palette` accepte
  un `.pal`, pas seulement une PNG) ; `inspect` sur les `.pal`. La règle
  générique convertit un plan après les images (`LATE_STAMPS`, qui absorbe
  `LEVEL_STAMPS`).
- **Trouvé en chemin** : le tri du quantificateur (img2snes) départageait les
  ex æquo selon le `qsort` de la libc — glibc 2.43 n'est pas stable — donc une
  même image donnait des octets différents selon l'OS, alors que les goldens
  ne tournent que sous Linux. Départage par l'index désormais ; les deux
  goldens d'img2snes re-enregistrés (erreur RMS contre la source : 21,48 →
  21,56 à 16 couleurs, 48,25 → 48,25 à 4). Première correction d'un outil 0.x
  entraînée par le contrat « mêmes octets partout » de la famille.
- `opensnes-image` : `hicolor` (le contrat de krom : 896 tuiles séquentielles,
  une palette par segment 64×8, coupe médiane + 3 passes de k-means — RMS
  contre la source 2,46 pour 2,44 au script Pillow, 377 couleurs à l'écran
  pour 357) et `perspective` (tables HDMA cos / sin / −sin). **La note
  d'archive disait l'arrondi de krom « non épinglé »** : faux, les 32 256
  entrées sont `round(trig·20480/ligne)` et l'entrée la plus proche d'une
  frontière d'arrondi en est à 0,0005 — aucune libm ne peut en basculer une.
  Les tables de krom deviennent le golden de l'outil et l'exemple les
  regénère à la build : ROM identique à l'octet près. `hicolor_1792` est
  requantifié (baseline visuelle et blocs CGRAM du manifeste re-épinglés, flux
  WRAM inchangé) ; `rpg` ROM identique. `hicolor64.py` et `m7ptables.py`
  supprimés : plus aucun exemple ne dépend d'un script Python pour ses assets.
- Reste de la famille : `opensnes-save` et le vrai programme `opensnes` ; la
  table de chaînes de `-text` ; le dernier `python3` de `common.mk`.
