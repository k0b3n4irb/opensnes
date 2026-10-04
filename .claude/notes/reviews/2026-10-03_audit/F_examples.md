# Audit F — exemples et jeux (2026-10-03)

Auditeur : agent `audit-examples`. `develop` @ `73fbdddb`, v0.47.0, luna v1.31.0, 89 exemples. Dépôt en lecture seule : aucun fichier suivi n'a été modifié, aucun exemple n'a été reconstruit. Les ROM existantes datent de 19 h 59-20 h 00, soit après `79c5ec73` ; les deux commits suivants ne touchent que le Super FX et la doc. Scripts et captures sont dans le scratchpad de la session.

## Défauts silencieux candidats

Méthode : ROM existantes lancées sur `tools/luna-test/bin/luna` au-delà des trames des manifestes, avec les boutons promis par les README.
- Pilotes en boucle fermée via `luna mcp` : un bot breakout qui suit la balle jusqu'au changement de niveau, un bot tetris jusqu'au game over puis la nouvelle partie, un bot rpg (coffre, PNJ, porte, maison, retour).
- Aller-retour de caméra sur 5 cartes.
- Balayage `--power-on` (zero/ones/random=7/random=4242) des 89 ROM à la trame 600, et des 8 jeux sous entrée à la trame 1300.
- `[asserts.dma] unsafe_writes = 0` rejoué **avec les entrées du README** sur 33 exemples. La porte `vram_dma_blank.py` le lance sans aucune entrée (`check_blank`, l. 66-78 : le manifeste généré ne contient que `rom` et `frames`).

