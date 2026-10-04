# Audit D — tests et intégration continue (2026-10-03)

Dépôt `/home/kobenairb/workspace/opensnes`, `develop` @ `73fbdddb` au début de l'audit (`756da353` à la fin : `12a2dcc7` a réparé pendant l'audit le rouge CI décrit au point faible 2), luna épinglé v1.31.0, lecture seule. Mission ajoutée : la chasse aux défauts silencieux avant le gel 1.0 (`.claude/notes/status/silent_defects_log.md`).

## Pourquoi les six défauts ont passé

### Les six, un par un

| # | Défaut (commit) | Test qui aurait dû le voir | Pourquoi il ne l'a pas vu | Classe de trou |
|---|---|---|---|---|
| 1 | KOF=$FF puis $00 à 60 cycles SPC (`8d83859f`) | `audio_snesmod_music_stop.toml` / `_pause.toml` : `V0..V7_ENVX = 0` après un appui | **Il l'a vu, par hasard.** Le manifeste appuie à une seule frame (`input = "60:0x8000,64:0"`). Le corps de `8d83859f` : le bug touche 8 frames d'appui sur 161 sous v1.24.0, et c'est le bump v1.27.0 qui a déplacé la frame 60 sur une frame malchanceuse. Le journal attribue la découverte à « reading the driver against anomie-sdsp », alors que le commit dit « Found by the luna v1.27.0 pin ». | Une course échantillonnée à une seule phase. |
| 2 | `gsuDmaFullFrame()` écrit un tiers du framebuffer pendant l'affichage (`556404fb`, `7a80ac1f`) | Le `[asserts.dma] unsafe_writes = 0` de luna | L'assert existait, mais seulement dans 17 manifestes choisis à la main (`git grep -l asserts.dma 556404fb~1`), dont aucun ne porte sur le Super FX. De son côté, le fbhash de `superfx_3d` avait capturé l'image partiellement périmée comme référence : un émulateur au cycle près reproduit le bug fidèlement, donc la baseline contenait le bug. Le défaut a été trouvé le jour où l'assert a été étendu à tout le corpus (`vram_dma_blank.py`, même commit). | Un oracle existant mais pas généralisé, et une image de référence qui fige le bug. |
| 3 | VOFS de BG3 écrit `y - 1` en Modes 2/4/6 (`9c34b8cd`) | Le fbhash de `backgrounds/mode2` à [200, 400] et `dma_mode2_opt_table.toml` | `990b32d3` (12/09) a re-capturé en un seul commit 67 images et 85 flux WRAM, avec une preuve A/B vérifiée sur `mode0` et `print_string` seulement. `backgrounds_mode2.png` est passé de 5897 à 1451 octets (`@400` : de 6047 à 1451 octets) : les deux points sont devenus la même image plate, sans que rien ne le refuse. Le manifeste assertait les octets de la table en VRAM (l'entrée du PPU) et `wave_phase`, ni BG3VOFS ni la sortie. | Une re-capture en masse, et des asserts sur l'entrée au lieu de la sortie. |
| 4 | `snesmodProcess` : latch H/V, OPVCT lu une fois par tour, file de 256 qui déborde (`0ff69aad`) | `rom_coverage.py` le compte « exécuté » ; aucun test n'assertait son attente, `$213F` ou la file | Aucun exemple ne met 86 commandes en file. L'attente n'était mesurée nulle part. Aucun exemple ne lie SNESMOD et le Super Scope (le corps de `0ff69aad` le dit : « no example links both »). | Exécuté mais pas asserté, et un cas limite jamais atteint. |
| 5 | `snesmodInit` écrit `$81` dans NMITIMEN (`74b9aeab`) | Aucun | Aucun exemple n'arme un IRQ H/V avant `snesmodInit` : `irqSet*` n'est appelé que par `chips/superfx_3d` et `color/hicolor_1792`, qui ne lient pas SNESMOD. Un invariant registre = shadow n'aurait rien vu non plus sans cette combinaison. | Une interaction entre modules sur un registre partagé, jamais combinée. |
| 6 | `oamDrawMetaFlip` suppose des pièces de 16/8 px quel que soit OBJSEL (`79c5ec73`) | Le vecteur « lot C » de `devtools/libtests/test_libtest.py` (`PPU_CASES`) | Le seul vecteur utilise des pièces de 8 px dans une boîte de 16 (`oam_full.40 = 108`). Aucune fixture ne fixe une taille OBJSEL (0 occurrence de `OBJ_SIZE` dans `devtools/libtests*/main.c`). Aucun exemple ne dessine un métasprite retourné à pièces de 32 px. | Une dimension implicite du contrat (l'état global OBJSEL) jamais échantillonnée. |

Bilan : **aucun des six n'a été vu par la suite telle qu'elle existait**. Deux l'ont été par un test, mais l'un par le hasard d'un décalage de timing (n° 1), l'autre par un test écrit le jour même (n° 2). Les quatre autres ont été trouvés en lisant du code (n° 4 et 5), en écrivant un exemple (n° 3) ou en écrivant le remplaçant (n° 6).

### Le journal est incomplet, et cela touche la mesure

Le critère 7 se mesure sur ce journal, mais celui-ci omet au moins quatre défauts silencieux de `lib/` ou du runtime corrigés dans la même période (`git log --since=2026-09-26 | grep fix(lib|runtime…)`) :
- `2981fe0b` (26/09) : le moteur de sprites dynamiques stockait `y` au lieu de `y - 1`. Trouvé par le nouveau `input_dynamic_metasprite.toml`. Le manifeste existant `oam_dynamic_sprite.toml` **avait figé la mauvaise valeur** ($64 pour y = 100).
- `20638bc3` (27/09) : l'IRQ du GSU sur STOP faisait boucler le CPU à l'infini dans son IRQ. Trouvé par la fixture GSU.
- `73a5644e` (27/09) : `gsuSetupHdmaBlanking` écrivait `$420C` directement et éteignait les autres canaux HDMA. Trouvé par la passe de documentation.
- `93ff5e2d` (02/10) : `mode7SetScale(0x0100)` donnait un zoom ×2. Le manifeste `rotate_scale` **avait figé les valeurs d'échelle fausses** (« helper-scale values halve »).

Trois des dix défauts de la période avaient donc une référence dorée qui **protégeait le bug** : le PNG de mode2, `$64` dans `oam_dynamic_sprite`, l'échelle de `rotate_scale`. → aspect gouvernance pour la tenue du journal.

### Les trous du même type, classés par sévérité du défaut qu'ils laisseraient passer

| Trou | Mesure | Défaut qu'il laisserait passer | Sév. |
|---|---|---|---|
| **T1. Les références dorées capturent le comportement, pas la spécification** | 89 exemples sous fbhash. Les manifestes `values` et `blocks` sont en majorité relevés sur l'exécution (cas documentés : mode2, `oam_dynamic_sprite`, `rotate_scale`). `diff_corpus` explique un DIFF mais ne l'empêche pas d'entrer : `990b32d3` a accepté 67 DIFF en un commit. | Un rendu ou un état faux dès la capture, ou accepté dans une re-capture de masse | 🟠 |
| **T2. Exécuté ≠ asserté** | **119 fonctions publiques sur 351 (34 %) n'apparaissent dans aucune fixture** (`devtools/libtests*/*.c`, `compiler-tests/runtime/*`). Leur justesse ne repose que sur les oracles d'exemples. Exemples vérifiés : `scopeCalibrate` (`superscope.toml` n'asserte que `scope_shothraw/vraw`, jamais `scopeGetX/Y` calibrés) ; `hdmaIrisWipe`, `hdmaWaterRipple`, `hdma*Stop` (`dma_hdma_helpers_effects.toml` asserte le shadow `hdma_enabled_state`, aucune image après appui) ; `mode7SetScroll`, `oamHide`, `padReleased`, `snesmodLoadEffect` (aucun vecteur). | Un mauvais résultat sur un chemin qui ne change pas l'image à la frame 200 (classe des défauts n° 4 et 6) | 🟠 |
| **T3. Dimensions implicites non échantillonnées** | Les fixtures n'appellent qu'une configuration par fonction : aucune taille OBJSEL, un seul mode BG par vecteur. 24 occurrences de `OBJ_SIZE8_L16` dans les exemples contre 1 de `OBJ_SIZE32_L64`. | Une fonction qui ignore un état global (classe du n° 6) | 🟠 |
| **T4. Courses vérifiées à une seule phase** | Chaque appui scripté tombe sur une seule frame. Prototype mesuré aujourd'hui : stop et pause de `snesmod_music` balayés sur les frames 40 à 200 (322 manifestes générés) donnent **322/322 PASS en 72 s** sur 6 cœurs. Ce balayage n'est pas dans la suite. | Une course qui ne frappe qu'une fraction des phases (classe du n° 1) | 🟠 |
| **T5. Combinaisons de modules sur un registre partagé** | NMITIMEN (console, irq, snesmod, superfx), `$213F`/OPVCT (input scope, snesmod, superfx), HDMAEN (hdma, window, superfx). Aucun exemple ne combine irq + snesmod ni scope + snesmod. Sonde ad hoc lancée aujourd'hui : `cpu_regs.nmitimen == nmitimen_shadow` et `stat78 & 0x40 == 0` à la frame 300, **89/89 OK** (`luna state --peek`). Sans la combinaison, l'invariant ne voit rien. | Un module qui écrase l'état d'un autre (classe du n° 5) | 🟠 |
| **T6. Exemples auto-animés capturés en un seul point** | Mesuré (fbhash aux frames 200, 400 et 1200) : **14 exemples animés n'ont qu'un point de capture** : `backgrounds/mode0`, `basics/fix32_orbit`, `basics/timer`, `chips/superfx_3d`, `chips/sa1_starfield`, `chips/superfx_game_skeleton`, `color/pseudo_hires`, `games/shmup_1942`, `games/tetris`, `mode7/extbg`, `scrolling/mixed_scroll`, `sprites/aseprite_pipeline`, `sprites/dynamic_sprite`, `text/scroll_message`. Une animation figée (classe du n° 3) passe le fbhash. Les manifestes compensent pour certains (`fix32_orbit` : valeurs à 5 checkpoints ; traces GSU et SA-1). | Un effet qui s'arrête après le boot | 🟡 |
| **T7. Images capturées avant le chemin intéressant** | 65 exemples ont la même image aux frames 200, 400 et 1200 (aucune animation sans entrée). Seuls 2 manifestes assertent un fbhash après une entrée (`color_shadow_tint`, `window_triangle_modes`). | Un effet déclenché au bouton qui rend faux, quand le shadow WRAM est juste | 🟡 |
| **T8. Oracles de corpus sur le chemin au repos seulement** | `vram_dma_blank.py` : `luna state --until-frame 200`, sans script d'entrée. `nmi_budget.py` : 9 lignes sur 7 ROMs. | Une DMA VRAM hors blank ou une NMI trop longue sur un chemin piloté (scroll, changement de carte) | 🟡 |
| **T9. Manifestes jamais rejoués depuis une RAM aléatoire** | 0 manifeste ne fixe `power_on`. Rejoués aujourd'hui avec `power_on = "random"` et les graines 2, 42, 31337, puis `ones` : **130/132** à chaque fois. Les 2 échecs sont un artefact du harnais (`backgrounds_mode4.toml:24`, `backgrounds_mode6.toml` : un `delta` au premier checkpoint compare la valeur à la RAM de mise sous tension, ex. `0xA12D -> 0x9A`). Aucun défaut de ROM. | Une lecture de mémoire non initialisée sur un chemin piloté | 🟡 |
| **T10. Manifestes « delta » seuls** | 9 manifestes n'assertent qu'un `increased` ou `changed` sur une variable : `map_scroll`, `movement_{aim_target,collision_demo,likemario,perspective,tiled}`, `sprites_random`, `state_dynamic_map`, `state_scene_stack`. En revanche, **0 manifeste n'asserte que `r_done`** (les quatre manifestes de chaîne d'alimentation assertent aussi des blocs SRAM). | Une mauvaise valeur qui va dans le bon sens | 🟡 |
| **T11. DSP-1 et GSU sans couverture CI** | 15 fonctions DSP-1 ne s'exécutent qu'avec le firmware (`executed_only_with_firmware.txt`), absent en CI. Le code GSU n'est pas mesuré (`--gsu-pc-set` absent de `rom_coverage.py` ; action 30 du suivi : « Left »). | Une régression DSP-1 ou GSU visible seulement sur la machine du propriétaire | 🟡 |

### Mise sous tension : trois graines autres que 1

`luna_runner.py --coverage --power-on X` pour `random=1`, `random=2`, `random=42`, `random=31337` et `ones` : **87 OK / 2 INPUT-DEP / 0 DEAD / 0 FAIL sur 89** dans chaque cas, en 11 à 13 s. J'ai poussé plus loin que demandé :
- `--compare` (fbhash) : **89/89** sous zéro, `random=1/2/42/31337` et `ones` ;
- les 132 manifestes : 130/132, avec les 2 artefacts de T9.

**Aucun changement** par rapport à la graine 1. La liveness est l'oracle le plus faible des trois. Le résultat qui compte est que les images et les manifestes tiennent sous cinq états de RAM.

## Périmètre couvert

- Lu : `Makefile` (`tests`, `test-pal`, `test-manifests`), les 8 workflows, `tools/luna-test/` (runner, `manifest.toml`, les 132 manifestes analysés par script avec `tomllib`, `ROM_COVERAGE.md`, baselines), `devtools/libtests*/test_*.py`, le journal des défauts silencieux, `2026-10-03_suivi_actions.md`, `OPEN_luna.md`, l'annexe D du 26/09, les corps des 10 commits `fix(lib|runtime|compiler)` depuis le 26/09 et de `990b32d3`.
- CI via `gh` (`GH_TOKEN=$GH_PAT_TOKEN`) : 100 runs build de `develop`, les runs fuzz, pal, luna-bench et release, et les logs complets de `37127175588` (vert) et `37146843602` (rouge).
- Lancé localement sur les `.sfc` existants : liveness ×5 états de RAM, `--compare` ×6, `wram_regress.py`, `audio_regress.py`, `nmi_budget.py` (gate et `--report`), `vram_dma_blank.py`, les 132 manifestes ×4 états de RAM, le balayage de 322 manifestes stop/pause, une sonde NMITIMEN/STAT78 sur 89 ROMs, les fbhash aux frames 200, 400 et 1200 sur 89 ROMs.
- Pas lancés : `make tests` complet, `make test-pal` (il reconstruit `games/tetris` dans `examples/`), le fuzz.
- Hors périmètre, renvoyé : la justesse des fonctions de la lib (→ aspect B), la tenue du journal du critère 7 (→ aspect H).

## Points forts

1. **Les ratchets et les oracles généralisés trouvent de vrais défauts.** `vram_dma_blank.py` a trouvé `gsuDmaFullFrame` dès sa première exécution (`556404fb`). Le nouveau manifeste `input_dynamic_metasprite` a trouvé le `y - 1` des sprites dynamiques (`2981fe0b`). Le ratchet de couverture a bloqué `dsp1SetCamera` (run `37146843602` : « NEW never-executed public function: dsp1SetCamera »).
2. **La suite est robuste à l'état de la RAM.** Mesuré ci-dessus : fbhash 89/89 et manifestes 130/132 (artefacts seulement) sous cinq états de RAM.
3. **`make tests` reste rapide malgré 7 oracles.** Run `37127175588` (cef95406), étape « Full test suite » : **x86_64 11 min 06**, **arm64 6 min 08**. Détail x86 :

   | Étape | Durée |
   |---|---|
   | Compiler checks | 2 s |
   | Liveness | 53 s |
   | Liveness `random=1` | 53 s |
   | fbhash | 55 s |
   | fbhash `random=1` | 56 s |
   | `rom_coverage.py` | 2 min 26 |
   | Audio | 22 s |
   | Budget NMI | 8 s |
   | DMA en blank | 54 s |
   | Manifestes | 1 min 18 (132 passed, 3 skipped) |
   | WRAM | 21 s |
   | Fixtures, runtime et link-modules | 1 min 58 |

   En local, chaque oracle prend entre 4 et 23 s (mesures ci-dessus).
4. **Windows et macOS exécutent enfin des ROMs.** Étapes « Execute the corpus on luna (macOS) » (33 s) et « (Windows) » (45 s) du même run. Le point faible 5 du 26/09 est corrigé (`7dbdaea7`, `b8e2be4f`).
5. **Le retard d'épinglage luna est résorbé.** Neuf bumps depuis le 26/09, de `f7367cdf` (v1.27.0) à `75599363` (v1.31.0). v1.32.0, publiée à 19:13 le 03/10, livre `luna diff --audio`. Ce que luna a livré est utilisé :
   - `--stack-floor` dans une invocation séparée de `--budget` (`rom_coverage.py:175`, ce qui évite le piège du code de sortie partagé) ;
   - périphériques rejoués en profil (`rom_coverage.py:99-101`) ;
   - `[asserts.gsu]` dans 3 manifestes ;
   - chaînes de batterie a/b, d/e, f/g en parallèle (v1.30.1).
6. **Les points faibles du 26/09 sont en majorité fermés, et vérifiés.**
   - `pal.yml` est vert (run `36409175093`, 28/09).
   - Les planifiés tournent sur `develop` (`fuzz.yml:54`, `pal.yml:26`, `luna-bench.yml:27`).
   - `nmi_budget.py --report` marche (lancé : 9/9).
   - La liveness détecte une NMI morte (`last_nmi_frame`, `luna_runner.py:202-206`).
   - `test_harness.py` (24 tests) tourne dans `lint.yml:116-121`.
   - L'audio couvre 11 exemples.
7. **Les manifestes sont denses, et aucun n'est creux.** 132 manifestes : 53 avec `values` de checkpoint, 41 avec `blocks`, 34 avec `delta`, 23 avec `values` globaux, 20 avec `dma`, 15 avec `dsp`, 10 avec `ppu` de checkpoint, 9 avec `oam`. 0 n'asserte que `r_done`. Le schéma est strict : une clé inconnue est refusée (`unknown field 'bogus_key'`).
8. **Chaque trou découvert a été fermé avec son contrôle négatif.** Une capture aux points identiques est refusée (`9c34b8cd`, `luna_runner.py`), `bgs.2.v_scroll = 0` est asserté et la ROM d'avant le correctif échoue. La fixture `libtests_snesmod` montre l'ancien code et le nouveau sur la même ROM (`r_latch` $40 → 0, `r_irq` 0 → 10).
9. **La stratégie de fuzz est maintenant la bonne.** Il tourne sur chaque push qui touche `tools/**` et chaque semaine sur `develop` (`fuzz.yml:25-31,54`). Il a trouvé une vraie bombe d'allocation (stbimage, run `36273674419`) et est vert 4 fois depuis.

## Points faibles

1. 🟠 **La suite ne joue pas le rôle de sentinelle pour la classe de défauts que le gel cherche.**
   - Preuve : le tableau d'ouverture (0 des 6 défauts vus par la suite existante), et 3 défauts sur 10 dans la période dont la référence dorée protégeait le bug.
   - Conséquence : les quatorze jours « sans défaut » du critère 7 mesureraient l'absence de lecteurs, pas l'absence de défauts. La suite est forte contre la régression de ce qui marche déjà, faible contre ce qui n'a jamais marché.
2. 🟠 **`develop` est restée rouge 2 h 50 sur 5 pushes, et 47 % des commits n'ont pas eu de verdict CI.**
   - Le rouge : run `37143108501` (`1d922c39`, 18:08) jusqu'au correctif `12a2dcc7` (20:58), le corps de ce dernier le confirme. Cause : la machine locale a `dsp1b.rom` et la CI non, donc `make tests` local n'est **pas** la porte CI pour le DSP-1. Le risque avait été nommé le 26/09.
   - Les verdicts : sur les 78 commits de `develop` depuis le 27/09, 32 verts, 5 rouges, **37 annulés** par le push suivant (`cancel-in-progress: true`, `opensnes_build.yml:27-29`), 3 sans run, 1 en cours.
   - Conséquence : une bissection tombe une fois sur deux sur un commit jamais vérifié, et le premier commit rouge « was not obvious » (`12a2dcc7`).
3. 🟠 **Exécuté n'est pas asserté (T2, T3).**
   - Preuve : 119 fonctions sur 351 sans vecteur de fixture. `ROM_COVERAGE.md` annonce « 351 of 351 public functions executed », un chiffre qui se lit comme une garantie.
   - Conséquence : les défauts n° 4 et 6 venaient exactement de là.
4. 🟠 **Les courses ne sont testées qu'à une phase (T4).**
   - Preuve : `audio_snesmod_music_stop.toml` appuie à la frame 60 seulement ; le balayage de 161 frames prouvé dans `8d83859f` n'est pas resté dans la suite.
   - Conséquence : une régression du même genre a environ 95 % de chances de passer.
5. 🟠 **Aucun test de combinaisons de modules (T5).**
   - Preuve : irq + snesmod et scope + snesmod n'existent dans aucun exemple ni aucune fixture avant `74b9aeab`.
6. 🟠 **Le matériel réel n'a toujours jamais tourné.**
   - Preuve : `2026-10-03_suivi_actions.md` ligne 48, « First console session … not started ». Le nouveau `make hardware-preflight` (`5cdf1aa0`) rejoue la liveness sur émulateur : c'est utile, mais ce n'est pas une console.
7. 🟡 **L'oracle WRAM reste une taxe pour peu de signal.**
   - Preuve : depuis l'exclusion de la pile (`b106ad43`), 24 re-captures sur 110 commits (22 %, contre 37 % avant). Les corps donnent la cause, « RAM that moved holds ROM addresses » (`79c5ec73` : `tcc__r9`, `dynamic_flush_hook`). Aucun des 10 correctifs de la période ne cite l'oracle WRAM comme découvreur.
8. 🟡 **Des images fixées trop tôt ou à un seul point (T6, T7)**, avec les chiffres ci-dessus : 14 exemples animés à un point, 65 statiques.
9. 🟡 **L'audio est re-capturé à chaque changement de phase.**
   - Preuve : 6 re-captures de `audio.json` depuis le 26/09, dont trois pour « trois cycles » de décalage (`74b9aeab`). `luna diff --audio` est publié en v1.32.0 mais pas épinglé (`OPEN_luna.md`, ligne du 03/10).
10. 🟡 **Des textes du harnais mentent encore**, trois points déjà relevés le 26/09 et non corrigés :
    - `wram_regress.py:258` imprime « 2 skipped (cross-arch) » en CI (log `37127175588`, 13:53:49) alors que les deux exemples sautés le sont faute de firmware ;
    - `manifest.toml:19` renvoie à `probes/mouse.py` et `probes/superscope.py`, mais `probes/` ne contient que `lib.py` ;
    - 72 entrées de `baselines.json` portent `luna_version: v1.21.0`.

    `ROM_COVERAGE.md` est resté sur la capture v1.27.0 / 312 fonctions du 26/09 au 03/10, jusqu'à `12a2dcc7`.
11. 🟡 **Deux manifestes sont fragiles sous une RAM aléatoire (T9)** : `backgrounds_mode4.toml:24`, `backgrounds_mode6.toml`. Effort S.
12. 🟡 **Le fuzz perd des runs à cause de la concurrence.**
    - Preuve : `fuzz.yml:33-35` (`cancel-in-progress: true`) ; 6 runs de 50 min annulés le 26/09 sur 13 depuis le 25/09.
    - Conséquence : une rafale de pushes dans `tools/` ne fuzz que le dernier.

## Risques

- **La campagne de chasse va produire des correctifs, donc des re-captures.** Chaque correctif de la semaine a re-capturé WRAM et souvent l'audio. Une re-capture de masse peut encore avaler une régression comme `990b32d3` l'a fait. Le refus « points identiques » ne couvre que les 10 exemples à plusieurs points.
- **La dérive « local ≠ CI » va revenir** à chaque nouvelle fonction DSP-1 : l'exemption est une liste tenue à la main (`executed_only_with_firmware.txt`, 15 noms).
- **Le bump v1.32.0 arrive pendant la chasse.** Il faut l'utiliser tout de suite pour l'audio (règle « quote `luna diff --audio` »), sinon la prochaine re-capture audio se fera encore à l'œil.
- **`make tests` en CI est passé de 16 min (25/09) à 11 min grâce au parallélisme, mais il croît avec le corpus.** Un balayage de phase complet ajouterait environ 72 s × N contrats sur 6 cœurs : il faut garder un sous-ensemble (16 phases ≈ 4 s par contrat).
- **Le journal du critère 7 sous-compte.** S'il reste tenu à la main, le compteur de quinzaine démarrera sur une base fausse.

## Améliorations recommandées

| # | Action | Sévérité traitée | Effort | Premier pas concret |
|---|---|---|---|---|
| 1 | Asserter la **sortie** et non l'entrée, à partir d'une valeur dérivée de la spec | 🟠 (T1, faible 1) | M | Pour chaque manifeste qui asserte des octets de table, de shadow ou de file : ajouter l'assert du registre PPU/DSP correspondant (`[asserts.ppu]`, `bgs.N.*`, `m7*`, `hdma` canal). Écrire dans le commentaire d'où vient la valeur (spec, calcul), jamais « relevé sur l'exécution ». Commencer par `dma_hdma_helpers_effects.toml` et `superscope.toml` (`scopeGetX/Y` calibrés). |
| 2 | Une re-capture de masse doit justifier chaque image | 🟠 (T1, n° 3) | S | Dans `luna_runner.py --update`, refuser plus de N images (ex. 5) sans `--reason-file`, un fichier où chaque label a une ligne de justification. Imprimer pour chaque image le rapport de taille PNG ancien/nouveau (1451/5897 aurait sauté aux yeux). |
| 3 | Balayage de phase pour les contrats à course | 🟠 (T4, n° 1) | S | Générer dans `test-manifests` 16 phases pour stop, pause et fade (prototype : 322/322 en 72 s ; 32 manifestes ≈ 7 s). Étendre ensuite à `gsuPresent` (swap) et à la file DMA près du budget. |
| 4 | Une fixture « combinaisons » | 🟠 (T5, n° 5) | M | `devtools/libtests_combo` : irq V-timer + snesmod + scope + hdma + superfx-less. Asserter à chaque frame `cpu_regs.nmitimen == nmitimen_shadow`, `stat78 & 0x40 == 0` hors tir et HDMAEN == shadow (sonde déjà écrite pour cet audit : 89/89 au repos). |
| 5 | Échantillonner les dimensions implicites des fixtures | 🟠 (T2, T3, n° 6) | M | Lister les 119 fonctions sans vecteur. Pour chaque fonction qui lit un état global (OBJSEL, mode BG, bpp, région), un vecteur par valeur de cet état. Commencer par `sprite.h` (les 6 modes OBJSEL) et `text.h` (2 et 4 bpp). |
| 6 | Arrêter d'annuler les runs build de `develop` | 🟠 (faible 2) | S | `opensnes_build.yml:29` : `cancel-in-progress: ${{ github.ref != 'refs/heads/develop' }}`, ou garder l'annulation et ajouter un job nocturne qui teste chaque SHA de la veille sans verdict. Faire pareil pour `fuzz.yml:35`. |
| 7 | Rendre la CI DSP-1 équivalente au local | 🟠 (faible 2) | S | Dans `rom_coverage.py`, échouer **localement** quand une fonction n'est exécutée que par une ROM marquée `firmware` et absente de `executed_only_with_firmware.txt`. Le rouge apparaît alors avant le push. |
| 8 | Épingler v1.32.0 et brancher `luna diff --audio` | 🟡 (faible 9) | S | `luna.version` → `v1.32.0`. Rejouer les trois runs de `2026-10-03_to_luna_diff-audio_reply.md`. Faire refuser `audio_regress.py --update` sans un `luna diff --audio` cité. |
| 9 | Un deuxième point de capture pour les 14 animés | 🟡 (T6) | S | Ajouter `frames = [200, 400]` dans `manifest.toml` pour la liste mesurée. Le refus « points identiques » s'applique alors automatiquement. |
| 10 | Les oracles de corpus sur les chemins pilotés | 🟡 (T8) | S | Dans `vram_dma_blank.py`, reprendre les scripts `input` des manifestes (comme `rom_coverage.manifest_runs`). |
| 11 | Manifestes sous RAM aléatoire en CI | 🟡 (T9, faible 11) | S | Corriger les deux `delta` du premier checkpoint (checkpoint à vide à la frame 60, comme `movement_likemario.toml`). Ajouter à `make tests` une passe `luna test` sur une copie avec `power_on = "random"`, `seed = 1` (script de cet audit : `mkmf.py`, 20 lignes). |
| 12 | Corriger les trois textes menteurs | 🟡 (faible 10) | S | `wram_regress.py:258` : distinguer firmware et cross-arch. `manifest.toml:19` : renvoyer à `mouse.toml` et `superscope.toml`. `baselines.json` : réécrire `luna_version` au prochain `--update` ou supprimer le champ. |
| 13 | Première session console | 🟠 (faible 6) | L | Exécuter les lignes 1 à 7 de `docs/HARDWARE_VERIFICATION.md` sur v0.47.0, après `make hardware-preflight ROWS=1-7`. |

## Verdict

Comme filet contre la régression, l'appareil de test est au niveau d'un SDK 1.0 : 7 oracles en 11 min en CI sur deux architectures, des ROMs exécutées sur les quatre OS, luna à jour à quelques heures près, et des résultats qui tiennent sous cinq états de RAM. Comme détecteur de ce qui n'a jamais marché, il ne l'est pas : aucun des six défauts de la semaine n'a été vu par la suite existante, trois références dorées de la période protégeaient leur bug, et un tiers de l'API publique n'est « couvert » qu'au sens où un PC y est passé. Avant de compter les quatorze jours du critère 7, il faut des asserts tirés de la spec sur la sortie, des balayages de phase et une fixture de combinaisons (actions 1 à 5), sans quoi la quinzaine mesurera le silence de la suite et non l'absence de défauts.

## Suivi 2026-10-05 (session)

- **T5 (irq + snesmod, scope + snesmod)** : la fixture `libtests_fx` arme
  un IRQ V-timer avant `snesmodInit`, compte dix frames de
  `snesmodProcess` (`r_irq_mod` = 10), lit STAT78 juste après l'appel du
  pilote (`r_mod_latch` = 0 : le drapeau de latch que le code Super Scope
  prend pour un tir, cf. anomie-timing `626b31bd887c2581`), et vérifie
  `cpu_regs.nmitimen` = 0xA1 et `cpu_regs.vtime` = 120 dans la vue luna.
  Les deux défauts du 10-03 l'auraient fait échouer. Le périphérique Super
  Scope lui-même n'est pas piloté (luna ne modélise pas l'entrée) : la
  combinaison est couverte par son point de contact, le drapeau de latch.
- **Rec 5 (en-tête de ROM)** : voir `C_build_tools.md`, suivi du 10-05.
- **PF7 (oracle WRAM, coût)** : la re-capture du jour (deux exemples objet)
  a une cause documentée : `mapLoad` copie 4096 octets de définitions de
  métatuiles quel que soit le fichier (126 octets), donc la queue de
  `metatiles` est une copie de la ROM qui suit, et elle bouge avec le code
  (ici un pointeur `getFrameCount` d'une table const). Noté dans `map.h` ;
  l'oracle ne peut pas l'exclure sans connaître la taille du fichier.
- **T4 (une seule phase)** : `phase_sweep.py` rejoue les manifestes stop,
  pause et fade de `snesmod_music` à seize phases d'appui (0..15 frames,
  points d'entrée et frame d'assertion décalés ensemble), dans
  `make test-manifests`. Contrôle négatif : une copie sans appui qui attend
  le silence échoue aux seize phases.
- **T7 / T10 (assertions « delta » seules, images avant l'entrée)** : les
  huit manifestes concernés assertent maintenant la valeur mesurée à chaque
  point (`[checkpoint.values]`, mesurée avec `luna state --until-frame F
  --input … --peek`) en plus de la direction, et l'image de fin de script
  (`[asserts] fbhash`, mesurée avec `--print-fbhash` à la même frame) ; 8/8
  verts. `state_scene_stack` était déjà sur une assertion de valeur.
- **PF10 / rec 12 (`luna_version` à v1.21.0)** : `luna_runner.py --update`
  rejoué sous la v1.32.0 épinglée : 89 hachages identiques, champs
  `luna_version` et `rom_sha256` à jour. Les deux autres textes de la rec 12
  (`wram_regress.py:258`, `manifest.toml:19`) restent à relire.
