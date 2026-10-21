# Refonte des assets : revue des exemples et besoins en assets

Date : 2026-10-18. Base : `develop` à `17a78521`. Lecture seule : README et `main.c` de chaque exemple,
`Makefile` (LIB_MODULES), `res/`, l'inventaire mécanique (`inventory.tsv`), `docs/HARDWARE_VERIFICATION.md`,
`testing/manifests/`, `ATTRIBUTION.md`, la note de chantier `assets_rebrand.md`.
Direction retenue par le propriétaire : **cyberpunk, personnages animaux anthropomorphes, palette nuancée**
(néons sourds posés en couches sur des tons profonds, pas d'arc-en-ciel saturé).

---

## 1. Résumé

> **Avancement au 2026-10-19 :** les fusions de la section 2 sont faites (60 exemples), et les cinq
> **Fix** sont sous `testing/fixtures/retired_examples/` (`mode6`, `sa1_hello`, `sa1_save`,
> `superfx_hello`, `superfx_save`). Les lignes 23, 25 et 26 du protocole matériel les nomment par
> leur chemin ; `make hardware-kit` et `make hardware-preflight` construisent ces fixtures avant de
> lire la table. `superfx_3d` est gardé (propriétaire, 2026-10-19). Le tutoriel
> SA-1 porte la sauvegarde en BW-RAM que `sa1_save` enseignait.

- **85 exemples** (le `examples/README.md` annonce « 88 small ROMs » alors que son propre tableau totalise 85 :
  l'ancre 3 de `check_doc_drift.py` ne voit pas cette formulation).
- Verdicts proposés : **31 gardés** (même leçon, nouveaux assets) · **27 recadrés** (la plupart absorbent
  un ou plusieurs voisins ; 4 sont renommés parce que leur nom vient d'une autre propriété ou d'une marque) · **21 fusionnés** dans
  un autre · **5 déplacés en fixtures** (diagnostics de puce et ROMs de protocole, pas des cas d'usage) ·
  **1 supprimé** (`maps/dynamic_map`).
- Résultat : **58 exemples** au lieu de 85, chaque module de `lib/` restant couvert par au moins un exemple
  gardé (vérifié sur les LIB_MODULES : `lzss`, `mosaic`, `scene`, `vramqueue`, `sprite_dynamic_meta`,
  `fixed32`, `object`, `panel`, `collision`, `apu`, `audio`, `sram` ont tous un exemple qui les garde).
- **49 assets partagés distincts** au catalogue (section 3) : 6 variantes du héros, 4 ennemis/machines,
  1 lot de PNJ, 2 véhicules, 7 familles d'objets, 1 lot de particules, 5 tilesets, 7 fonds, 3 textures
  Mode 7, 2 kits d'UI, 3 polices, 1 logo, 4 musiques/banques, 3 familles de sons.
- 8 palettes OBJ partagées par **tous** les exemples (une carte fixe), environ 20 palettes BG 4bpp tirées
  d'une même palette maîtresse en rampes, 2 palettes 256 couleurs.
- Ordre de grandeur : **600 à 800 générations PixelLab** au total, essais et retouches compris ; la musique
  (3 modules + 1 banque d'effets) passe par une commande à un musicien.

---

## 2. Revue des exemples

Légende du verdict : **G** garder · **R** recadrer · **F→x** fusionner dans x · **Fix** déplacer en fixture
(testing, plus dans `examples/`) · **S** supprimer. « Procédural » = dessiné par le code ou par un script du
dépôt, pas d'image source. « PVSL » = fichiers identiques à PVSnesLib (inventaire). Les ids d'assets
renvoient à la section 3.

Le monde proposé (à valider, §6) : **Néo-Lumen**, une cité nocturne en couches. Les *Toits* (néons, câbles,
passerelles : le plateformer), la *Rue basse* sous la pluie (le RPG, les écrans d'effets), le *Canal*
(reflets, eau), le *Ciel* (vue aérienne, Mode 7, le shmup), les *Terminaux* (texte, menus, sauvegardes),
l'*Arcade* d'un bar (casse-briques, puzzle). Héros : **Kit**, un renardeau coursier (un *kit* est un
renardeau : le nom de la note de chantier reste, la mascotte devient un animal). Adversaires : les machines
de la sécurité de la ville (drones, automates, insectes-robots).

### text (3 → 2)

| Exemple | Capacité enseignée | Aujourd'hui | Verdict | Scène proposée | Assets |
|---|---|---|---|---|---|
| text/print_string | `textModeInit` + `textPrintAt`, la poignée de main VBlank ; ROM de fumée (ligne 1 du protocole matériel) | police intégrée, aucun asset | **G** — le plus petit ROM, doit le rester | l'écran de démarrage d'un terminal : « OS: » puis une ligne | F1 |
| text/scroll_message | `bgSetScroll` sur le calque de texte (marquee) | police intégrée | **F→scrolling/mixed_scroll** — le défilement est enseigné par scrolling/ ; une ligne qui défile appartient à l'écran-titre | (le bandeau défilant de l'écran-titre) | F1 |
| text/pixel_text | texte proportionnel composé en RAM, `tileBlitGlyph2bpp` | police 5x7 déclarée en bits, procédural | **G** — seul exemple de texte hors grille | l'écran d'un terminal de bar : messages en police étroite, A les décale | F2, U1 |

### fundamentals (1 → 1)

| Exemple | Capacité | Aujourd'hui | Verdict | Scène | Assets |
|---|---|---|---|---|---|
| fundamentals/text_glyphs | tuiles 2bpp écrites à la main dans la VRAM, sans module | 9 tuiles en tableau C | **G** — le « sous le capot » ; garder procédural, redessiner les 9 glyphes dans le style de F1 | « HELLO WORLD! » dans la police de la ville | F1 (glyphes recopiés en C) |

### basics (7 → 5)

| Exemple | Capacité | Aujourd'hui | Verdict | Scène | Assets |
|---|---|---|---|---|---|
| basics/aim_target | `sqrt16`, `atan2_8`, `fixCos/fixSin` : viser | losange + croix 8x8, texte des valeurs, procédural | **R** — absorbe `fix32_orbit` : une tourelle vise le drone que le joueur déplace, un second drone orbite en 16.16 ; les chiffres restent à l'écran | une tourelle de sécurité sur un toit vise un drone | E1, O3, O4, P1, F1 |
| basics/collision_demo | AABB `collideRect` + collision de tuiles `collideTile`, glissement par axe | carrés procéduraux ; enseigne aussi l'écriture directe dans `oamMemory[]` « pour éviter les 158 octets de pile d'oamSet » | **R** — retirer la leçon de contournement (`oamMemory[]` direct, chiffre d'un ABI passé) ; garder les deux collisions | Kit dans un entrepôt de caisses, des drones fixes changent de couleur au contact | K5, E1, T2, O2 |
| basics/fix32_orbit | `fix32Sin/Cos/Mul`, un point en orbite | un point 8x8 procédural | **F→basics/aim_target** — même famille (math du mouvement), une seule figure ; le manifeste `movement_fix32_orbit` suit | (le drone en orbite) | — |
| basics/game_skeleton | la machine à états d'un jeu (titre → jeu → fin) autour d'une boucle | tuiles et sprites procéduraux, texte | **G** — le squelette à forker ; il devient le premier écran « dans le monde » | Kit ramasse des éclats de données avant la fin du chrono, écran-titre au logo | K5, O1, B5, U1, F1, T2 |
| basics/panel_hud | module `panel` : HUD + boîte de dialogue 9-slice sur un calque | feuille 3x3 procédurale | **R** — absorbe `mode1_bg3_priority` : le HUD passe sur BG3 haute priorité au-dessus d'une scène Mode 1, la leçon « HUD au-dessus du décor » au même endroit que le module qui le dessine | une rue, la barre d'état de Kit, un PNJ ouvre une bulle | U1, B2, F1 |
| basics/scene_stack | le cadre opt-in `scene` (push/pop, pause qui reprend) | texte seul | **G** — l'alternative au switch de game_skeleton (opt-in, PHILOSOPHY) ; reprendre les assets de game_skeleton pour que la comparaison soit directe | titre → compteur → pause, sur l'écran-titre de la ville | B5, U1, F1 |
| basics/timer | `frame_count`, `WaitForVBlank`, nombre à l'écran | texte seul | **F→input/controller** — trois lignes de leçon, déjà vues par print_string ; le compteur de trames rejoint l'écran de diagnostic de la manette | (compteur de l'écran de diagnostic) | — |

### backgrounds (9 → 5)

| Exemple | Capacité | Aujourd'hui | Verdict | Scène | Assets |
|---|---|---|---|---|---|
| backgrounds/mode0 | Mode 0 : 4 calques 2bpp, 4 palettes, défilements à vitesses différentes | 4 BMP PVSL (scène « style Kirby » selon son README) | **R** — garder la leçon, tout redessiner : 4 plans de la skyline en 3 couleurs chacun | la ville vue de loin, 4 plans en silhouette | B1 (découpe 2bpp) |
| backgrounds/mode1 | le mode courant : un fond 4bpp, tuiles/carte/palette | le logo actuel 256x256 | **R** — absorbe `mode1_lz77` : la même image chargée brute ou compressée (A recharge l'autre), la taille gagnée affichée | la rue sous la pluie, l'illustration-vitrine | B2, F1 |
| backgrounds/mode1_bg3_priority | `BG3_MODE1_PRIORITY_HIGH` : HUD au-dessus de BG1/BG2 | 3 PNG (2-4 couleurs) | **F→basics/panel_hud** — c'est la technique du HUD ; enseignée seule, elle n'a pas de cas d'usage visible | (le HUD de panel_hud) | — |
| backgrounds/mode1_lz77 | `lzssDecodeVram`, `lz = true` dans le réglage | le logo actuel | **F→backgrounds/mode1** — même image, même mode, une option du réglage | — | — |
| backgrounds/mode2 | offset-per-tile : BG3 table de décalages par colonne | bandes procédurales | **R** — absorbe `mode4` (A bascule Mode 2 / Mode 4, même chemin de données) ; image réelle : brume de chaleur au-dessus des bouches de vapeur | la rue, une brume ondule par colonnes | B2 |
| backgrounds/mode3 | Mode 3 : un fond 8bpp de 256 couleurs, tileset en deux DMA | une image PVSL de 237 couleurs | **R** — même leçon, image à nous | le panorama du canal la nuit, 256 couleurs | B3 |
| backgrounds/mode4 | OPT avec un BG1 8bpp, un mot par colonne | dégradé procédural | **F→backgrounds/mode2** | — | — |
| backgrounds/mode5_hires | Mode 5 + entrelacement 512x448, l'ombre de SETINI | page de texte générée par `gen_assets.py` (à nous) | **G** — ligne 8 du protocole ; seul exemple hi-res vrai | une page de journal ou de terminal en 512 colonnes | F3 |
| backgrounds/mode6 | Mode 6 : OPT en hi-res, mire pour une question ouverte | bandes procédurales + mire | **Fix** — c'est une question posée au matériel (ligne 23), pas un cas d'usage ; la mire reste un ROM de protocole | — | — |

### sprites (8 → 7)

| Exemple | Capacité | Aujourd'hui | Verdict | Scène | Assets |
|---|---|---|---|---|---|
| sprites/simple_sprite | un sprite 32x32 : tuiles, palette, OAM | un PNG 32x32 | **G** | Kit en grand, immobile, au centre | K2 |
| sprites/sprite_sizes | les 6 combinaisons d'OBJSEL | 4 PNG 8/16/32/64 | **G** — mais les quatre tailles doivent être le même personnage pour que la leçon se voie | Kit en 8, 16, 32, 64 ; **fait le 2026-10-21** (le 8 à la main dans `build.py`, le 16 et le 64 par PixelLab, le 32 est `kit.png`) | K4, K1, K2, F1 |
| sprites/animated_sprite | animation par directions, flip horizontal, module `anim` | feuille 16x144 (portage PVSL) | **G** | Kit vue de dessus marche dans 4 directions sur le trottoir | K5, T2 |
| sprites/aseprite_pipeline | la chaîne Aseprite → `opensnes-sprite sheet` + `anim` (tags, durées) | héros 32x48 PVSL + JSON | **G** — la leçon est la chaîne : il faut un vrai fichier `.aseprite` à deux tags | un grand Kit (ou un PNJ) : « walk » et « wave » ; **fait le 2026-10-21** : Kit 32x48 par PixelLab, ses deux animations par `animate-with-text-v3` (pas de fichier `.aseprite` : le PNG et le JSON ont la forme de l'export d'Aseprite) | K3 |
| sprites/dynamic_metasprite | `oamMetaDrawDyn` : métasprite en streaming, 3 configurations d'OBJSEL | 3 feuilles PVSL | **F→sprites/dynamic_sprite** — même moteur (streaming VRAM), le métasprite est l'étape suivante du même exemple | — | — |
| sprites/dynamic_sprite | streaming des tuiles d'animation en VRAM | 2 feuilles (une PVSL) | **R** — absorbe dynamic_metasprite : 4 éclats tournants en 16x16 puis un grand Kit métasprite, D-pad change la paire de tailles | des éclats de données tournent, Kit passe en grand ; **fait le 2026-10-21** : l'éclat en 24 trames (`spin`), Kit en 16, 32x48 et 64 sur une palette | O1 (24 trames), K2, K3, K4 |
| sprites/sprite_swarm | le coût par sprite, plafond honnête à 60 i/s | points procéduraux | **G** — leçon chiffrée, garder ; un vrai sprite rend le nombre parlant | un essaim de papillons-néons autour d'une enseigne | P1, B2 |
| sprites/world_sprites | `oamPlaceWorld` : caméra, culling, 9e bit x, tri en profondeur | carrés procéduraux | **G** — exactement le cas d'un jeu vu de dessus | une place de la Rue basse, éclats au sol, drones en patrouille triés par profondeur | O1, E1 (32x32), T2 |

### hdma (4 → 3)

| Exemple | Capacité | Aujourd'hui | Verdict | Scène | Assets |
|---|---|---|---|---|---|
| hdma/gradient_colors | HDMA direct sur CGRAM : dégradé du fond | logo actuel + dégradé | **R** — absorbe `hdma_indirect_gradient` : A bascule table directe / indirecte, même ciel | le ciel nocturne derrière une skyline en silhouette | B1 (plan lointain) |
| hdma/hdma_helpers | les 4 aides de `hdma` (luminosité, couleur, iris, ondulation) | image 13 couleurs | **G** — le catalogue des aides ; une image réelle | une ruelle : la nuit tombe, iris sur la porte, flaque qui ondule | B2 |
| hdma/hdma_indirect_gradient | HDMA indirect (`hdmaSetupIndirect`, `$43x7`) | procédural, portage krom | **F→hdma/gradient_colors** — même effet, l'autre format de table ; le README dit déjà qu'ils sont jumeaux | — | — |
| hdma/hdma_wave | format de table HDMA, ondulation à la main puis par le module | image d'eau 183 couleurs + table de krom | **G** — ligne 5 du protocole, l'entrée HDMA ; l'image d'eau devient le canal | le reflet des néons dans le canal ondule | B3 |

### color (6 → 5)

| Exemple | Capacité | Aujourd'hui | Verdict | Scène | Assets |
|---|---|---|---|---|---|
| color/direct_color | couleur directe 8bpp (CGWSEL bit 0) | cube de couleurs procédural | **G** — procédural, c'est la leçon (un bit change le sens des pixels) | — (reste abstrait) | — |
| color/hicolor_1792 | IRQ H par ligne + DMA CGRAM : 1792 couleurs | coucher de soleil procédural | **G** — ligne 7 ; source = le même panorama que mode3, converti par segments | le panorama du canal en 1792 couleurs | B3 |
| color/palette_cycle | rotation de CGRAM | tuiles procédurales | **G** — mais sur un vrai décor : une enseigne et un tuyau de liquide dont les indices sont dessinés pour tourner | une enseigne qui s'allume lettre par lettre, du liquide de refroidissement qui coule | T1 (enseignes à indices cyclables) |
| color/pseudo_hires | SETINI bit 3 : mélange 50 % sans color math | barres procédurales | **F→color/transparency** — deux façons de mêler deux calques ; l'exemple les compare (A) | — | — |
| color/shadow_tint | color math contre une couleur fixe (assombrir / teinter) | procédural | **G** — sur la rue : nuit, alerte rouge, pluie verte | la rue sous trois ambiances | B2 |
| color/transparency | addition de couleurs main/sub, BG3 en surimpression | 2 BMP PVSL | **R** — absorbe pseudo_hires : brouillard en addition, ou en pseudo-hires (A) | la brume qui glisse sur la ville | B2, B4 |

### windows (3 → 2)

| Exemple | Capacité | Aujourd'hui | Verdict | Scène | Assets |
|---|---|---|---|---|---|
| windows/transparent_window | fenêtre + color math : rectangle assombri | image PVSL | **F→windows/window** — même registre, autre cible (color math au lieu du masquage) | — | — |
| windows/window | fenêtre HDMA en triangle, masquage par calque | 2 images PVSL | **R** — absorbe transparent_window : le triangle devient le faisceau d'une lampe dans une ruelle noire (masquage) puis un cadre de dialogue assombri (color math) | Kit à la lampe torche dans la ruelle ; **fait le 2026-10-21** : la ruelle et Kit à la torche par PixelLab, le faisceau calculé (un cône), Y éclaire (addition) au lieu d'assombrir | B2, K1 |
| windows/window_multi_hdma | deux fenêtres, 4 registres par ligne | anneaux procéduraux 254 couleurs | **G** — ligne 6 ; derrière les hublots, le panorama | les hublots d'un tram aérien, la ville défile derrière ; **fait le 2026-10-21** (`build.py`, `tram_view`, 546 tuiles 8bpp en deux morceaux de banque) | B3 |

### transitions (2 → 1)

| Exemple | Capacité | Aujourd'hui | Verdict | Scène | Assets |
|---|---|---|---|---|---|
| transitions/fading | luminosité INIDISP, 3 vitesses | logo actuel | **R** — absorbe `mosaic` (qui refait déjà le fondu) : un seul exemple « transitions » | passage d'un quartier à l'autre (rue → panorama) ; **fait le 2026-10-21** avec une seule image, la lune sur la skyline (`title.png`, cadre repeint) | B2, B3 |
| transitions/mosaic | registre mosaïque + fondu | logo actuel | **F→transitions/fading** — les manifestes `transition_mosaic*` suivent | — | — |

### scrolling (3 → 3)

| Exemple | Capacité | Aujourd'hui | Verdict | Scène | Assets |
|---|---|---|---|---|---|
| scrolling/continuous_scroll | caméra à seuil, parallaxe à 2 calques | 3 PNG + perso 16x16 | **G** — la caméra sans moteur de carte | Kit court sur les toits, la skyline suit plus lentement | K1, T1, B1 |
| scrolling/mixed_scroll | un calque fixe, un calque qui défile | logo + motif | **R** — devient « l'écran-titre » : logo fixe, motif qui défile, absorbe le bandeau de `scroll_message` | l'écran-titre de Néo-Lumen ; **fait le 2026-10-21** : le logo fixe (priorité de tuile, BG1 est devant BG2 en Mode 1), la pluie qui tombe en diagonale (`build.py`, `rain`), le message | B5, B6, F1 |
| scrolling/parallax_scroll | parallaxe HDMA sur un seul calque (BG1HOFS par bande) | image PVSL 28 couleurs | **G** — ligne 3 | la skyline en trois bandes de vitesse | B1 (assemblage en bandes) |

### mode7 (5 → 4, + 2 absorbés d'ailleurs)

| Exemple | Capacité | Aujourd'hui | Verdict | Scène | Assets |
|---|---|---|---|---|---|
| mode7/rotate_scale | Mode 7 : rotation, échelle | texture 1024² 14 couleurs | **R** — absorbe `games/mode7_flying` : l'altitude pilote l'échelle et l'ombre glisse (les deux indices de profondeur), sans l'objectif d'atterrissage | le planeur de Kit survole la ville de nuit | M1, V2 |
| mode7/perspective | perspective par HDMA sur M7A/M7D seuls, ciel en haut | sol + ciel PVSL | **G (revu le 2026-10-18, pas F)** — à la lecture, il n'enseigne pas un sous-cas de perspective_rotate : il coupe l'écran par HDMA (BGMODE et TM changés à la ligne 96, un ciel en Mode 1 au-dessus d'un sol en Mode 7), ce qu'aucun autre exemple ne montre ; la fusion demandait six canaux HDMA et des tables de rotation à horizon. Garder, refaire ses assets avec le Ciel | — | — |
| mode7/perspective_rotate | matrice complète par ligne, 4 canaux HDMA | texture 1024² | **R** — absorbe perspective (le ciel au-dessus de l'horizon, D-pad déplace, L/R tourne) | les avenues de la ville jusqu'à l'horizon | M1, B1 (ciel) |
| mode7/dsp1_ground | DSP-1 Raster : sol en vraie perspective | sol + ciel PVSL | **R** — absorbe `chips/dsp1_cube` : des balises posées au sol projetées par `dsp1Project` (le cube montrait Objective/Project) ; un exemple DSP-1 au lieu de deux | une esplanade, des bornes lumineuses projetées | M1, B1, P1 |
| mode7/extbg | EXTBG : le bit 7 des pixels en priorité | plan procédural, balle | **G** — reste procédural ou dérivé de M1 | une balle d'énergie passe entre les piliers du métro aérien | M3, O6 |

### input (4 → 3)

| Exemple | Capacité | Aujourd'hui | Verdict | Scène | Assets |
|---|---|---|---|---|---|
| input/controller | `padHeld/padPressed`, constantes de boutons | texte | **R** — absorbe `timer` et `two_players` : l'écran de diagnostic que tout jeu a (deux manettes dessinées, boutons allumés, compteur de trames, deux avatars mobiles) | écran de test d'un terminal de la ville | U2, K5 (+ K6), F1 |
| input/mouse | souris : détection, deltas, boutons, sensibilité | curseur 16x16 | **G** | le bureau d'un hacker : un curseur sur un terminal ; **le curseur fait le 2026-10-21** (`build.py`, `CURSOR`) | O4, U1, F1 |
| input/superscope | Super Scope : calibrage par compteurs H/V, tir | mire + feuille de sprites PVSL (35 couleurs) | **G** | stand de tir de la sécurité : drones-cibles ; **fait en partie le 2026-10-21** : la cible de calibrage et le marqueur d'impact (`build.py`) ; pas de drones (l'exemple n'affiche que l'impact) | B7, E1, O4, O5 |
| input/two_players | deux manettes, deux sprites | carrés procéduraux | **F→input/controller** — ligne 14 suit | — | — |

### audio (9 → 5)

| Exemple | Capacité | Aujourd'hui | Verdict | Scène | Assets |
|---|---|---|---|---|---|
| audio/apu_switch | remplacer le programme APU en marche (`apuReset`) | programmes de play_noise et pitch_mod, `cello.brr` de krom (sans licence explicite, ATTRIBUTION) | **R** — absorbe `play_noise` et `pitch_mod` (il bascule déjà entre leurs deux programmes) ; le violoncelle de krom est remplacé | une borne de juke-box : A batterie de bruit, B note tenue avec vibrato | S2 |
| audio/echo | écho du DSP via le module `audio` | `pp.brr` | **F→audio/soundboard** — soundboard bascule déjà l'écho de salle | — | — |
| audio/pitch_mod | PMON : vibrato matériel | `cello.brr` (krom) | **F→audio/apu_switch** — manifestes `audio_pitch_mod_*` suivent | — | — |
| audio/play_noise | générateur de bruit, batterie sans échantillon | aucun échantillon | **F→audio/apu_switch** | — | — |
| audio/sfx_from_wav | `.wav` → `.brr` par le build, un son par bouton | blip.wav, coin.wav | **G** — ligne 12 ; la porte d'entrée des bruitages | un pupitre : saut, éclat, porte | S1 |
| audio/snesmod_music | SNESMOD : banque, lecture, pause, fondu, volume | `pollen8.it` (PVSL) | **R** — absorbe `snesmod_sfx` : musique + effets ensemble, comme dans un jeu | le thème de la ville, effets sur les boutons | MU1, MU4 |
| audio/snesmod_sfx | effets SNESMOD seuls, hauteur | `effectssfx.it` (PVSL, 76 Ko) | **F→audio/snesmod_music** | — | — |
| audio/soundboard | le moteur audio v2 piloté en C : échantillons, volume/pan/hauteur, enveloppes, écho | 4 BRR (dont le violoncelle de krom) | **R** — absorbe echo | une table de mixage | S2, S3, S1 |
| audio/speech_synth | banque de phonèmes séquencée sur une voix | phonèmes générés par `gen_phonemes.py` (à nous) | **G** — sons déjà à nous ; changer la phrase si la ville a un nom | le terminal dit « NEO LUMEN » ou « OPEN SNES » | S3 |

### maps (3 → 1)

| Exemple | Capacité | Aujourd'hui | Verdict | Scène | Assets |
|---|---|---|---|---|---|
| maps/dynamic_map | moteur de « sprites » en entrées de tilemap Mode 3, tampon FAR, 1 page par VBlank | portage PVSL + Rockford de Boulder Dash (autre propriété) | **S** — technique de niche (objets alignés sur la grille en 8bpp) ; les objets-tuiles sont enseignés par breakout (briques) et blocks, FAR par `docs/tutorials/far_ram.md` ; un personnage d'une autre propriété | — | — |
| maps/map_scroll | Tiled → `opensnes-level` → moteur de carte, caméra | `tiledMario`, `tilesMario`, `mario.png` (PVSL) | **R** — la leçon reste, tout est renommé et redessiné (nouveau niveau dans Tiled) | une rue de toits dessinée dans Tiled, Kit marche | T1, K1 |
| maps/slope_collision | `objCollidMapWithSlopes`, moteur d'objets, caméra | Mario + tuiles PVSL | **F→games/mapandobjects** — même pile (map + object + sprite_dynamic) ; la pente devient une partie du niveau | — | — |

### memory (2 → 2)

| Exemple | Capacité | Aujourd'hui | Verdict | Scène | Assets |
|---|---|---|---|---|---|
| memory/hirom_demo | construire en HiROM (`USE_HIROM`) | texte, police en ligne | **G** — ligne 15 ; aucun asset nécessaire au-delà de F1 | — | F1 |
| memory/save_game | SRAM : 2 emplacements, sauver/charger | texte | **R** — un vrai écran de sauvegarde (cases, icône, nom du quartier) : c'est le cas d'usage | la borne de sauvegarde d'un terminal | U1, K5, F1 |

### chips (8 → 2)

| Exemple | Capacité | Aujourd'hui | Verdict | Scène | Assets |
|---|---|---|---|---|---|
| chips/dsp1_cube | DSP-1 Attitude/Objective/Project | 8 points procéduraux | **F→mode7/dsp1_ground** — ligne 16 suit | — | — |
| chips/sa1_hello | diagnostic de démarrage SA-1 | texte, codes d'état | **Fix** — diagnostic, pas cas d'usage | — | — |
| chips/sa1_save | sauvegarde en BW-RAM SA-1 (compteur de démarrages) | texte | **Fix** — ROM de protocole (ligne 25) ; le guide dit que l'API `sram` est la même | — | — |
| chips/sa1_starfield | SA-1 : 128 positions par trame, I-RAM partagée | points procéduraux, 4 palettes de luminosité | **G** — la démonstration de puissance SA-1 | une nuée de papillons-néons au-dessus du canal | P1 |
| chips/superfx_3d | GSU : cube fil de fer, Bresenham, letterbox | procédural | **G (révisé le 2026-10-18, confirmé par le propriétaire le 2026-10-19 ; était F→superfx_game_skeleton)** — ce n'est pas un doublon : c'est le chemin simple (job lancé depuis la ROM par `gsuLaunch`, le CPU attend ; un seul tampon, `gsuDmaFullFrame` ; IRQ V-timer armée pendant les jobs) que le squelette écarte exprès (cache, double tampon, `gsuPresent`). Trois tutoriels l'enseignent comme marche (`superfx.md`, `superfx_advanced.md`, `interrupts.md`), il est le seul appelant de `gsuDmaFullFrame` et la référence de `nmi_budget.py`. Fusionner = un squelette à deux modes de présentation (gros travail GSU) ou une fixture + trois tutoriels réécrits sans la marche simple. Même raisonnement que mode7/perspective. | — | — |
| chips/superfx_game_skeleton | jeu à 60 i/s, GSU à 30, double tampon, SNESMOD | procédural + module de snesmod_music | **R** — reste la référence Super FX ; l'objet 3D devient un hologramme (logo ou cube de données) | un hologramme tourne au-dessus d'une place, un viseur suit le pad | O4, MU1 |
| chips/superfx_hello | diagnostic GSU (SRAM, FMULT) | texte | **Fix** | — | — |
| chips/superfx_save | sauvegarde en Game Pak RAM (ligne 26) | texte | **Fix** | — | — |

### games (8 → 7)

| Exemple | Capacité | Aujourd'hui | Verdict | Scène | Assets |
|---|---|---|---|---|---|
| games/breakout | jeu complet : briques en tuiles BG modifiées en RAM, ombres, niveaux | tuiles, cartes, palettes PVSL | **G** — leçon solide ; tout redessiner | « Neon Breaker », la borne d'arcade du bar | T5, O6, O7, F1 |
| games/likemario | plateformer : physique en virgule fixe, streaming de colonnes, `vramqueue`, `anim`, SNESMOD | sprite et tuiles d'une autre propriété, `overworld.it` PVSL | **R + renommer** (p. ex. `games/rooftop_run`) — nouveau niveau et nouveaux assets (pilote de la note de chantier) ; garde son moteur à lui (seul utilisateur de `vramqueue`) | Kit livre un colis par les toits | K1, T1, B1, E3, O1, MU2, MU4 |
| games/mapandobjects | moteur de carte + moteur d'objets, ennemis, projet multi-fichiers | Mario, Goomba, Koopa (PVSL) | **R + renommer** (p. ex. `games/platform_objects`) — absorbe slope_collision : un niveau avec pentes et deux ennemis | patrouille sur les passerelles : automates et insectes-robots | K1, T1, E2, E3, E1 |
| games/mode7_flying | altitude → échelle Mode 7, ombre, atterrissage | terrain généré (à nous) | **F→mode7/rotate_scale** — les deux indices de profondeur sont la leçon ; le reste double mode7_racing comme « jeu Mode 7 » | — | — |
| games/mode7_racing | caméra de course Mode 7, carte de classes | circuit généré par `gen_track.py` (à nous) | **G** — le jeu Mode 7 | course d'aéro-motos sur un circuit aérien | M2, V1, F1 |
| games/rpg | Tiled (terrain, collision, entités, dialogues), panneaux, HUD | tuiles, héros, PNJ (à nous mais fonctionnels) | **G** — le gabarit vue de dessus | la Rue basse : parler aux habitants, ouvrir un casier | K5, N1, T2, T3, O2, U1, F1 |
| games/shmup_1942 | défilement vertical, pools de sprites, collisions tirs/ennemis | pack CC0 de Kenney | **R + renommer** (« 1942 » est un titre Capcom ; p. ex. `games/shmup`) | le planeur de Kit au-dessus de la ville contre des drones | T4, V2, E4, O3, O5 |
| games/blocks | puzzle complet, DAS, HDMA, SNESMOD | tuiles générées (à nous), Korobeiniki | **R + renommer** (« Tetris » est une marque) — p. ex. `games/blocks` | empiler des conteneurs de données dans un puits | T5, MU3, F1 |

### Remarques transverses relevées pendant la revue

- Plusieurs README citent encore Mesen2 (`mode1`, `mode1_lz77`, `mode1_bg3_priority`, `timer`, `collision_demo`
  « Mesen2 recommended ») ; `collision_demo` enseigne un contournement chiffré (« 158-byte stack overhead »),
  motif voisin de ceux que l'ancre 14 refuse. À corriger avec la refonte, pas avant.
- `audio/apu_switch`, `pitch_mod`, `soundboard` embarquent le violoncelle de krom, dont le dépôt n'a pas de
  licence explicite (`ATTRIBUTION.md`) : à remplacer en priorité avec les fichiers PVSL.
- Chaque fusion déplace des manifestes (`testing/manifests/`) et des lignes du protocole matériel (lignes 10,
  14, 16, 18) ; les fixtures (`mode6`, `sa1_*`, `superfx_hello`, `superfx_save`) doivent continuer d'être
  construites par `make hardware-preflight`.

---

## 3. Catalogue des assets partagés

Conventions : tuiles 8x8, métatuiles 16x16 pour les décors, 4bpp sauf indication, couleur 0 transparente.
« Pal » renvoie à la carte des palettes (§4). PixelLab : *character* (perso, 4/8 directions), *animate*
(animation texte ou squelette), *tileset* (side-scroller ou top-down), *map object*, *pixflux/bitforge*
(image libre, palette forcée), *image-to-pixelart*, *inpaint*.

| Id | Famille | Description dans le monde | Spécification | Exemples | Route |
|---|---|---|---|---|---|
| K1 | personnage | Kit, renardeau coursier, profil (side-scroller) : veste à capuche, sacoche, queue | 16x16 (à décider : 16x32 plus lisible), idle 2, marche 4, saut 1, chute 1, touché 1 ; 1 direction + flip ; Pal OBJ0 | likemario, mapandobjects, map_scroll, continuous_scroll, sprite_sizes, window | PixelLab *character* (vue de côté) + *animate* ; ancre n° 1 |
| K2 | personnage | Kit en 32x32, même pose | 32x32, idle 1, marche 8 (pour le streaming) ; Pal OBJ0 | simple_sprite, sprite_sizes, dynamic_sprite | PixelLab, dérivé de K1 en référence de style |
| K3 | personnage | Kit grand format, métasprite | 32x48, tags « walk » (6) et « wave » (4, ping-pong) ; Pal OBJ0 ; source `.aseprite` + JSON | aseprite_pipeline, dynamic_sprite | PixelLab *animate*, puis assemblage des tags **à la main dans Aseprite** |
| K4 | personnage | Kit en icône minuscule et en géant | 8x8 et 64x64, une pose ; Pal OBJ0 | sprite_sizes, dynamic_sprite | 64x64 PixelLab ; **8x8 à la main** (trop petit pour le générateur) |
| K5 | personnage | Kit vue de dessus | 16x16, 4 directions (gauche = flip de droite), marche 3, idle 1 ; Pal OBJ0 | animated_sprite, rpg, collision_demo, game_skeleton, controller, save_game | PixelLab *character* 4 directions + *animate* ; ancre n° 2 |
| K6 | personnage | le 2e joueur | soit K5 en Pal OBJ1 (échange de palette, coût nul), soit un second animal (raton laveur mécano) aux mêmes spécifications | controller (2 joueurs) | échange de palette, ou PixelLab comme K5 (question §6) |
| E1 | ennemi | drone de sécurité flottant, œil unique, lisible de côté comme de dessus | 16x16 vol 4 trames + 32x32 même dessin ; Pal OBJ3 | aim_target, collision_demo, world_sprites, superscope, mapandobjects | PixelLab *character* 1 direction + *animate* ; ancre n° 4 (référence de style = K1) |
| E2 | ennemi | automate patrouilleur (marche) | 16x32, marche 4, demi-tour 1 ; Pal OBJ3 | mapandobjects | PixelLab *character* + *animate* |
| E3 | ennemi | insecte-robot rampant | 16x16, marche 2, écrasé 1 ; Pal OBJ3 ; **fait** : 32x32, marche 4 (`characters/walk/insect_*`), l'écrasé tiré par script ; likemario le 2026-10-20, avec le drone d'E1 | mapandobjects, likemario | PixelLab *character* + *animate* |
| E4 | ennemi | chasseurs de la sécurité vus de dessus | 32x32, 2 modèles x 2 trames ; Pal OBJ3 | shmup | PixelLab *map object* / *pixflux* vue de dessus |
| N1 | PNJ | 4 habitants : hibou barman, rat brocanteur, grenouille mécano, pangolin vigile (à décider) | 16x16, 4 directions, idle 2 ; une Pal OBJ2 commune aux quatre | rpg | PixelLab *character* 4 directions, référence de style K5 |
| V1 | véhicule | aéro-moto de course, vue de dessus (la caméra de mode7_racing est vue de dessus) | 32x32 + ombre ; **fait le 2026-10-20** : la moto de Kit et deux rivales, une palette OBJ pour les trois, les rivales tournées en 5 trames par RotSprite (`devtools/art_assets/racing.py`) | mode7_racing | PixelLab `generate-image-v2`, seed 120 |
| V2 | véhicule | planeur de livraison de Kit, vue de dessus | 32x32, 3 trames de roulis + ombre 32x32 ; Pal OBJ4 | shmup, rotate_scale | PixelLab *pixflux* / *rotate* |
| O1 | objet | éclat de données (la « pièce » du monde) | 16x16, rotation 8 trames ; variante 24 trames pour le streaming ; Pal OBJ5 ; **fait le 2026-10-20** pour likemario : une image PixelLab (`props/shard.png`), les 8 trames de rotation par script | game_skeleton, world_sprites, dynamic_sprite, likemario | PixelLab *animate* ou **à la main** (petit, cyclique) |
| O2 | objet | objets de carte vue de dessus : casier (fermé/ouvert), caisse, borne-terminal, porte | 16x16 chacun ; tuiles BG (Pal BG T2) | rpg, collision_demo | PixelLab *map object* |
| O3 | objet | projectiles : tir de Kit, tir ennemi | 8x8 et 16x16, 2 trames ; Pal OBJ5 | shmup, aim_target | **à la main** ou PixelLab |
| O4 | UI/objet | curseur de souris, viseur Super Scope, viseur de pad | 16x16, 3 dessins ; Pal OBJ7 | mouse, superscope, superfx_game_skeleton, aim_target | **à la main** (quelques pixels) |
| O5 | effet | explosion / impact | 16x16 et 32x32, 5 trames ; Pal OBJ6 | shmup, superscope | PixelLab *animate* |
| O6 | objet | orbe d'énergie (balle) | 8x8 et 16x16 ; Pal OBJ5 ; **fait le 2026-10-21** pour extbg : 16x16, 8 trames de pulsation, dessiné par script (`mode7.py`, `orb`) ; le 8x8 de breakout le 2026-10-21 (la balle, dessinée par `breakout.py`) | breakout, extbg | **à la main** |
| O7 | objet | raquette du casse-briques | 32x8 + ombre ; Pal OBJ5 ; **fait le 2026-10-21** (`breakout.py`, dans la palette OBJ 0 que le jeu utilise) | breakout | **à la main** |
| P1 | particules | goutte de pluie, étincelle, papillon-néon, point lumineux | 8x8, 2-4 trames ; Pal OBJ6 déclinée en 4 luminosités | sprite_swarm, sa1_starfield, aim_target, dsp1_ground | **à la main** / procédural |
| T1 | tileset | les Toits, profil : dalles, passerelles traversables, tuyaux, câbles, antennes, enseignes, pentes 45° et 22,5° | métatuiles 16x16, ~100 tuiles 8x8, 2 Pal BG ; enseignes dessinées avec une rampe néon à indices contigus (pour palette_cycle) ; propriétés de tuiles (solide, pente) dans Tiled | likemario, mapandobjects, map_scroll, continuous_scroll, palette_cycle | PixelLab *tileset* side-scroller ; ancre n° 3 ; pentes probablement retouchées à la main |
| T2 | tileset | la Rue basse vue de dessus : chaussée, trottoirs, façades, flaques, vitrines, néons au sol | métatuiles 16x16, transitions (Wang), ~120 tuiles, 2 Pal BG | rpg (ville), world_sprites, animated_sprite, collision_demo, game_skeleton | PixelLab *tileset* top-down |
| T3 | tileset | intérieur vue de dessus : bar, atelier | métatuiles 16x16, ~60 tuiles, 1 Pal BG | rpg (maison) | PixelLab *tileset* top-down, référence T2 |
| T4 | tileset | la ville vue d'avion : toits, canaux, avenues | métatuiles 16x16, scène 256x512 en 4bpp (2 Pal BG) ; la même source réduite à ≤256 tuiles 8bpp donne M1 | shmup, (M1) | PixelLab *tileset* top-down ou *pixflux* + découpe |
| T5 | tileset | l'Arcade : briques (6 teintes), blocs de puzzle (7 teintes), cadre de borne, puits | tuiles 8x8, 2 Pal BG ; **la moitié breakout faite le 2026-10-21** : la borne, une brique en 8 teintes (une palette BG chacune), le plateau de BG3 (`breakout.py`) ; reste blocks | breakout, blocks | **à la main** ou script (comme `gen_tiles.py` aujourd'hui) ; PixelLab pour le cadre |
| B1 | fond/parallaxe | la skyline : plan lointain (tours, ciel, lune voilée), plan moyen, plan proche | 3 calques 512x256 4bpp (1 Pal chacun) ; déclinaisons : 4 plans 2bpp pour mode0, une image en 3 bandes pour parallax_scroll, le ciel pour Mode 7 | continuous_scroll, parallax_scroll, mode0, gradient_colors, perspective_rotate, dsp1_ground, likemario | PixelLab *pixflux* par tranches de 256 px + raccords à la main (ou *image-to-pixelart* d'une peinture) ; taille maximale d'une génération à vérifier |
| B2 | fond | « la rue sous la pluie », l'illustration-vitrine | 256x224 4bpp, ≤3 Pal BG ; doit contenir une zone sombre (lampe), une enseigne, une flaque, de la vapeur | mode1, mode2, hdma_helpers, shadow_tint, transparency, window, fading, panel_hud, sprite_swarm | PixelLab *pixflux* / *image-to-pixelart*, retouches |
| B3 | fond | panorama du canal la nuit, riche en couleurs | 256x224 en 8bpp (256 couleurs) ; la même source convertie en segments 64x8 pour hicolor | mode3, hicolor_1792, hdma_wave, window_multi_hdma, fading | PixelLab *image-to-pixelart* d'une image peinte, ou *pixflux* sans palette forcée |
| B4 | fond | brume et pluie en couche | 256x256 2bpp, raccordable, 1 Pal 4 couleurs | transparency | **à la main** / procédural |
| B5 | logo/fond | l'écran-titre : logo pixel + Kit | 256x224 4bpp, 2 Pal BG | mixed_scroll, game_skeleton, scene_stack | dérivé du logo L1, retouche à la main |
| B6 | fond | motif d'arrière-plan qui défile (grille holographique, pluie de code) | tuile raccordable 32x32, 2bpp ou 4bpp | mixed_scroll | **à la main** / procédural |
| B7 | UI/fond | mire de calibrage du Super Scope | 256x224, 4 couleurs | superscope | **à la main** |
| M1 | texture Mode 7 | la ville vue d'en haut (toits, avenues, esplanade) | 1024x1024, ≤256 tuiles 8x8 8bpp, 1 palette 256 | rotate_scale, perspective_rotate, dsp1_ground | dérivé de T4 (requantifié, dédupliqué) ; contrôle de la limite de 256 tuiles **fait le 2026-10-19** : deux jeux Wang PixelLab + l'esplanade et des héliports découpés dans la vue aérienne, 235 tuiles, 16 couleurs (`devtools/art_assets/mode7.py`) ; la réduction de la vue aérienne seule ne tient pas (3448 tuiles) |
| M2 | texture Mode 7 | circuit aérien de course + carte de classes (piste, bord, vide) | 1024x1024, ≤256 tuiles ; **fait le 2026-10-20** : 227 tuiles, 19 couleurs, sa propre palette (le circuit n'a pas de ciel), deux jeux Wang (asphalte/gravier, gravier/lagune), six objets de toit, deux plots d'accélération | mode7_racing | `devtools/art_assets/racing.py` (remplace `gen_track.py`), tuiles PixelLab |
| M3 | texture Mode 7 | dalle + piliers (bit 7 = devant) | ≤128 couleurs par moitié de priorité ; **fait le 2026-10-21** : une fenêtre de M1 et trois viaducs du métro aérien avec un train (bit 7, couleurs 1-13, répétées en 129-141), 93 tuiles (`mode7.py`, `extbg_plane`) | extbg | procédural ou dérivé de M1 |
| U1 | UI | kit de panneaux 9-slice (style HUD terminal, style bulle) + icônes 8x8 (batterie/cœur, éclat, clé, disquette) | tuiles 8x8, version 4bpp et 2bpp, 1 Pal BG UI | panel_hud, rpg, save_game, scene_stack, game_skeleton, pixel_text, mouse | PixelLab *UI elements* puis découpe à la main |
| U2 | UI | schéma de manette générique (pas celle de Nintendo), boutons allumables | tuiles 8x8 4bpp, 1 Pal BG | controller | **à la main** |
| F1 | police | police système 8x8 (module `text`, intégrée à la lib) | 96 glyphes, 2bpp (et sa conversion 4bpp `text4bpp`) | tous les exemples à texte | **à la main** (une police générée est rarement régulière) |
| F2 | police | police étroite 5x7 proportionnelle | glyphes en bits dans le C | pixel_text, (rpg) | **à la main** |
| F3 | police | police hi-res (8 px à 512) | rendue par `gen_assets.py` | mode5_hires | procédural, restylé sur F1 |
| L1 | logo | le logo OpenSNES dans la nouvelle identité | SVG + PNG 16/32/64/128/256 + favicon ; base de B5 et du site | site, README, `assets/`, B5 | dessin vectoriel (designer ou à la main) d'après K1 ; PixelLab seulement pour les petites tailles pixel |
| MU1 | musique | thème de la ville (nuit, nappes, basse) | `.it` ≤ 8 voix, ≤ 58 Ko de banque (`opensnes-music inspect`) | snesmod_music, superfx_game_skeleton | **commande** à un musicien chiptune |
| MU2 | musique | thème des Toits (plus rythmé) | idem | likemario | commande (ou variation de MU1) |
| MU3 | musique | thème de l'Arcade | idem ; remplace Korobeiniki | blocks | commande |
| MU4 | bruitages | banque d'effets SNESMOD (saut, éclat, touché, tir, explosion, menu) | `.it` d'échantillons courts | snesmod_music, likemario | sons de S1 montés dans OpenMPT **à la main** |
| S1 | bruitages | blip, éclat, saut, tir, explosion, touché, porte | WAV mono < 1 s → BRR par `opensnes-sample` | sfx_from_wav, soundboard, (MU4) | **sfxr / jsfxr / ChipTone** |
| S2 | échantillons | note tenue en boucle (remplace le violoncelle de krom), une basse ou nappe | BRR bouclé, 32 kHz ou moins | apu_switch, soundboard | synthèse par script (procédural) ou commande |
| S3 | échantillons | phonèmes (AU, EA, NN, PP, SS) | BRR, déjà générés par `gen_phonemes.py` | speech_synth, soundboard | existant, à nous |

---

## 4. Palette

### Combien

- **OBJ : 8 palettes, une carte fixe pour tous les exemples.** Elle tient exactement dans les 8 emplacements
  matériels, si bien qu'un sprite a la même palette dans chaque exemple et qu'aucun exemple ne doit
  la renégocier :

  | Slot OBJ | Contenu |
  |---|---|
  | 0 | Kit (K1-K5) |
  | 1 | 2e joueur (K6, ou K5 recoloré) |
  | 2 | habitants (N1) |
  | 3 | machines de la sécurité (E1-E4) |
  | 4 | véhicules (V1, V2, ombres) |
  | 5 | objets : éclats, projectiles, balle, raquette (O1, O3, O6, O7) |
  | 6 | effets et particules (O5, P1) ; sa1_starfield en tire ses 4 luminosités |
  | 7 | curseurs, viseurs, icônes en sprite (O4) |

- **BG 4bpp : environ 20 palettes distinctes**, jamais plus de 8 dans une scène : UI/texte (1, plus les
  palettes 4 couleurs du BG3 2bpp), T1 (2), T2 (2), T3 (1), T4 (2), T5 (2), B1 (3), B2 (3), B5 (2),
  B4/B6/B7 (2bpp). Mode 0 tire ses 4x8 palettes de 4 couleurs des mêmes rampes que B1.
- **8bpp : 2 palettes 256** : une pour Mode 7 (M1, M2, et M3 sur sa moitié basse de 128), une pour B3
  (mode3, et la source de hicolor qui se requantifie par segment).

### Comment les partager

- Chaque palette de 16 garde **les mêmes indices pour la même fonction** : 0 transparent, 1 encre (contour
  sélectif), 2-3 ombres de nuit, puis trois rampes de 4. Une teinte (`colorMathTint`), une ombre ou un
  fondu traite alors tous les assets de la même façon, et un échange de palette (K6, alerte rouge) ne
  touche que les rampes.
- Les rampes néon ont des **indices contigus** dans les palettes de décor, pour que palette_cycle les
  fasse tourner sans redessiner.
- Les palettes BG d'une scène se choisissent par `opensnes-palette plan` dans le catalogue ; un exemple ne
  crée pas de palette à lui sauf B3 et les textures Mode 7.

### Structure de la palette maîtresse (sans couleurs exactes)

Environ **56 à 64 couleurs en 13 rampes** de 3 à 5 pas, chaque canal multiple de 8 (15 bits) :

| Rampe | Pas | Rôle |
|---|---|---|
| Encre → nuit | 5 | le fond de tout : contours, ombres profondes, ciel haut |
| Sarcelle profonde | 4 | eau, verre, métal mouillé |
| Violet (la marque) | 5 | architecture, ombres colorées, logo |
| Acier froid / gris bleuté | 4 | machines, béton |
| Laiton / cuivre | 5 | tuyaux, rivets, mécanismes (si le côté vapeur reste) |
| Rouille / brique | 3 | vieux quartiers, briques |
| Pelage A (roux → crème) | 5 | Kit |
| Pelage B (gris-brun → beige) | 4 | habitants |
| Néon magenta | 3 | du pas sombre (halo) au cœur presque blanc ; saturation plafonnée |
| Néon vert phosphore / cyan | 3 | terminaux, UI, yeux de Kit |
| Néon ambre | 3 | lampes, alertes douces |
| Brume | 3 | lumières diffuses, pluie, reflets désaturés |
| Blanc cassé | 1 | reflets spéculaires seulement |

Règles de nuance : un néon ne s'utilise jamais seul en aplat, toujours avec son pas sombre autour (le halo
en couches) ; les décors prennent les pas désaturés et plus sombres, les sprites les plus contrastés pour
se lire d'un coup d'œil ; lumière en haut à gauche, contour sélectif en encre (pas en noir pur).

---

## 5. Ordre de production

Nombre de générations : ordre de grandeur, essais refusés compris (compter 3 essais par pièce retenue en
moyenne, davantage pour les ancres).

| Vague | Contenu | Exemples débloqués | Générations PixelLab |
|---|---|---|---|
| 0 — fondations | palette maîtresse (à la main), bible du monde, puis les **5 ancres** : K1 (Kit profil), K5 (Kit dessus), T1 (Toits), E1 (drone), T2 (Rue basse) ; ce sont les 5 tests d'essai de la note de chantier | — (validation du style dans un ROM sur luna) | 80 à 120 |
| 1 — sprites | animations de K1 et K5, K2, K3 (+ Aseprite), K4, O1, P1, O4, O3 | simple_sprite, sprite_sizes, animated_sprite, aseprite_pipeline, dynamic_sprite, sprite_swarm, world_sprites, aim_target, collision_demo, game_skeleton, mouse | 80 à 120 |
| 2 — Toits (plateformer) | T1 complet (pentes, enseignes), E2, E3, B1 et ses déclinaisons | likemario (pilote), map_scroll, mapandobjects, continuous_scroll, parallax_scroll, mode0, palette_cycle | 120 à 160 |
| 3 — Rue basse (vue de dessus) | T2 complet, T3, N1, O2, U1 | rpg, panel_hud, save_game, scene_stack, controller (+K6) | 120 à 160 |
| 4 — illustrations et effets | B2, B3, B4, B5 (après L1), B6 | mode1, mode2, mode3, hicolor, hdma_*, color/*, windows/*, fading, mixed_scroll | 50 à 80 |
| 5 — ciel et Mode 7 | T4, M1, M2, M3, V1, V2, E4, O5 | shmup, rotate_scale, perspective_rotate, dsp1_ground, extbg, mode7_racing, superscope, superfx_game_skeleton | 100 à 140 |
| 6 — arcade | T5, O6, O7 (surtout à la main) | breakout, blocks | 10 à 30 |
| 7 — son | S1 (sfxr), S2, MU4 ; commande de MU1-MU3 | sfx_from_wav, soundboard, apu_switch, snesmod_music, likemario, blocks | 0 (hors PixelLab) |
| 8 — identité | L1 vectoriel, tailles pixel, favicon, site | site, README | 10 à 20 (petites tailles) |
| **Total** | | | **≈ 600 à 800** |

À chaque exemple refait : conversion par les fichiers de réglage, budget (`opensnes-sprite inspect`,
`opensnes-palette plan`), captures luna aux trames des manifestes côte à côte avec l'ancien, puis
re-capture des références (fbhash, WRAM, audio) avec leur justification, et manifestes `movement_*`
adaptés à la nouvelle géométrie des niveaux.

---

## 6. Questions pour le propriétaire

1. **La distribution** : Kit en renardeau coursier vous convient-il (le nom reste) ? Quels animaux pour les
   habitants ? Les adversaires sont-ils des machines (proposé : lisibles, sans ambiguïté morale) ou aussi
   des animaux (un gang de rats, des corbeaux de la sécurité) ?
2. **[Tranché le 2026-10-18 : Néo-Lumen, sans laiton.]** **Le côté vapeur/laiton** de la note de chantier reste-t-il dans le cyberpunk, ou part-on sur un
   cyberpunk pur (la rampe laiton disparaît alors au profit d'une rampe acier) ?
3. **Une seule ville en quartiers** (proposé : Toits, Rue basse, Canal, Ciel, Arcade) ou plusieurs lieux ?
   Le nom de la ville (« Néo-Lumen » est un exemple) servira aussi à speech_synth et aux écrans-titres.
4. **Garder à la fois un plateformer et un RPG vue de dessus ?** Recommandation : oui, ce sont les deux
   formes de jeu les plus courantes, mais cela coûte deux jeux de Kit (K1 et K5) et deux tilesets.
5. **Kit en 16x16 ou en 16x32** dans le plateformer : un animal lisible demande plutôt 16x32 ; `likemario`
   lit aujourd'hui des trames 16x16.
6. **[Tranché le 2026-10-18 : oui, moins d'exemples.]** **Les fusions et le passage en fixtures** : acceptez-vous de passer de 85 à 58 exemples, de déplacer
   `mode6`, `sa1_hello`, `sa1_save`, `superfx_hello`, `superfx_save` hors de `examples/` (toujours construits
   pour le protocole matériel) et de supprimer `maps/dynamic_map` ?
7. **[Tranché le 2026-10-18 : `tetris` devient `blocks` ; les autres attendent.]** **Les renommages** : `likemario`, `mapandobjects`, `shmup_1942`, `tetris` (marque déposée) — vos noms ?
8. **La police intégrée de la lib** (F1) : la redessiner dans l'identité ? Elle change l'image de chaque
   ROM à texte et toutes les références fbhash.
9. **La musique** : commander 3 modules et une banque d'effets à un musicien chiptune (budget ?), ou des
   modules libres vérifiés un par un ? Korobeiniki (domaine public mais lié à Tetris) part-il ?
10. **[Tranché le 2026-10-18 : CC0.]** **La licence** des nouveaux assets : CC0 (honnête pour une sortie d'IA) ou CC-BY 4.0 (pour un kit
    commandé et retouché) ; la clause de PixelLab sur l'entraînement est à confirmer avant CC0.
11. **Le logo** : un designer pour la version vectorielle, ou le dessin de Kit retouché à la main ?
