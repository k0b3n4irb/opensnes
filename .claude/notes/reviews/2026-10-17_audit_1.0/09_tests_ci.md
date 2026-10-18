# 09 — Tests et intégration continue

Expert tests et CI · 2026-10-17 · HEAD audité `73a3fdfe` (`develop`), luna épinglé v1.38.0 (dernier tag du dépôt luna local, `git -C ~/workspace/luna tag` → `v1.38.0`).

## Périmètre couvert

- Lu : `Makefile` (cibles `tests`, `test-manifests`, `test-pal`, `fuzz`), les 10 workflows de `.github/workflows/`, `testing/` (runners, `manifest.toml`, 138 manifestes, `baselines/`, rapports), `.claude/rules/testing.md` et `luna_tooling.md`, `silent_defects_log.md` (80 lignes datées), `D_testing.md` et `2026-10-04_reste_a_traiter.md` (lettre D).
- Lancé : `make tests` complet, chronométré, dans une copie privée du worktree (`scratchpad/wt`) : **270,7 s mur, 815 s CPU, sortie 0** (8 cœurs aarch64, machine partagée avec les autres experts). Deux lancements dans le worktree partagé ont échoué à cause des builds concurrents des autres experts (voir 09-9).
- Contrôles négatifs (défaut injecté puis retiré, dans la copie privée) : DMA VRAM en affichage actif, appel public retiré d'un exemple, dépendance de module retirée, délai inaudible dans un exemple audio.
- Mesuré : re-captures des références depuis le 2026-10-05 (`git log`), couverture des fixtures (`rom_coverage.public_functions()` croisé avec les sources des fixtures), versions de provenance des références, comportement des portes sur une sélection vide.
- Pas mesuré : les durées réelles des jobs CI. Le mandat interdit `.env` et toute action réseau, donc pas de `gh run view`. Les seules durées que je donne pour la CI sont les `timeout-minutes` des workflows.

## Points forts

1. **La suite est rapide et déterministe.** `make tests` complet prend 4 min 31 s en local (`WALL=270.67 USER=815.29 EXIT=0`). Le parallélisme `LUNA_JOBS` est réel : CPU/mur ≈ 3. Je n'ai trouvé dans l'historique aucun test instable : `git log --since=2026-09-01 --grep='flak\|nondetermin\|intermittent'` ne renvoie rien qui porte sur le harnais. Avec un émulateur déterministe, un rouge est un vrai rouge.
2. **Les portes que j'ai testées échouent quand il faut** (contrôles négatifs, tous annulés ensuite) :
   - `vram_dma_blank.py` : j'ai ajouté une `dmaCopyVram` après une boucle d'attente de 3000 tours dans `sprites/simple_sprite`. Résultat : `FAIL … 6272 VRAM byte(s) written during active display (first: frame 5 line 57 …)`, exit 1.
   - `rom_coverage.py` : j'ai retiré l'unique appel à `hdmaWaterRipple` (`hdma/hdma_helpers/main.c:213`). Résultat : `NEW never-executed public function: hdmaWaterRipple (hdma.h)`, exit 1, en 25 s. Le manifeste `dma_hdma_helpers_effects.toml` échoue aussi (`values.hdma_wave_amplitude: 0x0 violates eq 0x6`).
   - `devtools/link_modules.py` : j'ai vidé `_DEP_fixed32`. Résultat : `FAIL fixed32 does not link alone … Reference to an unknown label "math_sine_table"`, exit 1, en 3 s.
