# 11 — Exemples et jeux

Expert : exemples et jeux · date : 2026-10-17 · HEAD audité : `develop` @ `73a3fdfe` (worktree `/home/kobenairb/workspace/opensnes-audit`, luna v1.38.0)

## Périmètre couvert

- Lu : `examples/README.md`, `examples/games/README.md`, les 8 README de jeux, `.claude/rules/new_example.md`, `.claude/skills/port-example/SKILL.md`, `ATTRIBUTION.md`, `ROADMAP.md` (porte 1.0), `docs/STABILITY.md` l. 47, `F_examples.md` et `2026-10-04_reste_a_traiter.md` (lettre F).
- Lu en entier ou par sections : 6 `main.c` — `text/print_string`, `sprites/simple_sprite` (basiques), `scrolling/parallax_scroll`, `windows/window` (intermédiaires), `games/breakout`, `games/likemario` (jeux) — plus `tetris/main.c` + `render.c`, `shmup_1942/main.c`, `mapandobjects/mario.c` / `slope_collision/mario.c`.
- Lancé dans le worktree : `python3 testing/luna_runner.py --compare` (85/85 ok, 10,7 s), `make test-manifests` (143/143 + phase sweep 48/48), les 9 manifestes de jeux un par un (`luna test`, tous verts), `make lint-docs` (vert).
- Mesuré : md5 de chaque fichier binaire suivi sous `examples/` contre l'arbre `~/workspace/pvsneslib` ; grep de couverture des fonctions SNES et des modules ; globals non `static`, commentaires Doxygen orphelins, écritures de registres brutes.
- Sonde luna hors arbre (copie de `shmup_1942` dans le scratchpad, `make OPENSNES=<worktree>`) pour instruire le « mystère BG3 » (F n°5). Corpus : une requête `snes_search` sur l'ordre KON / registres de voix (F n°4).

## Points forts

- **Chaque exemple a un manifeste fonctionnel.** Les chemins de ROM cités par `testing/manifests/*.toml`, comparés à `find examples -name main.c` : 85 sur 85. `make test-manifests` : « 143 passed, 0 failed », plus « Phase sweep: 48/48 ok ». `luna_runner.py --compare` : « COMPARE: 85/85 ok ».
- **Le chemin piloté par l'entrée est sous la porte DMA.** Les 19 manifestes `movement_*` / `state_*` portent `[asserts.dma] unsafe_writes = 0` (2 lignes trouvées par grep dans chacun). Au 04/10, seul `state_tetris` l'avait.
- **Breakout boucle tout son cycle de jeu, et c'est testé.** `state_breakout_game_over.toml` : START → 5 balles perdues → GAME OVER (`lives = 0` à la trame 1950) → nouvelle partie (`lives = 4`, `score = 0` à 2030) → on rejoue (`lives = 3` à 2200), et le test passe.
- **Les exemples qui enseignaient des bugs ont été purgés, et une sentinelle les surveille.** `grep -rn -i 'framesize=158\|spill to bank\|assumes bank\|must be in bank\|compiler quirk' examples` ne renvoie rien. L'ancre 14 de `check_doc_drift.py` porte ces motifs. `likemario/README.md:120` dit maintenant que `>>` sur un signé est arithmétique.
- **Le HDMA de tetris passe par la bibliothèque, en VBlank.** `render.c:382-385` : `WaitForVBlank(); hdmaSetup(HDMA_CHANNEL_6, …); hdmaEnable(…)`, et `hdma` figure dans `LIB_MODULES` (`tetris/Makefile:11`).
- **L'onboarding est cohérent.** `make lint-docs` : « OK: no doc drift detected ». Le tableau par catégorie fait bien 85 au total, `games/README.md` liste les 8 jeux, et chaque exemple est cité par `docs/LEARNING_PATH.md` (ancre 5).
- **La couverture matérielle est large** (matrice ci-dessous) : chaque mode BG de 0 à 7 a au moins un exemple ; offset-per-tile en modes 2, 4 et 6 ; hi-res 512 et entrelacé ×3 ; direct color ; HiColor ; Mode 7 EXTBG ; DSP-1 ×2 ; SA-1 ×3 ; Super FX ×4 ; sauvegarde sur les trois mappings (SRAM, BW-RAM, Game Pak RAM) ; souris ; Super Scope ; audio v2 ×3 et APU brut ×4.
- **Les basiques sont propres.** `print_string` tient en 57 lignes avec une séquence d'init correcte (`WaitForVBlank` avant `setScreenOn`). `simple_sprite` explique le calcul du numéro de tuile (`($2100-$2000)/16 = 0x10`, l. 77-80).