| # | exemple | attendu | vu | commande | sévérité | couche probable |
|---|---|---|---|---|---|---|
| 1 | `games/tetris` | Aucun octet VRAM écrit pendant l'affichage actif (CLAUDE.md, première contrainte ; arbitre snesdev-wiki chunk `4e0a1bda9e0e98e5` : « Any VRAM writes during horizontal-blank or active-display will be ignored »). | **1 280 octets de tilemap BG1 écrits sur les lignes 16-24** de la trame 163, dès le premier verrouillage de pièce. Le bot en partie (2 200 trames) en compte 36 occurrences, une par verrouillage. Cause mesurée (MCP `run_until_mem_write $00420B` + `enable_cpu_trace`) : `renderFlush()` envoie d'abord BG2 (2 Ko, lignes 230-242), puis sa boucle de coalescence des 28 rangées coûte ~1 500 instructions, soit 65 462 mclk ≈ 48 lignes. Le détail : `renderFlush@while_body.146` 500, `@scmp.27` 357, `@logic_right.148` 316. Le DMA BG1 part donc après la fin du VBlank. **Aujourd'hui sans effet visible** : sur 1 280 octets, 2 seulement changent le contenu, et ils sont renvoyés en VBlank la même trame (rangée 3, délai 0). Les rangées marquées sales sont surtout identiques. | `luna test --jobs 0 <manifeste>` avec `frames = 2600`, `input = "90:0x1000,100:0,120:0x0400,…"` et `[asserts.dma] unsafe_writes = 0` → `FAIL tetris … 12800 VRAM byte(s) written during active display (first: frame 163 line 16 ch0 vram_word $0060 src $0008C0)` | 🟠 latent : un changement de rendu (rangées réellement modifiées, comme l'apparition d'une pièce sans descente immédiate) le rend visible sur console | exemple (`render.c:382-421`, budget VBlank : le commentaire l. 395-397 connaît le risque). Le compilateur aggrave : une boucle `u8` sur 28 rangées à ~54 instructions par rangée (→ aspect A). La porte de test ne le voit pas, faute d'entrée (→ aspect D). |
| 2 | `basics/scene_stack` | README l. 20 et `main.c` l. 31 : « Start: pops back to title ». | `scene_top` passe bien de 2 à 1 (trame 410), mais **l'écran reste figé sur « COUNTER: 197 / SELECT=PAUSE / START=TITLE »**. Le titre n'est jamais redessiné, puisque `title_init` ne se relance pas après un `scenePop` (`scene.h` l. 152, 211). Un second Start relance le compteur à 0, ce qui masque le défaut. Le manifeste `state_scene_stack.toml` ne vérifie que `scene_top`. | `luna state examples/basics/scene_stack/scene_stack.sfc --until-frame 450 --input "100:0x1000,104:0,200:0x2000,204:0,300:0x2000,304:0,400:0x1000,404:0" --screenshot …` ; `--peek scene_top:1` aux trames 399, 410, 450 donne `02`, `01`, `01` | 🟠 l'exemple qui enseigne le module démontre le piège sans le dire | exemple. API : `scene.h` l. 86 refuse le callback `resume` ; c'est documenté, mais l'exemple canonique tombe dedans. |
| 3 | `games/tetris` | Première image d'une partie entière. | Trame 105 (première image après Start) déchirée : lignes 0-~30 noires, dégradé HDMA en dessous, plateau sans pièce. Cause : `WaitForVBlank(); setScreenOff(); … setScreenOn(); renderEnableGradient();` (`main.c` l. 417-421, idem l. 564-568 et 590-594). `setScreenOn` et l'écriture brute de `$420C` (`render.c` l. 370) tombent en milieu de trame. | `luna frames examples/games/tetris/tetris.sfc --from-frame 96 -c 12 --input "90:0x1000,100:0"` → `frame_009_f105_live.png` | 🟡 cosmétique, une image | exemple. Il contourne aussi `hdmaSetup`/`hdmaEnableMask` : canal 6 programmé à la main, `render.c` l. 255-263 ; `hdma` absent de `LIB_MODULES`. |
| 4 | `audio/snesmod_sfx` (et `likemario`) | Une voix d'effet démarre avec son volume, son pitch et son enveloppe. | Ordre des écritures DSP de la voix 7, pour les 6 effets : `V7_SRCN, KON=$80, V7_VOLL, V7_VOLR, V7_PL, V7_PH, V7_ADSR1, V7_GAIN`. **VOL, pitch et enveloppe sont écrits après KON.** L'effet n'est pas mesurable : luna `--dsp-trace` donne `spc_cycles = 0` sur toutes les lignes (voir Risques). L'arbitre (snesdev-wiki chunk `6768cd86607515b5`) dit seulement que KON est échantillonné une fois sur deux. | `luna state examples/audio/snesmod_sfx/sfx.sfc --until-frame 800 --dsp-trace sfx.csv --input "100:0x80,104:0,200:0x8000,…"` | 🟡 hypothèse non tranchée | pilote SNESMOD (amont). À joindre au suivi `mukunda-/snesmod` si le corpus confirme. |
| 5 | `games/shmup_1942` | — | README l. 34-58 : « BG3 enable hides BG1 », un mystère jamais élucidé. Absent de `KNOWN_LIMITATIONS.md` ; non reproductible sans modifier la source. | — | 🟠 inconnu : peut être un défaut lib (`text`, BG3SC) ou une erreur de VRAM de l'exemple | à instruire sur luna avec une ROM de test dédiée |
| 6 | `games/likemario` (doc) | — | README l. 122-126 : « The compiler's `>>` operator uses logical shift », d'où `asr8()` (`main.c` l. 482). **Faux** : `cc65816` étend le signe (`xba/and #$FF/cmp #$80/bcc/ora #$FF00` pour `>>8` sur `short`). | `bin/cc65816` sur `short f(short v){return v>>8;}` | 🟠 la doc enseigne un faux bug de compilateur | exemple / doc |