3. **L'oracle différentiel du compilateur est celui qui trouve.** Dans `silent_defects_log.md`, une douzaine de défauts du compilateur ont été trouvés par `difftest.py` / `difftest_stmt.py` (lignes 88-113). La chasse nocturne `compiler-hunt.yml` balaie 3000 nouvelles graines par test et par nuit.
4. **La couverture mesurée de l'API est complète, et c'est un cliquet.** Sortie de ce jour : `ROM coverage: 344/344 public functions executed by 97 ROMs; 0 never`, `never_executed.txt` vide. La garde firmware que demandait la rec 7 du 03/10 existe : `rom_coverage.py:379-392` échoue en local quand une fonction n'est exécutée que par une ROM DSP-1 et n'est pas listée.
5. **Chaque exemple a un manifeste, et chaque exemple qui lit la manette a un script d'entrée.** Mesure ce jour (script `tomllib`) : 85 exemples sur 85 ont un manifeste ; 55 ont un manifeste avec entrée, et ils couvrent les 53 exemples dont `main.c` lit une manette. La souris et le Super Scope sont rejoués (`mouse.toml`, `superscope.toml`). Le « 36 exemples sans script d'entrée » des audits précédents n'est plus vrai.
6. **Le pin luna n'est plus en retard.** v1.38.0 est épinglée depuis `1ee6e9d9` (2026-10-10), et c'est le dernier tag de luna. `gen_luna_doc.py --check`, lancé par `make tests`, garantit que `docs/tools/luna.md` est l'aide du binaire épinglé.
7. **La CI garde son verdict sur `develop`** (constat D PF2 / PF12 du 03/10) : `cancel-in-progress: ${{ github.ref != 'refs/heads/develop' }}` dans `opensnes_build.yml:32`, `lint.yml:43` et `fuzz.yml:38`. Les tâches planifiées testent la pointe de `develop`, pas `main` (`pal.yml:26`, `luna-bench.yml:27`, `compiler-hunt`, `fuzz`).
8. **Les mêmes images passent depuis une RAM pseudo-aléatoire.** `luna_runner.py --compare --power-on random=1` est une porte de `make tests` (`Makefile:247`). Résultat de ce jour : `COMPARE: 85/85 ok` deux fois, une passe à zéro et une passe aléatoire.

## Points faibles

**09-1 🔴 — Aucune ROM n'a encore tourné sur une console.** C'est un « must-have » de la porte 1.0, et la vérification repose entièrement sur un seul émulateur. `ROADMAP.md:51` dit « first console session pending » ; `docs/HARDWARE_VERIFICATION.md:170` dit « No session recorded yet. » Tous les oracles de la suite (fbhash, WRAM, audio, DMA, budget NMI, manifestes) interrogent luna seul : Mesen2 est retiré, et aucun second émulateur ne sert de contre-témoin. Or les émulateurs divergent déjà sur ce corpus : `partners/luna/2026-10-03_to_luna_mode6-mesen2-et-v1.30.4_reply.md` (Mode 6, luna juste et Mesen2 différent). Conséquence : un défaut que luna reproduit fidèlement, ou qu'il pardonne, sort en 1.0 sans que rien ne le voie. `make hardware-preflight` existe, mais il rejoue luna. Effort L (propriétaire : une session FXPak, rangées 1-7).

**09-2 🟠 — Windows et macOS ne passent que la vivacité.** Ni les images, ni les manifestes, ni l'identité des ROM avec Linux n'y sont vérifiés. `opensnes_build.yml:168-187` lance seulement `luna_runner.py --coverage` sur ces deux jambes : NMI vivant et CPU non bloqué. Les références `fbhash` sont stables entre architectures (le commentaire de `opensnes_build.yml:430` le dit, et x86_64 passe sur des références capturées en aarch64). Pourtant aucune jambe Windows ou macOS ne lance `--compare` ou `make test-manifests`, et aucune ne compare le sha256 de ses ROM à celui de Linux. Conséquence : une génération de code qui diverge sur l'hôte Windows (MSYS2, clang différent, `long` de 32 bits) et produit une ROM vivante mais fausse sortirait dans le zip Windows. C'est le public visé par `two_audiences.md`. Effort S.