### Matrice de couverture (dossiers d'exemples qui utilisent la fonction ; grep sur `.c`, `Makefile`, `.asm`)

| Fonction | N | Exemples |
|---|---|---|
| Mode 0 / 1 / 2 / 3 / 4 / 5 / 6 / 7 | 7 / 36 / 1 / 6 / 1 / 1 / 1 / 7 | mode0…mode6, 5 mode7 + 2 jeux |
| Offset-per-tile | 3 | backgrounds/mode2, mode4, mode6 |
| Hi-res 512 / entrelacé | 3 | mode5_hires, pseudo_hires, extbg |
| Direct color | 1 | color/direct_color |
| HDMA | 12 | 4 hdma/, tetris, dsp1_ground, perspective… |
| Fenêtres | 4 | 3 windows/, hdma_helpers |
| Color math | 4 | transparency, shadow_tint, direct_color, transparent_window |
| Mosaïque | 1 | transitions/mosaic |
| HiROM / FastROM | 1 / 1 | memory/hirom_demo / chips/sa1_starfield |
| SRAM | 3 | save_game, sa1_save, superfx_save |
| SA-1 / Super FX / DSP-1 | 3 / 4 / 2 | chips/*, mode7/dsp1_ground |
| SNESMOD / audio v2 / APU brut | 5 / 3 / 4 | snesmod_*, likemario, tetris, superfx_game_skeleton / echo, sfx_from_wav, soundboard / apu_switch, pitch_mod, play_noise, speech_synth |
| Pad / souris / Scope / **multitap** | ~60 / 1 / 1 / **0** | — |
| IRQ H/V | 3 | superfx_3d, superfx_hello, hicolor_1792 |
| **MSU-1, PAL (`isPAL`/`getRegion`), `profile*`, `easeIn/Out`, `debug`** | **0** | — |

## Points faibles

**11-1 🔴 — Assets Nintendo et PVSnesLib distribués sans attribution, et la porte 1.0 affirme le contraire.** Mesure : md5 des fichiers binaires suivis sous `examples/` contre `~/workspace/pvsneslib`. **55 fichiers identiques dans 19 exemples, 824 Ko.** Parmi eux : `mapandobjects/res/mario.png`, `goomba.png`, `koopatroopa.png`, `likemario/res/mariojump.brr`, `likemario/res/overworld.it`, `tiledMario.*` (×2), et les 7 `.dat` de breakout. `grep -niE 'nintendo|mario|goomba|koopa' ATTRIBUTION.md` ne renvoie rien. `ATTRIBUTION.md` ne crédite que Kenney, krom et les œuvres originales. Or `ROADMAP.md:53` affirme : « 66 asset files in 23 examples are PVSnesLib's, some of them Nintendo characters … **they are listed in `ATTRIBUTION.md` until then** ». C'est faux. Ces fichiers sont publiés : `make release-examples` (`Makefile:688-696`, `git ls-files examples | tar`) et `release.yml:229-240` mettent sources, assets **et ROM construites** dans `opensnes-examples_<tag>.zip`, sur chaque GitHub Release. Conséquence : à chaque release, des personnages Nintendo sont distribués sous une licence MIT qui ne les couvre pas, et un document public affirme une attribution qui n'existe pas. Effort : S pour la liste, L pour le remplacement des assets.

**11-2 🟠 (item de la porte 1.0) — Le « jeu vitrine (not a port) » n'avance pas.** `ROADMAP.md:52` dit « In progress outside this repository ». `~/workspace/opensnes-rpg` : dernier commit `ea3a0a8` du 2026-09-26 (un déplacement de doc) ; dernier commit de code `32762de` du 2026-08-18 ; 2 651 lignes de C. Le seul autre jeu actif sur le SDK est un projet tiers sous licence, privé, donc précisément ce que la ligne exclut. Conséquence : la case est notée « in progress » sur la foi d'un projet dormant depuis deux mois. → expert 06 pour la gouvernance de la porte.

**11-3 🟠 — Deux jeux sur huit seulement sont finis au sens « vrai jeu ».** Lecture des `main.c`, avec grep de `game over`, `score`, `sram`, `lives`, `STATE_` :

| Jeu | Lignes (c/h/asm) | Boucle titre→jeu→fin→rejouer | Score | Sauvegarde | Musique + SFX | Niveaux |
|---|---|---|---|---|---|---|
| tetris | 1 769 | oui (`STATE_TITLE…GAME_OVER`, `main.c:78-84`) ; fin non testée par manifeste | oui | non | musique seule | vitesse |
| breakout | 1 018 | oui, testée | oui, `hiscore` en RAM (`main.c:152`) | non | aucun son | 1 motif sur 4 (11-4) |
| likemario | 750 | non : pas d'ennemi, pas de but, chute = réapparition (`main.c:38`) | non | non | **oui** (seul) | 1 |
| mapandobjects | 544 | non : ennemis qui patrouillent, pas de mort | non | non | non | 1 |
| shmup_1942 | 394 | non : pas de mort du joueur, ni score ni HUD | non | non | non | 1 |
| rpg | 569 | non : 2 scènes, 1 coffre ; PV « static in this template » (`main.c:164`) | or | non | non | 2 scènes |
| mode7_racing | 246 | non : pas de tour, pas de chrono | non | non | non | 1 |
| mode7_flying | 274 | non : compteur d'atterrissages | non | non | non | 1 |

Aucun jeu ne sauvegarde. Un seul fait une transition d'écran (`rpg`, grep `fade|setBrightness|mosaic`). Ce que l'ensemble ne prouve pas : sauvegarde + musique + SFX + plusieurs niveaux + transitions **dans un même jeu**.

**11-4 🟠 — Le README de breakout, présenté comme le jeu de synthèse (« If you can follow this one, you can make a game »), enseigne du code qui n'existe plus.** `README.md:44` montre `REG_INIDISP = 0x80;`, alors que le code appelle `setScreenOff()` (`main.c:836`). `README.md:215` montre `pad0 = pad_keys[0];`, alors que le code fait `padHeld(0)` (`main.c:798`). `README.md:91-93` place `blockmap` « 2 KB at $0800 », alors qu'il est `extern FAR u16 blockmap[]` (`main.c:120`, dans `$7E:2000+`). Dans le même jeu :
- du code mort : `bg2map1-3` sont déclarés (`main.c:65`, `data.asm:82-86`, 6 Ko) et jamais lus, et la reco du 03/10 est inchangée ;
- des fichiers non lus par le build : `tiles2_original.dat`, `tiles1_tiles.png`, `tiles2_tiles.png`, `tiles1_color.png`, `tiles2_color.png` ;
- `mycopy()`, une copie octet par octet (`main.c:252`), là où `memcpy` suit les deux pointeurs 24 bits (constat du commit `15541dca`).

Conséquence : un débutant recopie le registre brut et l'adresse en bande basse, contre la règle du SDK.

**11-5 🟠 — Le skill `port-example` enseigne un outil retiré.** `SKILL.md:32-108` et `:236` imposent des « exact gfx4snes flags » et `$(GFX4SNES) … -t bmp`, et la checklist `:335` exige « gfx4snes flags match PVSnesLib exactly ». Or `ls bin/` ne liste que `opensnes`, `opensnes-image`, `-level`, `-music`, `-palette`, `-rom`, `-sample`, `-save`, `-sprite`, `-text` et `-tileset` ; `make/common.mk` ne connaît pas `GFX4SNES`. L'ancre 11 (`check_doc_drift.py`) cherche Mesen2, opensnes-emu et `tests/*.sh` dans `.claude/skills`, pas gfx4snes. Conséquence : tout portage guidé par ce skill échoue au premier `make`.

**11-6 🟠 — Le « RPG template » demande Python au développeur de jeu.** `rpg/README.md:91` : `python3 gen_assets.py --keep-map`, l'étape d'édition qui fait de l'exemple un *template*. Le README reconnaît que `opensnes-level` ne couvre pas la grille de collision (l. 110-117). Au total, 9 scripts `.py` sont lancés à la main dans `examples/` (rpg, mode7_racing, mode7_flying, shmup ×2, tetris, mode5_hires, soundboard, speech_synth). Conséquence : cela contredit la promesse de `two_audiences.md` (« Installs the zip, make, luna. Nothing else: no Python »), et cela sur l'exemple qui vend le pipeline Tiled.

**11-7 🟡 — Tetris enfreint le contrat que son propre en-tête rend obligatoire.** `snesmod.h:54` : « snesmodProcess() MUST be called every frame! Failure to do so will cause audio glitches ». Tetris ne l'appelle qu'une fois, dans la boucle principale (`main.c:706`). Ses boucles de pause (`:298`), de fin de partie (`:586`) et d'attente (`:617`) tournent sans l'appeler. Soit l'en-tête exagère, soit le jeu vitrine montre le contraire de la règle.

**11-8 🟡 — Le « mystère BG3 » de shmup a été effacé, pas élucidé ; la sonde d'aujourd'hui en donne la cause.** Le commit `15541dca` retire le passage du README (« is not kept on a user page ») sans instruction. Sonde luna (copie hors arbre, trame 120) :
- `TM_BG3` seul donne un motif fantôme (BG3 lit la carte et les tuiles à `$0000`) ;
- le module `text` avec ses défauts documentés (`text.h:233`, `textLoadFont(0x0000)`) écrase la carte BG1 du jeu, qui est à `$0000` en `SC_32x64` (`main.c:287`, `bgLoad(0, &scene, 0, 0x4000, 0x0000)`) : la moitié basse du terrain devient des tuiles de police.

C'est une collision de plan VRAM côté exemple, pas un défaut de la lib. Le HUD reste absent. Le même jeu garde son propre LCG (`prng_next`, `main.c:112`) au lieu de `rngNext()`.

**11-9 🟡 — Mesen2 est toujours recommandé.** 33 fichiers suivis le citent, dont `README_TEMPLATE.md:24` (« Open the .sfc in Mesen2 ») et des formules du type « Mesen2 recommended » (breakout, likemario, mode1, collision_demo…), alors que luna est l'émulateur du SDK et que `examples/README.md` le présente.

**11-10 🟡 — Les comptes d'exemples dérivent, et la sentinelle ne le voit pas.** `examples/README.md:3` : « 88 small ROMs ». `release.yml:290` : « the 86 examples' ». `find examples -name main.c | wc -l` donne 85. Les motifs de `check_doc_drift.py:190-205` ne reconnaissent ni « N small ROMs » ni « N examples' ». La ligne `basics/` du tableau annonce « randomness », qui n'a pas d'exemple.

**11-11 🟡 — De l'histoire datée et du jargon interne restent dans les sources d'exemples.**
- Histoire datée hors de la portée de l'ancre 18 : `tetris/render.c:26` (« Until 2026-10-03 ») et `:370` (« Until 2026-10-10 ») ; `likemario/main.c:149-159` (« Comment corrected 2026-10-04 ») ; `shmup_1942/main.c:380-382` ; `scene_stack/main.c:101` ; `breakout/data.asm:25` ; `superfx_hello/gsu_loader.asm:6` ; `sa1_starfield/Makefile:18`.
- Commentaire périmé : `tetris/render.c:3` dit « Bare-metal HDMA … avoids hdma module's large iris RAM », alors que le fichier appelle `hdmaSetup()`.
- Jargon de suivi interne dans 20 fichiers : « (#4, part 2) », « rung », « Effects arc 4/7 (#109) », par exemple `panel_hud/main.c:3`.

**11-12 🟡 — Doxygen mal employé.**
- 25 blocs `/** @brief */` orphelins dans 12 `main.c`, restes de la migration vers les `.inc` : `simple_sprite/main.c:35`, `parallax_scroll/main.c:38-39`, `windows/window/main.c:49-54`.
- 48 commentaires `/**<` posés sur des instructions dans 12 fichiers, par exemple `parallax_scroll/main.c:78-87`.
- `*(u16 *)&scroll_table[1]` (`parallax_scroll:84-96`) : un type punning que le débutant recopiera.

**11-13 🟡 — Des registres sont encore écrits à la main là où la lib a l'appel.**
- `REG_OBJSEL =` dans 5 fichiers : `collision_demo:424`, `breakout:878`, `mouse:124`, `superscope:137`, `aseprite_pipeline:122`. La macro est documentée (`sprite.h:71`), mais `oamInit()` existe.
- `REG_TM =` ×3 dans `dynamic_map:323,365,373`, au lieu de `setMainScreen`.
- `REG_BG2SC = 0x41` et `REG_BG12NBA = 0x50` dans `dsp1_ground:153-154`.
- `windows/window` encode des valeurs W12SEL brutes pour les re-décoder en masques du module fenêtre (`main.c:122-150`), afin de rester à l'identique de PVSnesLib.

**11-14 🟡 — Duplication et globals.**
- `build_solid_tile` ×3 (`direct_color:63`, `palette_cycle:102`, `shadow_tint:91`).
- `mario.c` ×2 : `mapandobjects` (135 l.) et `slope_collision` (154 l.), 77 lignes de diff.
- Trois exemples de carte sur le même asset `tiledMario`.
- **126 globals non `static` dans 49 fichiers** (101 au 03/10), par exemple `rpg/main.c:157-164`. Aucun ne collisionne avec un symbole de la lib (comparaison avec les 261 labels de `lib/source/*.asm`).

**11-15 🟡 — Des contournements expliqués, mais conservés.** `likemario/main.c:149-165` garde ses clips d'animation en RAM et explique en 9 lignes que `DECLARE_ANIM_CLIP` « would work here too ». L'exemple montre la mauvaise voie et la commente au lieu de montrer la bonne.

**11-16 🟡 — Des trous de couverture.** Aucun exemple pour : multitap (la lib ne peut pas l'armer, `input.h:107`), MSU-1, la région PAL, `profile*`, `easeIn/Out`, `debug`. HiROM et FastROM n'ont qu'un exemple chacun, et aucun jeu ne les utilise.

**11-17 🟡 — `new_example.md` n'exige ni la provenance des assets ni un manifeste luna.** `grep -n -i 'manifest\|ATTRIBUTION\|provenance' .claude/rules/new_example.md` ne renvoie rien. C'est le garde-fou qui aurait évité 11-1.

**11-18 🟡 — L'ordre KON / registres de `snesmod_sfx` (F n°4) reste non tranché.** L'arbitre (snesdev-wiki, chunk `6768cd86607515b5`) dit que KON « is polled every second sample » et remet l'enveloppe à 0 et le BRR au début. Il n'énonce aucun ordre VOL / P avant KON. Statut : hypothèse. `luna state --dsp-trace` existe désormais pour mesurer.

## Risques

- **Juridique :** la release 1.0 publierait des ROM et des sprites Mario, Goomba et Koopa sous une étiquette MIT, avec une phrase de la ROADMAP qui affirme une attribution inexistante (11-1).
- **Le HUD BG3 :** le premier utilisateur qui ajoute un HUD avec les défauts de `text` sur un plan VRAM qui a sa carte BG1 à `$0000` aura la même corruption que shmup. Les défauts du module ne sont pas sûrs avec un plan quelconque, et aucun exemple ne montre la cohabitation BG1 à `$0000` + texte (11-8).
- **La fin de partie de tetris** n'est couverte par aucun manifeste : une régression de la boucle de fin passerait la suite.
- **Le jeu vitrine dormant :** si la porte 1.0 est jugée sur un projet tiers sous licence, elle l'est sur un jeu invérifiable publiquement.
- **L'ancre 18 ne lit que `main.c` et les README :** l'histoire datée continue d'arriver dans `render.c`, `data.asm`, les Makefiles.

## Améliorations recommandées

| # | Action | Sévérité traitée | Effort | Premier pas concret |
|---|---|---|---|---|
| 1 | Lister les 55 fichiers (md5, source PVSnesLib, ayant droit) dans une section « Third-party assets » d'`ATTRIBUTION.md`, et corriger `ROADMAP.md:53` ; puis remplacer les assets Nintendo (likemario, mapandobjects, map_scroll, slope_collision) par des assets CC0 ou originaux, ou retirer ces exemples du zip d'exemples | 🔴 11-1 | S puis L | rejouer la comparaison md5 de ce rapport (voir le retour structuré) et commiter la liste |
| 2 | Dater et chiffrer le jeu vitrine, ou requalifier la ligne de la porte | 🟠 11-2 | S (décision) | une ligne propriétaire dans `ROADMAP.md:52` avec le dépôt, le dernier commit et le critère de « fini » |
| 3 | Réécrire la marche à suivre de `breakout/README.md` à partir du code réel ; supprimer `bg2map1-3`, les 5 fichiers non lus et `mycopy` → `memcpy` | 🟠 11-4 | S | `README.md:44`, `:91-93`, `:215` |
| 4 | Mettre `port-example/SKILL.md` aux outils `opensnes-*` et ajouter `gfx4snes` aux motifs de l'ancre 11 | 🟠 11-5 | S | remplacer « Rule 1-3 » par les fichiers `*.png.toml` |
| 5 | Faire porter la grille de collision du RPG par `opensnes-level` (settings `res/town.tmj.toml`), puis supprimer `gen_assets.py` | 🟠 11-6 | M | la reco est déjà écrite dans `rpg/README.md:115-117` |
| 6 | Faire de breakout le « jeu complet » : un SFX audio v2, le meilleur score en SRAM, les 4 motifs, un fondu entre niveaux, et un manifeste de la fin de partie de tetris | 🟠 11-3 | M | `state_tetris_game_over.toml` en premier (S) |
| 7 | HUD BG3 dans shmup avec un plan VRAM qui n'écrase pas BG1 (police hors `$0000`), et `rngNext()` ; un `@warning` dans `text.h` sur le défaut `$0000` | 🟡 11-8 | S | `textLoadFont(0x5000)` + `bgSetGfxPtr(2, 0x5000)` dans la sonde |
| 8 | Appeler `snesmodProcess()` dans les boucles d'attente de tetris, ou adoucir `snesmod.h:54` si la mesure luna montre que rien n'est perdu | 🟡 11-7 | S | `luna diff --audio` sur une pause de 300 trames, avec et sans |
| 9 | Remplacer « Mesen2 recommended » par luna dans les 33 fichiers et le template ; ajouter le motif à l'ancre 14 | 🟡 11-9 | S | `README_TEMPLATE.md:24` |
| 10 | Corriger « 88 » et « 86 » ; ajouter les motifs `N small ROMs` et `N examples'` à `COUNT_PATTERNS` ; scanner `.github/workflows` | 🟡 11-10 | S | `check_doc_drift.py:190` |
| 11 | Étendre l'ancre 18 à tous les `.c` / `.asm` / Makefiles d'exemples et au mot « corrected » ; purger le jargon `#N`, « rung », « arc » | 🟡 11-11 | S | `tetris/render.c:3,26,370` |
| 12 | Supprimer les `@brief` orphelins et les `/**<` posés sur des instructions (script de détection de ce rapport) | 🟡 11-12 | S | `simple_sprite/main.c:35` |
| 13 | `oamInit()` au lieu de `REG_OBJSEL` (5), `setMainScreen` au lieu de `REG_TM` (3), `bgSetMapPtr` / `bgSetGfxPtr` dans `dsp1_ground` | 🟡 11-13 | S | `dynamic_map:323` |
| 14 | Exiger dans `new_example.md` une ligne `ATTRIBUTION.md` par asset tiers, un manifeste luna et des globals `static` | 🟡 11-17, 11-14 | S | trois lignes de checklist |
| 15 | Fusions pédagogiques (liste ci-dessous) | 🟡 11-14 | M | `transitions/fading` → `mosaic` |

### Fusions et suppressions proposées (`STABILITY.md:47` les autorise)

| Proposition | Justification |
|---|---|
| `transitions/fading` → fondu dans `transitions/mosaic` | `mosaic` fait déjà « Mosaic and fade » : deux exemples pour le même `setBrightness` |
| `scrolling/mixed_scroll` → `scrolling/continuous_scroll` | deux exemples de parallaxe à deux couches au CPU ; `mode0` et `parallax_scroll` (HDMA) couvrent les autres cas |
| `sprites/dynamic_sprite` → `sprites/dynamic_metasprite` | même moteur `oamDynamic*`, le second est un sur-ensemble |
| `hdma/gradient_colors` → mode de `hdma/hdma_helpers` | le dégradé est déjà l'un des 4 effets de `hdma_helpers` ; garder `hdma_indirect_gradient` (mode indirect) |
| `maps/map_scroll` → `games/mapandobjects` ; un seul `mario.c` partagé avec `slope_collision` | trois exemples sur le même asset `tiledMario` et le même moteur `map` ; c'est aussi trois des exemples à assets Nintendo (11-1) |
| `games/likemario` + `games/mapandobjects` → un seul plateformeur à assets originaux | deux plateformeurs Mario, l'un « à la main » et l'autre « moteur objet » ; garder les deux techniques dans un jeu, sans Mario |
| `chips/sa1_hello`, `chips/superfx_hello` → `testing/` (ROM de diagnostic) | « boot diagnostic » par leur propre `@brief` : ce sont des tests, pas des leçons ; `sa1_starfield` et `superfx_3d` enseignent |

Net : 85 → environ 78 exemples, sans perdre de technique.

## Verdict

Le socle de test des exemples a le niveau d'une 1.0 : 85 sur 85 sous manifeste, régression visuelle verte, porte DMA sur les chemins pilotés par l'entrée, et les mensonges pédagogiques du 03/10 purgés et gardés par la sentinelle. Ce qui n'y est pas : la provenance des assets (55 fichiers PVSnesLib, dont des personnages Nintendo, distribués à chaque release sous MIT, sans attribution, alors que la ROADMAP affirme le contraire) et un vrai jeu complet (deux jeux sur huit bouclent, aucun ne sauvegarde, et le jeu vitrine dort hors dépôt depuis le 18/08). Le reste est du soin à faire en jours, pas en semaines : un README de breakout qui enseigne du code disparu, un skill de portage qui appelle un outil retiré, Mesen2, des Doxygen orphelins, des comptes faux.

## Suivi de l'audit du 03/10

| Constat | Statut au 17/10 | Preuve d'aujourd'hui |
|---|---|---|
| F n°3 tetris : `$420C` brut en milieu de trame, `hdma` absent | **corrigé** | `render.c:382-385` (`WaitForVBlank` + `hdmaSetup` + `hdmaEnable`), `Makefile:11` contient `hdma` ; il reste le commentaire périmé `render.c:3` (11-11) |
| F n°4 snesmod_sfx : ordre KON / registres | **partiel** | `luna state --dsp-trace` existe ; l'arbitre (`6768cd86607515b5`) ne tranche pas l'ordre (11-18) ; aucune réponse amont |
| F n°5 shmup : « BG3 enable hides BG1 » | **partiel** | retiré du README par `15541dca` sans instruction ; sonde luna du jour : collision VRAM côté exemple (police de `text` à `$0000` sur la carte BG1 à `$0000`), HUD toujours absent (11-8) |
| F n°6 likemario : `>>` « logical shift » | **corrigé** | `README.md:120` : « cc65816's `>>` on a signed value IS arithmetic » |
| F PF1 assets Nintendo sans attribution | **ouvert, aggravé** | `ATTRIBUTION.md` toujours muet ; 55 fichiers / 19 exemples identiques à PVSnesLib ; `ROADMAP.md:53` affirme une liste qui n'existe pas (11-1) |
| F PF2 porte DMA sur les chemins pilotés | **corrigé** | 19/19 manifestes `movement_*` / `state_*` avec `[asserts.dma]` ; `make test-manifests` 143/143 |
| F PF4 commentaires qui enseignent des bugs résolus | **corrigé** | grep des six motifs vide ; ancre 14 ; il reste de l'histoire datée hors de la portée de l'ancre 18 (11-11) |
| F PF5 jeux incomplets, `bg2map1-3` morts | **ouvert** | aucune sauvegarde, aucun SFX dans breakout, `bg2map1-3` toujours déclarés (`main.c:65`) et jamais lus (11-3, 11-4) |
| F PF6 jeu vitrine hors dépôt | **ouvert** | `opensnes-rpg` : dernier code le 2026-08-18 (11-2) |
| F PF7 contournements de la lib dans les exemples | **partiel** | tetris HDMA et `vblank_flag = 0` corrigés (grep vide) ; `REG_OBJSEL` ×5, `REG_TM` ×3, `REG_BG2SC` / `REG_BG12NBA` toujours là (11-13) |
| F PF8 Mesen2, README de games, exemples non cités, contrôles de shmup | **partiel** | `games/README.md` liste 8 jeux, chaque exemple est dans `LEARNING_PATH` (ancre 5), shmup a son tableau de contrôles ; Mesen2 reste dans 33 fichiers (11-9) |
| F PF9-11 duplication, globals, `@brief` jargon, `new_example.md` | **partiel** | `changeObjSize` n'est plus que dans 1 exemple ; `build_solid_tile` ×3, `mario.c` ×2 et `mycopy` restent ; 126 globals non `static` (101 avant) ; jargon dans 20 fichiers ; `new_example.md` toujours sans provenance ni manifeste (11-14, 11-11, 11-17) |