**Ce qui a été cherché et n'a rien donné** (c'est aussi une mesure) :
- **Mémoire au démarrage** : 0 exemple sensible au contenu de la RAM à la mise sous tension. 89 ROM × 4 `--power-on` à la trame 600 donnent un `fbhash` identique ; 8 jeux × 3 états sous entrée à la trame 1300, identique aussi.
- **Défilement des cartes** : `map_scroll`, `tiled` et `likemario` donnent un rendu BG1 identique pixel pour pixel après un aller-retour de caméra (900 px → 400 → 900). Le moteur de carte ne laisse pas de colonne périmée. `slope_collision` et `mapandobjects` n'ont pas pu être replacés à la même caméra, à cause de leur zone morte (non concluant).
- **DMA en blank** : 32 des 33 exemples passent `unsafe_writes = 0` sous les entrées du README. Parmi eux : breakout (cycle complet), rpg, likemario, mapandobjects, dynamic_map (A/B/X/Y), hdma_helpers (A/B/X/Y + D-pad), mosaic, fading, window, extbg, pseudo_hires, superfx_game_skeleton, soundboard, snesmod_sfx.
- **Bot breakout** : 6 000 trames, 43 briques détruites, niveau 2 atteint (`level` 0→1, fond vert, « ROUND 2 »), 0 vie perdue, 0 octet hors blank (`take_dma_trace`). La pause par Start gèle bien la balle (`pos_x/pos_y` identiques aux trames 210 et 260).
- **Bot tetris** : Start puis game over (trame 2296), puis nouvelle partie (2365).
- **Bot rpg** : coffre +10 or (« YOU FOUND 10 GOLD! »), puis « THE CHEST IS EMPTY. », PNJ « WELCOME, TRAVELER! », porte puis maison (`scene` 0→1) et « WELCOME TO MY HOME! », tapis puis retour en ville en (42,43).
- **Musique + effet simultanés** (`likemario`) : deux `KON=$80` (voix 7) au milieu des KON de la musique, pour deux sauts.
- **`snesmod_sfx`** : A/B/X/Y/L jouent les sources `$40` à `$44`. D-pad droite fait passer le pitch de `$1000` à `$1800`.

## Périmètre couvert

- Inventaire : 89 `main.c` (audio 10, backgrounds 10, basics 8, chips 6, color 8, fundamentals 1, games 8, hdma 4, input 5, maps 4, memory 2, mode7 5, scrolling 3, sprites 8, text 2, transitions 2, windows 3). 18 360 lignes de C suivies (`git ls-files examples | grep -E '\.(c|h)$' | xargs cat | wc -l`). Assets suivis : 3,8 Mo. Nouveaux depuis le 2026-09-26 : `mode4`, `mode6`, `superfx_game_skeleton`, `pseudo_hires`, `extbg`.
- Lecture intégrale de 6 `main.c` (`print_string`, `simple_sprite`, `hdma_wave`, `windows/window`, `breakout`, `shmup_1942`), plus `tetris/render.c`, `scene_stack/main.c`, `dynamic_map/main.c` l. 379-470, et les README des 8 jeux.
- Exécution luna (v1.31.0) : ~60 `state`/`frames`, 4 bots MCP, 33 manifestes générés (`luna test --jobs 0`), 445 runs `--power-on`, 2 traces DSP, 3 traces CPU/DMA.
- Statique : greps de motifs périmés, des écritures de registres directes, des duplications et de la matrice de couverture. `md5sum` croisé avec `~/workspace/pvsneslib/snes-examples`. `make lint-docs` (vert, 89).
- Comparaison point par point avec l'annexe F du 2026-09-26 (`reviews/2026-09-26_etat_des_lieux.md` l. 605-720).

## Points forts

- **Aucun exemple ne lit de mémoire non initialisée** : 356 runs sur 89 ROM et 4 états de RAM donnent un `fbhash` identique à la trame 600 ; 24 runs sur les jeux sous entrée aussi. Résultat fort pour un corpus de cette taille.
- **Le moteur de carte est cohérent en aller-retour** : BG1 identique au pixel après 900 → 400 → 900 px sur `map_scroll`, `tiled` et `likemario` (`rt3.py`, sortie `SAME`).
- **Breakout est désormais un jeu complet et rejouable** :
  - le cycle game over → titre → nouvelle partie est épinglé par `state_breakout_game_over.toml` ;
  - le bot atteint le niveau 2 sans perdre de vie et sans octet VRAM hors blank ;
  - la pause marche.