**09-3 🟠 — Les références dorées capturent le comportement, pas la spécification** (T1/PF1 du 03/10, toujours ouvert). Preuve de ce mois : le défaut « un sprite dessiné une ligne trop haut » (`oamSet` stockait y−1 depuis le 2026-04-27) n'a été vu par aucune porte. Il a été signalé de l'extérieur (issue #165), et sa correction `caa73986` a re-capturé 33 images. Ces images protégeaient donc le bug depuis six mois. Le contrôle négatif n°1 montre aussi l'angle mort du fbhash : la ROM qui écrit 6272 octets en affichage actif donne `PASS sprites_simple_sprite / COMPARE: 1/1 ok`. Rien ne refuse une re-capture de masse : `grep -n 'reason' testing/luna_runner.py` ne trouve rien, et `75d0d942` a re-capturé 115 PNG en un commit (un déplacement de dossier, mais rien ne l'aurait arrêté). Effort M.

**09-4 🟠 — Deux oracles de hachage sont surtout une taxe : ils mesurent la phase, pas le défaut.**
- *WRAM* : 69 re-captures de `wram.json` sur 305 commits depuis le 2026-10-05, soit 23 %. Le taux est inchangé depuis le 03/10 (22 %), malgré l'exclusion de la pile. 38 de ces 69 commits sont des `perf`. Le journal des défauts silencieux lui attribue **0 trouvaille sur 80** : `awk -F'|' … silent_defects_log.md | grep -i wram` ne renvoie que des re-captures. Le contrôle négatif n°1 le confirme : il a bien échoué, mais avec le motif « ROM bytes differ from baseline capture », celui qu'il donne pour n'importe quel changement de code.
- *Audio* : 33 re-captures de `audio.json` sur 305 commits. Contrôle négatif n°4 : j'ai ajouté `volatile u16 k; for (k = 0; k < 40; k++)` en tête de `audio/echo/main.c`, un délai inaudible. Résultat : `DIFF audio/echo sha256 f0592672… -> 9f083783…`, exit 1. Sur les mêmes deux ROM, `luna diff --audio --align-onset … --until-frame 300` dit `max delta 0.15% … MATCH`. Le hachage échoue sur un son identique ; la comparaison de luna, faite pour cela, ne sert qu'à commenter la re-capture.
- Conséquence : `fb672b2b`, un `perf`, a re-capturé à lui seul 11 images, 6 manifestes, 58 flux WRAM et 6 hachages audio. Le réflexe « re-capturer et justifier » devient la norme, et c'est exactement ce qui laisse passer le cas 09-3. Effort M.

**09-5 🟠 — Une re-capture audio sans comparaison citée est toujours acceptée** (D PF9 du 03/10). `testing.md` exige de citer `luna diff --audio` à chaque re-capture, mais `audio_regress.py:98-102` écrit la référence sans condition sous `--update`. Effort S.

**09-6 🟡 — Le code GSU n'est pas mesuré, alors que luna sait le faire** (T11 du 03/10, partiel). `luna profile --help` documente `--gsu-pc-set` (« Lets a coverage tool count `.sfx` »), mais `grep -n gsu testing/rom_coverage.py` ne trouve que la fixture 65816 `libtest_gsu`. Le chantier Super FX ajoute du `.sfx` que personne ne compte. Côté DSP-1, 13 fonctions ne s'exécutent qu'avec le firmware (`executed_only_with_firmware.txt`) ; la CI n'a pas ce firmware, donc elle ne les exécute jamais (`dsp1_cube`, `dsp1_ground` sont en SKIP). Effort S.

**09-7 🟡 — `vram_dma_blank.py` ne regarde que les 200 premières frames sans entrée** (T8 du 03/10, ouvert). `vram_dma_blank.py:54` lance `luna state --until-frame` sans `--input`. Un DMA hors blank qui n'apparaît qu'en jeu (changement de scène, chargement de niveau) ne se voit que dans les 41 manifestes qui portent `[asserts.dma]` (`grep -l 'asserts.dma' testing/manifests/*.toml | wc -l` → 41 sur 138). Effort S.

**09-8 🟡 — Les fixtures n'appellent que 72 % de l'API** (T2/T3 du 03/10, partiel). Mesure : 249 fonctions publiques sur 344 sont appelées dans une source de `testing/fixtures/libtests*`. Les manques les plus gros sont `hdma.h` (13), `colormath.h` (9), `sprite.h` (8) et `text.h` (7). Aucune taille OBJSEL (`OBJ_SIZE*`) n'apparaît dans une fixture. Le reste n'est « couvert » que dans le sens où un PC y est passé : 148 fonctions ne sont exécutées que par une seule ROM (`ROM_COVERAGE.md`), dont 91 par `libtest` seul. Effort M.

**09-9 🟡 — `make tests` consomme les ROM de l'arbre telles qu'elles sont, et une porte passe en vert sur une sélection vide.** Le premier lancement dans le worktree partagé a échoué sur `nmi-budget: no ROM in examples/chips/superfx_3d`, et le second sur 6 `FAIL … I/O error reading ROM` : un autre expert reconstruisait en même temps. Dans la copie privée, j'ai retiré une ROM qui a une référence : `luna_runner.py --compare --only sprites_` donne `COMPARE: 7/7 ok` (au lieu de 8), et `wram_regress.py --only simple_sprite` donne `0/0 ok`, exit 0. Une référence orpheline n'est jamais signalée. La passe `--coverage` de la même cible rattrape le cas, ce qui limite la gravité. Effort S.

**09-10 🟡 — Les rapports et commentaires committés sont périmés.**
- `ROM_COVERAGE.md` dit « luna v1.34.0 … 325 of 325 », alors que la passe de ce jour donne 344/344.
- 70 entrées sur 85 de `baselines.json` portent encore `luna_version: v1.34.0`.
- `CORPUS_COVERAGE.md` décrit INPUT-DEP comme « device input unmodelled (gap G4) », alors que souris et Scope sont rejoués depuis.
- `fuzz.yml:2-8` parle de « five targets », alors que `tools/fuzz/Makefile:30` en liste 7.
- La passe `--coverage` réécrit `CORPUS_COVERAGE.md`, un fichier suivi par git : `make tests` salit l'arbre. Je l'ai restauré après mon lancement interrompu.

Effort S.

**09-11 🟡 — Des trous restent dans la grille de la CI.**
- Les jobs de `lint.yml` (`sanitizers`, `host-coverage`, `doc-render`…) n'ont pas de `timeout-minutes` : la valeur par défaut est 360 min.
- Les branches `wip/*` n'ont aucune CI : les déclencheurs `push` ne visent que `main`, `develop` et `maint/**`.
- PAL n'est vérifié qu'une fois par semaine (`pal.yml:12`).
- L'A/B `diff_corpus.py` exigé pour la classe A est manuel, et ne voit pas les fixtures (`silent_defects_log.md`, 2026-10-10).
- L'analyseur TOML des réglages des outils (`tools/common/cli.c`) et le lecteur de ROM d'`opensnes-rom` ne sont pas des cibles de fuzz (`FUZZERS := lodepng itloader tiled json stbimage wav rearrange`) → expert outils pour le fond.

Effort S.

## Risques

- **La croissance du corpus contre la limite de 20 min** du job `functional-tests` (`opensnes_build.yml:364`). Aujourd'hui : 815 s CPU en local sur aarch64. Un runner GitHub a 4 vCPU, et le job exécute aussi la construction et trois bancs. Je n'ai pas pu mesurer la marge réelle (réseau interdit). À vérifier sur le dernier run avant la 1.0.
- **Le fuzz hebdomadaire est au bord de son plafond** : 7 cibles × 600 s = 70 min, pour un `timeout-minutes: 90`. Le run du 2026-10-16 a été annulé à 70 min avant la hausse (`fuzz.yml:44-50`). Une 8ᵉ cible sans relever le plafond le refait tomber.
- **Le bump luna reste un événement coûteux.** Chaque bump re-capture des références (v1.34 → v1.38 : 15 entrées réécrites). Un changement de timing dans luna serait indiscernable d'un changement de l'SDK, faute de second émulateur (09-1).
- **La période de stabilisation qui précède la 1.0 mesurera surtout le silence de la suite.** Ce sont les audits et un jeu bâti sur le SDK qui ont trouvé la majorité des 80 défauts silencieux, pas les oracles permanents (09-4). Une quinzaine sans rouge ne prouve donc pas qu'il n'y a pas de défauts.

## Améliorations recommandées

| # | action | sévérité traitée | effort | premier pas concret |
|---|---|---|---|---|
| 1 | Première session console, rangées 1-7 | 🔴 09-1 | L | `make hardware-preflight ROWS=1-7`, puis le protocole de `docs/HARDWARE_VERIFICATION.md` sur FXPak ; consigner dans « Results log » |
| 2 | Vérifier sur Windows et macOS que les ROM produites sont celles de Linux | 🟠 09-2 | S | Sur les jambes Windows et macOS, ajouter `python3 testing/luna_runner.py --compare` et `make test-manifests` (les références sont inter-architectures), ou comparer le sha256 des 85 ROM à un artefact de la jambe Linux |
| 3 | Remplacer le hachage audio par la comparaison de luna | 🟠 09-4, 09-5 | M | Demander à luna (message direct, prototype à l'appui) `diff --audio` contre un profil de référence (RMS par fenêtre, en JSON) au lieu d'une ROM ; en attendant, `audio_regress.py --update` refuse sans `--reason-file` qui contient la sortie de `luna diff --audio` |
| 4 | Sortir l'oracle WRAM du chemin de chaque commit | 🟠 09-4 | M | Le faire tourner dans `diff_corpus.py` (A/B, même ROM avant et après) au lieu d'une référence committée, ou indexer ses captures par arrivée de symbole (`at_symbol`/`hit`, comme `backgrounds_mode4.toml`) pour qu'un boot plus court ne le fasse pas bouger |
| 5 | Des asserts tirés de la spécification sur la sortie | 🟠 09-3 | M | Un manifeste par fonction de position (`oamSet`, `bgSetScroll`, `mode7SetScroll`) qui asserte la ligne/colonne mesurée dans `ppu.oam_full` ou le framebuffer, avec la valeur de l'arbitre en commentaire ; refuser plus de N images re-capturées sans `--reason-file` |
| 6 | Compter le code GSU | 🟡 09-6 | S | Dans `rom_coverage.py:profile_pcs`, ajouter `--gsu-pc-set` pour les ROM `USE_SUPERFX` et les ranger sur les labels du `.sfx.sym` |
| 7 | DMA en blank sous entrée | 🟡 09-7 | S | Dans `vram_dma_blank.py`, rejouer les scripts `input` des manifestes (`rom_coverage.manifest_runs` le fait déjà) |
| 8 | Fixtures : un vecteur par valeur d'état | 🟡 09-8 | M | Les 6 modes `OBJ_SIZE*` dans `libtests`, puis les 13 fonctions de `hdma.h` absentes des fixtures |
| 9 | Pas de vert sur une sélection vide, pas de référence orpheline | 🟡 09-9 | S | `luna_runner.py`, `wram_regress.py` : exit 1 si 0 cible, et `MISSING` pour une clé de référence sans ROM |
| 10 | Rapports et commentaires à jour, et aucun fichier suivi réécrit par `make tests` | 🟡 09-10 | S | `rom_coverage.py --update` sous v1.38.0, `CORPUS_COVERAGE.md` écrit seulement sous `--update`, en-tête de `fuzz.yml` |
| 11 | `timeout-minutes` sur les jobs de `lint.yml` ; cibles de fuzz pour `cli.c` (TOML) et la lecture de ROM | 🟡 09-11 | S | `timeout-minutes: 45` sur `sanitizers`, `fuzz_cli_toml.c` sur le modèle de `fuzz_json.c` |

## Verdict

Comme filet de non-régression, l'appareil de test est au niveau d'une 1.0 : 4 min 31 s en local pour une suite déterministe dont les portes échouent bien quand on leur injecte un défaut, la couverture de l'API à 344/344 en cliquet, et une recherche différentielle du compilateur qui a trouvé plus de défauts que tout le reste. Comme preuve que le SDK fait ce qu'il promet, il ne l'est pas encore. Rien n'a tourné sur une console, Windows et macOS ne vérifient que la vivacité, et les oracles WRAM et audio sont devenus des détecteurs de phase : re-capturés dans un commit sur quatre, ils n'ont jamais rien trouvé seuls, alors que les images de référence ont protégé un bug de positionnement des sprites pendant six mois.

## Suivi de l'audit du 03/10

| ID (D) | État au 17/10 | Preuve d'aujourd'hui |
|---|---|---|
| T1 / PF1 — références dorées = comportement | **ouvert** | Ni `--reason-file` ni refus de masse (`grep reason testing/luna_runner.py` vide) ; `caa73986` (33 images protégeaient le bug y−1) ; voir 09-3 |
| T2/T3 / PF3 — API absente des fixtures | **partiel** | 249/344 fonctions appelées dans les fixtures (72 %, contre 66 %) ; toujours aucune taille `OBJ_SIZE*` ; 09-8 |
| T4 / PF4 — une seule phase | **corrigé** | `Phase sweep: 48/48 ok (3 manifests x 16 press phases)` dans `make tests` |
| T5 / PF5 — irq + snesmod | **corrigé** | Combinaison dans `libtests_fx` (suivi du 10-05), verte ce jour |
| T6 / PF8 — 14 exemples animés à un seul point | **corrigé** | Les 14 ont `frames = [200, 400]` dans `testing/manifest.toml` (vérifié par `tomllib`) |
| T7 / T10 — asserts delta seuls | **corrigé** | 49 manifestes assertent un `fbhash`, valeurs mesurées en plus des deltas (suivi du 10-05) |
| T8 — DMA et budget NMI sans entrée | **partiel** | `nmi_budget.py` rejoue maintenant des manifestes (`EXTRA`, 12 lignes) ; `vram_dma_blank.py:54` reste sans entrée ; 09-7 |
| T9 / PF11 — delta contre la RAM de mise sous tension | **corrigé** | Checkpoint de référence vide dans `backgrounds_mode4.toml:25` et `backgrounds_mode6.toml:18` ; passe `--power-on random=1` dans `make tests` (`Makefile:242,247`) |
| T11 — DSP-1 et GSU sans couverture CI | **ouvert** | 13 fonctions DSP-1 exemptées en CI ; `--gsu-pc-set` disponible dans luna, absent de `rom_coverage.py` ; 09-6 |
| PF2 (reste) — `cancel-in-progress` | **corrigé** | `opensnes_build.yml:32`, `lint.yml:43`, `fuzz.yml:38` : faux sur `develop` |
| PF6 / rec 13 — matériel réel | **ouvert** (propriétaire) | `docs/HARDWARE_VERIFICATION.md:170` « No session recorded yet » ; 09-1 |
| PF7 — oracle WRAM, coût | **ouvert** | 69 re-captures sur 305 commits depuis le 10-05, 0 trouvaille dans le journal ; 09-4 |
| PF9 — audio re-capturé sans citation | **ouvert** | `audio_regress.py:98-102` écrit sans condition ; 09-5 |
| PF10 / rec 12 — `luna_version` périmées, textes | **partiel** | Les textes sont corrigés, mais 70/85 entrées de `baselines.json` sont à v1.34.0 pour un pin à v1.38.0, et `ROM_COVERAGE.md` est à v1.34.0 ; 09-10 |
| PF12 — fuzz perdu par la concurrence | **corrigé** | `fuzz.yml:38` ; plafond relevé à 90 min pour 7 cibles (`fuzz.yml:50`) |
| rec 7 — DSP-1 en CI équivalente au local | **corrigé** | `rom_coverage.py:379-392` échoue en local sur une fonction exécutée seulement par le firmware et non listée |