- **Tetris boucle** titre → jeu → game over → nouvelle partie (bot : états 0 → 1 → 3 → 1 aux trames 100/105/2296/2365).
- **Le RPG tient toutes ses promesses de README** : coffre, coffre vide, 2 PNJ, porte, scène intérieure à palette propre, retour par le tapis (6 captures, `rpg_bot2.py`). La collision des maisons et de l'étang est dans les données (`town_collision.bin`, 386 tuiles pleines).
- **Musique et effets simultanés prouvés au niveau du DSP** (likemario, `KON=$80` pendant la musique), et 5 effets distincts + pitch sur `snesmod_sfx`.
- **Trous de couverture comblés depuis le 26/09** :
  - Mode 4, Mode 6, pseudo-hires et EXTBG ont chacun leur exemple ;
  - `hdma_wave_table` est fusionné (`855bf2c7`) ;
  - `encode_4bpp` ×4 est remplacé par `tileEncode4bpp` (`lib/include/snes/tile.h:46`, `7b776e0b`) ;
  - `windows/window` et `hdma_wave` passent par l'API (`windowEnable/SetInvert/SetMainMask` l. 159-167 ; `hdmaSetup`, `hdmaWaveH`, `hdmaEnableMask`).
- **`port-example/SKILL.md` corrigé** : Phase 6 via `luna_runner.py` (l. 249), Pitfall 7 dit 2/4 octets (l. 291), plus de `oamMemory` ni de `spcLoad` prescriptifs.
- **Commentaires « peur du compilateur » purgés dans tetris et breakout** (`38c4b7ed`).

## Points faibles

1. 🔴 **Assets Nintendo toujours livrés sans attribution** (inchangé depuis le 26/09).
   - 40 fichiers dans 23 exemples sont md5-identiques à PVSnesLib, dont `games/mapandobjects/res/{mario,goomba,koopatroopa,tilesMario}.png`, `games/likemario/res/{tiles.png,overworld.it,mariojump.brr}` et `maps/map_scroll/res/{mario,tilesMario}.png`.
   - `grep -niE 'nintendo|mario|goomba|koopa' ATTRIBUTION.md` ne renvoie rien.
   - Conséquence : chaque zip de release redistribue de la propriété intellectuelle tierce sous bannière MIT. Bloquant pour 1.0. Effort M.
2. 🟠 **La porte « DMA en blank » ne voit aucun chemin piloté par l'entrée**.
   - Défaut n° 1 du tableau : tetris viole la règle à chaque verrouillage de pièce.
   - Il est sans effet aujourd'hui seulement parce que les rangées renvoyées sont identiques. La boucle `u8` de 28 tours coûte ~48 lignes.
   - Conséquence : le premier changement de rendu (ou de codegen) en fait un défaut visible sur console, jamais sur la CI. Effort S (exemple) + S (porte, → aspect D).
3. 🟠 **`scene_stack`, l'exemple canonique du module scene, est faux à l'écran** (défaut n° 2).
   - Le manifeste ne vérifie que la profondeur de pile.
   - Conséquence : le lecteur apprend que `scenePop` « revient au titre » alors que l'écran reste figé ; le README promet ce que la ROM ne fait pas. Effort S (ajouter un `title_redraw` au premier `update` après reprise, ou un `scene_resumed()`).
4. 🟠 **Commentaires et README qui enseignent encore des bugs résolus ou inexistants** :
   - `likemario/README.md` l. 122-126, `>>` logique : **faux**, voir défaut n° 6 ;
   - `shmup_1942/main.c:379` et `input/superscope/README.md:86` : « oamSet framesize=158 » ;
   - `shmup_1942/README.md:38-42` : « tiny strings spill to bank 1 = garbage », faux depuis #127.3 ;
   - `windows/window/main.c:60-67` : « hdmaSetup() assumes bank $00 », faux depuis A6 ;
   - `scrolling/parallax_scroll/main.c:44` : « must be in bank $00 WRAM » ;
   - `input/two_players/README.md:87` : « compiler quirk ».

   Effort S.
5. 🟠 **Jeux toujours incomplets au sens « vrai jeu »** (mesures par grep et par les bots) :

   | Jeu | `main.c` / dossier | Fin de partie | Redémarrage | Audio | Sauvegarde | Niveaux |
   |---|---|---|---|---|---|---|
   | tetris | 725 / 1 601 | oui | oui | musique, 0 effet | non | niveaux de vitesse |
   | breakout | 931 | oui | oui (corrigé) | aucun | non | même motif ×∞ (`bg2map1-3` déclarés l. 65, jamais lus) |
   | likemario | 763 | non | — | musique + effet | non | 1 |
   | mapandobjects | 147 / 519 | non | — | aucun | non | 1 |
   | shmup_1942 | 392 | non (pas de vies) | — | aucun | non | 1 ; HUD BG3 abandonné |
   | mode7_racing | 235 | non (0 tour) | — | aucun | non | 1 |
   | mode7_flying | 262 | non | — | aucun | non | 1 |
   | rpg | 534 | non (pas de combat) | — | aucun | non | 2 scènes |

   Aucun jeu ne sauvegarde (`grep -ril sram examples/games` = 0), y compris `superfx_game_skeleton`, alors que `c937bb0b` vient d'ouvrir `USE_SRAM` au Super FX. Aucun ne réunit transitions + musique + effets + sauvegarde. Effort M par jeu.
6. 🟠 **Le jeu vitrine reste hors dépôt**.
   - ROADMAP l. 229 le dit maintenant honnêtement : « outside this repository … moved to its own repository on 2026-09-26 ».
   - `git ls-files projects | wc -l` = 0. `git log -3 -- examples/games/rpg projects/` : dernier commit `1db4e88b` du 2026-08-08. `projects/rpg` a son propre `.git`.
   - Dans ce dépôt, `games/rpg` est un gabarit (coffre, PNJ, porte), pas un jeu : ni combat, ni fin, ni sauvegarde.
   - Effort L, ou rien si on assume que la preuve « jeu complet » vit ailleurs.
7. 🟠 **Contournements de la lib restant dans les exemples** (indices de manques de l'API) :
   - tetris programme le HDMA à la main (`render.c` l. 255-263, 370) ;
   - `REG_OBJSEL` écrit en brut dans 6 exemples (`breakout:890`, `mouse:123`, `superscope:137`, `collision_demo:427`, `aseprite_pipeline:126`, `metasprite:196-200`). `oamInit()` vide l'OAM et aucune fonction ne change la taille OBJ à chaud (→ aspect B) ;
   - `REG_BG2SC`/`REG_BG12NBA` bruts dans `mode7/perspective:113-114` et `dsp1_ground:156-157` ;
   - `REG_TM` brut dans `dynamic_map:344/386/394` ;
   - `vblank_flag = 0` écrit par `dynamic_map:398,458` et `metasprite:190`, alors que `system.h` l. 55-66 l'interdit (« Writing the flag from user code breaks the handshake »). Sans effet aujourd'hui, mais l'exemple enseigne l'ancien protocole.

   Effort M.
8. 🟡 **Dérive documentaire** :
   - Mesen2 encore recommandé (`examples/README.md:132-145`, `README_TEMPLATE.md:24`, 42 README d'exemples) ;
   - `examples/games/README.md` ne liste que 3 jeux sur 8 (l. 11-13) ;
   - 30 exemples ne sont cités nulle part dans `examples/README.md` (tetris, shmup, les 6 `chips/`, 7 `basics/`…) ;
   - `shmup_1942` n'a pas de tableau de contrôles (A = tir, `main.c:372`).

   Effort S.
9. 🟡 **Duplication** :
   - `build_solid_tile` ×3 (`color/{palette_cycle,shadow_tint,direct_color}`) ;
   - `changeObjSize` ×3 (`sprites/{metasprite,sprite_sizes,dynamic_metasprite}`) ;
   - `mario.c` ×2 (115 lignes de diff) ;
   - `mycopy` dans breakout l. 251, faute de `memcpy` dans la lib.

   Effort S.
10. 🟡 **Globals de fichier sans `static`** : 41 `main.c` sur 89, 101 globals. En hausse : le nouvel exemple `superfx_game_skeleton` en ajoute 5.
11. 🟡 Autres points mineurs :
    - 11 `@brief` portent encore « Family/rung/S5 » ;
    - 3 en-têtes Doxygen incomplets, toujours les mêmes (`sfx_from_wav`, `fix32_orbit`, `sa1_starfield`) ;
    - `.claude/rules/new_example.md` n'exige ni provenance d'assets ni manifeste luna.

#### Matrice de couverture (grep sources + Makefiles)

| Fonction | # | | Fonction | # |
|---|---|---|---|---|
| Mode 0 / 1 / 2 / 3 | 6 / 37 / 1 / 7 | | SRAM | 1 (`save_game`) + 2 puces |
| Mode 4 / 5 / 6 | 1 / 2 / 1 | | SA-1 / Super FX / DSP-1 | 2 / 3 / 2 |
| Mode 7 / EXTBG | 7 / 1 | | SNESMOD / audio v2 | 6 / 7 |
| HDMA / fenêtres / color math | 11 / 3 / 5 | | Souris / Super Scope | 1 / 1 |
| Mosaïque / direct color / pseudo-hires | 1 / 1 / 1 | | **Multitap / MSU-1 / PAL** | **0 / 0 / 0** |
| Hi-res 512 + interlace | 2 | | Offset-per-tile | 3 (mode2, 4, 6) |
| HiROM | 1 | | Musique + effet simultanés | 1 (likemario) |

#### Fusions / suppressions proposées

Elles restent valables depuis le 26/09, sauf celle de `hdma_wave_table`, faite :
- `mode7/perspective` → `mode7/dsp1_ground` : mêmes `sky.png` et `ground.png` ;
- `maps/map_scroll` → `maps/tiled` : même leçon, mêmes assets Mario ;
- `basics/random` + `basics/timer` + `input/controller` → un exemple `gameloop` ;
- supprimer `audio/snesmod_music_large` : même leçon que `snesmod_music` plus un mapper.

## Risques

- **La vérification d'exemple repose sur des instants précoces** : les manifestes de `scene_stack` et de `tetris` ne vérifient que des variables. Les défauts n° 1 et n° 2 sont sortis dès qu'on a appuyé sur les boutons du README, au-delà des trames testées. D'autres du même type sont probables dans les 56 exemples non pilotés ici.
- **Le défaut n° 1 est un piège armé** : toute évolution du rendu de tetris qui modifie vraiment des rangées au verrouillage d'une pièce (pièce fantôme, animation, nouvelle pièce visible plus tôt) produit une image fausse sur console. luna rend la ROM correctement, et la porte CI ne passe pas par ce chemin.
- **Partenaire luna** : `luna state --dsp-trace` écrit `spc_cycles = 0` sur toutes les lignes en v1.31.0. Constaté sur `likemario` (80 589 lignes, 1 valeur unique) et `echo` (92 lignes, 1 valeur unique) ; la commande est dans le tableau, défaut n° 4. Sans horodatage, la question « registre écrit avant ou après le KON effectif » est invérifiable. **À reporter dans `partners/luna/OPEN_luna.md`**, je ne l'ai pas fait (lecture seule).
- **L'exposition juridique croît avec la visibilité 1.0** : baselines `fbhash`, captures et tutoriels renvoient aux assets Mario, et leur retrait coûtera plus cher à chaque release.
- **Le mystère BG3 de `shmup_1942`** reste dans un README depuis mai. S'il s'agit d'un défaut de `text` ou de BG3SC, il touche tout jeu qui ajoute un HUD sur BG3.

## Améliorations recommandées

| # | action | sévérité traitée | effort | premier pas concret |
|---|---|---|---|---|
| 1 | Remplacer les assets Nintendo (likemario, mapandobjects, map_scroll, slope_collision) et attribuer le reste dans `ATTRIBUTION.md`, exemple par exemple | 🔴 PF1 | M | Commiter la liste md5 (40 fichiers, 23 exemples) comme section « Third-party assets » |
| 2 | Faire passer `vram_dma_blank.py` par les scripts d'entrée des manifestes de jeux (ou ajouter `[asserts.dma]` aux 9 manifestes `movement_*`/`state_*`) | 🟠 PF2 | S | Ajouter `[asserts.dma] unsafe_writes = 0` à `state_tetris.toml` : il échoue aujourd'hui, ce qui confirme le constat |
| 3 | Tetris : ne pas marquer sales des rangées inchangées et vider BG1 avant BG2, ou découper BG2 sur deux trames ; retirer le HDMA écrit à la main au profit de `hdmaSetup` + `hdmaEnableMask` ; ajouter un `WaitForVBlank()` avant `setScreenOn()` | 🟠 PF2, 🟡 défaut n° 3 | S | Mesurer `renderFlush` avec `luna profile` avant et après |
| 4 | `scene_stack` : redessiner le titre à la reprise ; ajouter au manifeste un `fbhash` ou une assertion VRAM après le pop | 🟠 PF3 | S | `title_update` : un drapeau `needs_redraw` posé par `counter_update` avant `scenePop()` |
| 5 | Purger les 6 commentaires/README périmés de PF4 et ajouter leurs motifs au sentinel `check_doc_drift.py` (`framesize=158`, `logical shift`, `assumes bank \$00`, `spill to bank 1`) | 🟠 PF4 | S | `grep -rn -E 'framesize|logical shift|assumes bank|spill to bank' examples` |
| 6 | Instruire le mystère BG3 de shmup sur luna (ROM de test, `assets-dump` BG3) ; selon le résultat, une entrée `KNOWN_LIMITATIONS.md` ou un correctif `text` | 🟠 défaut n° 5 | S-M | Reprendre le diff S6 depuis l'historique, construire hors arbre |
| 7 | Ouvrir à luna : horodatage de `--dsp-trace` ; puis trancher l'ordre KON/registres de SNESMOD avec le corpus (`snes_search contrast=true`) | 🟡 défaut n° 4 | S | Une ligne dans `OPEN_luna.md` avec la commande du tableau |
| 8 | Un jeu « vrai » de bout en bout dans le dépôt : breakout avec les 4 motifs, un effet SNESMOD et le meilleur score en SRAM | 🟠 PF5 | M | Lire `bg2map1-3` dans `new_level()` (l. 407) ; `sramSave` du meilleur score |
| 9 | Fonction lib « changer OBJSEL sans vider l'OAM » ; migrer les 6 écritures brutes de `REG_OBJSEL`, puis `REG_TM`/`BGxSC` vers l'API ; supprimer les `vblank_flag = 0` | 🟠 PF7 | M | → aspect B pour l'API ; puis `metasprite` |
| 10 | Docs d'exemples : Mesen2 → luna, `games/README.md` à 8 jeux, 30 orphelins dans `examples/README.md`, tableau de contrôles de shmup | 🟡 PF8 | S | Étendre l'ancre 5 de `check_doc_drift.py` à « tout exemple cité au moins une fois » |
| 11 | Fusions (4 ci-dessus), `build_solid_tile`/`changeObjSize` factorisés, `static` sur les globals, `@brief` nettoyés | 🟡 PF9-11 | M | Commencer par `perspective` → `dsp1_ground` |

## Verdict

Le corpus est plus large et plus propre qu'au 26/09 : 89 exemples, quatre trous de couverture comblés, les contournements les plus criants retirés. Il est aussi robuste : aucun exemple ne dépend de la RAM au démarrage, la carte est cohérente en aller-retour, et breakout, tetris et rpg se jouent de bout en bout sous un bot. La chasse a pourtant sorti deux défauts invisibles aux tests : tetris écrit 1 280 octets de VRAM en plein affichage à chaque pièce posée, sans effet aujourd'hui seulement parce que le contenu est redondant, et `scene_stack` affiche un écran figé là où il promet le titre. Les deux ont la même cause : la porte DMA et les manifestes ne rejouent pas les boutons du README. Pour un SDK 1.0, restent bloquants les assets Nintendo non attribués, et un « vrai jeu » (sauvegarde, effets, plusieurs niveaux) qui n'existe toujours pas dans le dépôt.
