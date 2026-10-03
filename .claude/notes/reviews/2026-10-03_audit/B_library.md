# Audit — bibliothèque et runtime (2026-10-03, campagne de chasse avant la 1.0)

Rapport de l'agent `audit-library`, non retouché (copié depuis sa sortie : l'agent n'a pas écrit le fichier lui-même). Les textes successifs de l'agent sont séparés par une règle.

Je n'ai pas créé `.claude/notes/reviews/2026-10-03_audit/B_library.md` : mes consignes d'agent m'interdisent d'écrire des fichiers de rapport. Le rapport complet est ci-dessous, prêt à y être copié tel quel.

# Audit B — Bibliothèque et runtime (2026-10-03, `develop` @ `73fbdddb`)

## Défauts silencieux candidats

Chaque fonction publique des 36 en-têtes a été relue contre son corps, en quatre passes parallèles. J'ai revérifié moi-même dans les sources les quatre plus graves (lignes 1, 3, 8 et 10).

### Code faux (le corps ne tient pas la promesse)

| # | fonction | promesse (en-tête) | code (corps) | preuve | sév. |
|---|---|---|---|---|---|
| 1 | `mapVblank` / `mapUpdate` / `mapRefreshAll` | `CLAUDE.md` et le tableau KL : la lib écrit `y - 1` dans BGnVOFS | `dispyofs_L1` porte déjà le −1 hérité de PVSnesLib (`map.asm:1133-1136` `clc / sbc mapdisplaydeltay`, `map.asm:601` `dec a`). `mapVblank` en retire un second (`map.asm:870`, `:886`), ajouté par `990b32d3`. Le registre reçoit `y - 2` | luna : map_scroll, slope_collision et tiled donnent `v_scroll` = 1022 pour BG1 contre 1023 pour les autres BG ; sur mapandobjects, `map_cam_y=0x10` donne VOFS=14. Les baselines ont figé le décalage | 🟠 (carte 1 px trop bas par rapport aux sprites et aux autres BG) |
| 2 | `hdmaIrisWipe`, `hdmaBrightnessGradient`, `hdmaColorGradient` réappelées en cours d'image | `hdma.h:586` : « Call again with a different radius to animate » (double tampon) | `hdmaSetup()` réécrit A2A et NTRL=1 en pleine image (`hdma.c:445-458`, `hdma.asm:118,148`). La table repart de la ligne 0 au HBlank suivant : le bas de l'écran reçoit le haut de la table pendant une image | luna, `hdma_helpers.sfc` : seule l'image f150 diffère (`f8b806e2`), avec la moitié basse du masque cassée. Luminosité : image f83 (`a7565a67`) | 🟠 |
| 3 | `oamHide`, `oamClear` | `sprite.h:371-393` : « Park a sprite off screen » ; `sprite.c:206` : « safe for any sprite size » | X=256 exactement (bas=0, bit haut=1) et Y=240 (`sprite.c:140-146`, `:208-222`) | corpus anomie-regs, chunk `2304edd2bf6755b9` : un OBJ à X=256 compte comme X=0 pour les limites Range/Time. Un sprite de 32 ou 64 px à Y=240 revient sur les lignes 0..47 et consomme les 32 sprites / 34 slivers par ligne. En `OBJ_SIZE32_L64`, `oamClear` produit 128 sprites de 32 px qui privent de slivers les vrais sprites des lignes 0-15. Le moteur dynamique utilise X=257 (`sprite_dynamic.asm:427`) | 🟠 (non mesuré sur luna) |
| 4 | `oamDynamicInit` | `OamDynamicConfig.vramLarge/vramSmall` (`sprite.h:584-590`) : « VRAM base » au choix | OBJSEL est écrit avec name base 0 et écart 0 (`sprite_dynamic.asm:292-299`) ; les tables de numéros de tuile sont figées (`sprite_lut.asm:45-134`). Seule la destination de la DMA suit la config | lecture du code : toute valeur autre que 0x0000/0x1000 envoie les tuiles d'un côté et fait pointer l'OAM de l'autre. Les 5 exemples utilisent 0x0000/0x1000, d'où un défaut latent | 🟠 |
| 5 | `oamMetaDrawDyn(..., OBJ_SMALL)` en modes `OBJ_SIZE16_L64` / `OBJ_SIZE32_L64`, `oamDynamicSetSize` | `sprite.h:684-687` : « OBJ_SMALL → small half of the size pair » | le test de `sprite_dynamic.asm:280` ne vise que le mode 3 ; `oamDynamic32Draw` force « large » (`:893`) ; aucune vérification de la taille contre la paire (`sprite_dynamic_dispatch.c:88-91`) | lecture du code : la pièce s'affiche en 64 px | 🟠 |
| 6 | file d'envoi VRAM du moteur dynamique | `oamDynamicDraw` ne mentionne aucune limite (`sprite.h:629-644`) | `oamQueueEntry` fait 768 octets (`sprite_dynamic.asm:108`), l'index avance de 6 sans borne (`:780`, `:962`, `:1179`), la NMI ne vide que 42 octets (7 entrées) par trame (`:508-513`) | lecture du code : plus de 7 rafraîchissements par trame pendant plusieurs trames écrasent `.dynamic_sprite_state`, y compris l'index lui-même | 🟠 |
| 7 | `gsuSetupHdmaBlanking(top, bottom)` | `superfx.h:405`, `:416-428` : des bandes de blank pour toute paire avec top+bottom ≥ 73 | `top=0` donne une première entrée de table nulle, qui arrête le canal (corpus snesdev-wiki `4204113bc8ef0f4b`), alors que `gsu_pres_bottom` est publié quand même. `gsuDmaFullFrame` DMA alors sur des lignes visibles (`superfx.asm:258-270`). `top>128` passe en mode répétition ; `top+bottom>224` déborde. Les lignes visibles réécrivent INIDISP=`$0F` à chaque trame (`:381`, `:387`, `:402`), ce qui annule `setBrightness`, les fondus et `setScreenOff()` | code + corpus | 🟠 |
| 8 | `fixLerp` | `math.h:284-300` : interpole entre deux `fixed` quelconques | `b - a` est calculé sur 16 bits, puis son bit 15 sert de signe (`math.asm:206-218`) | arithmétique : `fixLerp(FIX(-64), FIX(64), 128)` rend −128.0 au lieu de 0 ; même défaut dès que l'écart atteint 128.0 | 🟠 |
| 9 | `hdmaWaveH`, `hdmaWaterRipple`, `hdmaWaveStop` | `hdma.h:463`, `:480`, `:628` : « wavy distortion » du fond | la table écrit un HOFS **absolu** (`hdma.c:183-187`), qui remplace le scroll de `bgSetScroll`. `hdmaWaveStop` écrit HOFS=0 sans marquer `bg_scroll_dirty` (`:286-287`) | lecture du code (non mesuré) | 🟠 probable |
| 10 | `consoleInit` (graine de `rngNext`/`rand`) | `console.h` : « seeds rngNext() from the H/V counters » | `console.c:77` lit OPHCT/OPVCT sans latch préalable (ni `$2137`, ni front sur WRIO ; crt0 écrit `$FF` dans `$4201`, `crt0.asm:428-429`). La partie H/V vaut 0, la graine vaut STAT78 | corpus fullsnes `5f1c3420ba3a986c` et snesdev-wiki `7a230e740aeb9460`. luna, `random.sfc` : `rand_seed=$8001` identique sans `--power-on`, avec `random=1` et avec `random=7`. Même suite à chaque démarrage | 🟠 |
| 11 | `objKill(h)` appelé depuis un callback d'update | `object.h:274-279`, aucune restriction | `objUpdateAll` empile le `next` de l'objet courant (`object.asm:943-944`). Si on tue ce suivant, la boucle continue sur la liste libre (`:767-783`) et appelle l'update de type 0 sur des emplacements morts. `objKill` retire aussi son propriétaire au workspace (`:790-791`) | raisonnement sur le code (non exécuté) | 🟠 probable |
| 12 | `mode7SetAngle`, `mode7Rotate`, `mode7Transform` | `mode7.h:102-162`, aucune contrainte de moment | multiplications via M7A/M7B puis `$2135` (`mode7.c:113-118`) | corpus snesdev-wiki `91d05f02383ae664` : le multiplicateur PPU n'est valable qu'en VBlank ou en modes 0-6, et le latch M7A est partagé avec BG1HOFS. Un appel en pleine image Mode 7 donne une matrice fausse | 🟠 (docs à compléter ; les exemples l'appellent juste après `WaitForVBlank`) |
| 13 | `setMode` | `video.h:53` : mode « optionally OR'd with priority flags » | `console.c:249` : `mode & 0x07`, le bit 0x08 est jeté | code (aucun appelant du dépôt n'utilise la forme OR) | 🟡 |
| 14 | `colorMathInit` | `colormath.h:147` : réinitialise tout | `REG_COLDATA = 0` : aucun plan de couleur n'est sélectionné, la couleur fixe n'est pas remise à zéro (il faudrait `0xE0`) | corpus anomie `8095d317ffcf5375` | 🟡 |
| 15 | `windowCentered(1)`, `windowCentered(256)`, `windowSplit(0)` | `window.h:240` : largeur 1-256 ; `window.h:252` | 1 donne une fenêtre vide (128..127), 256 devient 0 en u8, une largeur impaire perd 1 px ; `split=0` couvre x=0 | code | 🟡 |
| 16 | `hdmaIrisWipe`, `hdmaIrisWipeStop` | « configure les registres de fenêtre » | écrit W12SEL, W34SEL, WOBJSEL et TMW sans passer par les shadows de `window.c` (`hdma.c:466-482`) ; `Stop` efface la config fenêtre de l'utilisateur | code | 🟡 |
| 17 | `audioInit` rappelée | `audio.h:179-197` : `AUDIO_ERR_TIMEOUT` si le driver ne répond pas | `apuWaitBoot` attend `$AA`/`$BB` sans borne (`apu.asm:29`) ; le driver déjà lancé ne les renvoie jamais | code : la console bloque au lieu de rendre une erreur | 🟡 |
| 18 | `apuUpload(src, addr, 0)` | `apu.h:46-53`, aucune borne annoncée | l'octet est envoyé avant le test de fin (`apu.asm:110-121`) : 65 536 octets transférés | code | 🟡 |
| 19 | `audioSetVoiceVolume` | `audio.h:372` : 0-127 | pas de bornage (`audio.c:204-215`) ; VxVOL est signé, donc une valeur ≥128 inverse la phase | code | 🟡 |
| 20 | `AUDIO_PAN_CENTER` | `audio.h:95` : 8 = centre | sur l'échelle 0..15 (`audio.c:365`), 8 donne L=59, R=67 au volume 127 | code | 🟡 |
| 21 | réentrance en callback NMI, non documentée | aucun en-tête ne l'interdit | `profileColorStart` écrit `tcc__r9` en adressage long (`profile.asm:95,98`) ; `dsp1.asm:414-591` et `map.asm:1202,1269` écrivent `tcc__r0`, ce qui contourne l'isolation DP ; `fix32Sin`/`fix32Cos` écrivent `f32_res_lo` (`fixed32.asm:674-707`) ; `textLoadFont*` font `sta $43xx` en absolu (5 dans `text.asm`, 5 dans `text4bpp.asm`, 26 dans `map.asm`) alors que le callback tourne avec DB=$7E (`crt0.asm:1295`) | code | 🟡 (classe : aucune liste des fonctions sûres en NMI) |
| 22 | `sramSave`, `sramSaveOffset` | `sram.h` : « Source pointers may be in any bank » | banque $00 : chemin rapide `mvn $7E,…` (`sram.asm:128-131`, `:157`). Une source en `$00:2000-$FFFF` (ROM banque 0) est lue en WRAM | code | 🟡 |
| 23 | `profileScanline*` avec le Super Scope | aucun effet de bord annoncé | lire `$2137` lève STAT78.6, que `ReadScope` prend pour un tir (`profile.asm:139,167,194`). `gsu_present_step` latche aussi dans la NMI | corpus fullsnes (même chunk que la ligne 10) | 🟡 probable |
| 24 | objets : `objNew(type≥64)`, `nID` sur 8 bits, `objCollidObj` | `object.h:224` (0-63), `:241` (handle périmé → 0) | pas de contrôle de borne (`object.asm:507-553`) ; l'identifiant revient de 255 à 1 (`:559-565`) ; `bmi` asymétrique et bornes incluses (`:2530`) | code | 🟡 |
| 25 | `animPlay` sur un clip `ANIM_ONCE` | `anim.h` dit à la fois « safe every frame » et « finished → restarts » | `anim.c:31-34` : appelé à chaque trame, le clip boucle | code | 🟡 |
| 26 | `collideTile` / `collideRectTile`, bas de carte | `collision.c:130-133` renvoie à `collideRectMap`, qui n'existe pas | aucune borne en hauteur dans `collideRectTile` | code | 🟡 |
| 27 | `UNFIX_ROUND` | `math.h:109` | `x + 128` déborde l'`int` 16 bits pour x ≥ 127.5 | code | 🟡 |

### Doc fausse, code juste (comptés à part, hors critère 7)

| # | où | écart | preuve |
|---|---|---|---|
| D1 | `audio.h:124-128`, `:405-413`, `AUDIO_RELEASE_*` | le champ de 5 bits d'ADSR2 est le sustain rate ; le release après KOFF est fixe. `RELEASE_INSTANT` coupe une note encore tenue | fullsnes `10a5e8717d3ab4e9` (🟠 : la doc fait faire le contraire) |
| D2 | `colormath.h:27-35`, `colorMathTransparency50` | l'exemple de transparence à 50 % calcule (BG2+BG2)/2 ; les exemples du dépôt font l'inverse (`examples/color/transparency/main.c:111-129`) | anomie |
| D3 | `colorMathSetHalf` | pas de division quand le sous-écran est le backdrop | anomie `8095d317ffcf5375` |
| D4 | exemple d'en-tête de `hdma.h` (table vers COLDATA) | aucun plan sélectionné, la table est sans effet | même chunk |
| D5 | `hdmaEnableMask` « starts next frame » ; `hdma.asm:127-135` « clean » | faux si l'activation a lieu en pleine image | luna, ligne 2 ci-dessus |
| D6 | prose d'avant A6 : `hdma.asm:8-19` (« must be in bank 0 »), `sprite.h:697` (gfxptr « bank $00 »), `map.h:150-157` (lecture C en banque $00) | la banque est lue dans le pointeur. L'ancre 8 de `check_doc_drift.py` ne voit pas ces formulations | lecture |
| D7 | `collideTile`, `collideTileEx` | « 0 if out of bounds » ; le code rend 1 (`collision.c:121`, `:128`, `:146`, `:162`) | code |
| D8 | `mapGetMetaTile` | « 0-511 » ; le masque est `$03FF` (`map.asm:1199`), et `$6800` est codé en dur | code |
| D9 | `bgSetMapPtr` | « 1KB aligned » : il s'agit de 0x400 mots | code |
| D10 | `oamMetaDrawDyn` | `MetaspriteItem.tile` est un indice d'image ici, en unités 8x8 ailleurs (`sprite.h:444-451`) : les mêmes données ne valent pas pour les deux fonctions | code |
| D11 | `Scene.init` | « called once » ; en réalité une fois par push (`scene.c`) | code |
| D12 | `snesmodSetModuleVolume`, `snesmodFadeVolume` | 0-127 annoncé ; l'amont dit 0..255 | corpus `3d8c4e3181b85d5e` (probable) |
| D13 | `fix32Div` (`fixed32.h:200` contre `:211`), `fix32Lerp` (débordement présenté comme une perte de précision), `DSP1_FIX_FROM_T` (« truncating » ; c'est un plancher), `AUDIO_PITCH_C3` (le do central est C4), `dma.h` (« ~2,200 cycles ≈ 4KB » incohérent), `mosaic.h:118,136` (exemples avec un nom déprécié), `oamSetFast` (« framesize=158 cliff », résolu) | — | lecture |

## Périmètre couvert

- Les 36 en-têtes `lib/include/snes/*.h` relus contre `lib/source/*.c|*.asm`, `lib/contrib/object.asm` et `templates/crt0.asm`, en quatre passes parallèles plus mes vérifications croisées.
- `crt0.asm` lu de `NmiHandler` à `WaitForVBlank` (lignes 1004-1510). `PHILOSOPHY.md` (principes, l. 50-158), `KNOWN_LIMITATIONS.md`, `silent_defects_log.md`, `docs/BENCHMARK.md` et `MIGRATING_FROM_PVSNESLIB.md` (étape 4).
- Lancés : `check_asm_abi.py` (sur `lib/source` puis `--source lib/contrib`), `make lint-docs` (0 dérive, v0.47.0, 89 exemples), `symmap.py --check-ram-budget` (rpg, tetris, breakout).
- luna v1.31.0 : `luna profile --budget` sur breakout, rpg, dynamic_sprite et map_scroll, plus les mesures des passes (hdma_helpers, random, map_scroll, mapandobjects).
- Corpus cartouche cité par chunk id (exclusions opensnes appliquées). Recherche par motif des banques codées en dur dans tout l'asm de la lib.

## Points forts

- **Le motif de l'object engine ne reste nulle part.**
  - Les sites à DB=$00 (`map.asm:321`, `object.asm:2363`, les 14 de `snesmod.asm`) ne servent qu'aux registres matériels ; la banque source y est prise dans le pointeur (`map.asm:323` `lda 16,s`, `object.asm:2385` `lda 12,s`).
  - Les 13 sites de c9ddef1d lisent `maptile_L1b` (`object.asm:1323`… `:2767`).
  - Les sites à DB=$7E (`lzss:100`, `map:180/498/919/927/1295`, 6 dans `sprite_dynamic`, 14 dans `object`) ne lisent aucune table ROM en absolu (0 occurrence au script). Aucun adressage indirect relatif à DB (`(dp),y`) n'existe dans l'asm de la lib : tous les indirects sont longs (`[ ]`).
- **Lints verts et câblés.** `check_asm_abi` : 339 signatures, 100 fonctions asm vérifiées, 0 écart (y compris les 18 de `object.asm` en lançant à la main). `check_bank_reads`, `check_nmi_wram_race` et `--check-ram-budget` tournent après chaque link (`make/common.mk:621`, `:649`, `:670`).
- **NMI ordonnée selon la règle** : OAM, puis tilemap, puis scroll, puis callback, puis pads (`crt0.asm:1104-1438`). Isolation DP. DB=$7E pendant le callback. `in_nmi_ctx` protège mul/div (`:1288`). Le VOFS de BG3 en offset-per-tile est corrigé (`:1201-1211`). `WaitForVBlank` résiste à un réveil par IRQ (boucle sur le drapeau, `:1506-1508`).
- **Gate `nmi_budget.py`** avec plafond (12 000) et dérive de 10 % par exemple (`nmi_budget.py:50-73`), Super FX inclus.
- **Dépréciations comptées** : 29 `OPENSNES_DEPRECATED`, dont la définition (`types.h:320`), soit 28 fonctions ; 19 `#pragma clang deprecated` de constantes, plus une mention en commentaire (`types.h:316`). Cohérent avec l'annonce, et `lint-docs` (ancre 10) attrape les noms dépréciés cités sans le dire.
- **Modèle mémoire mesuré** : bande plain libre rpg 2784, tetris 1696, breakout 6880 octets ; bande far `$7E:2000+` 52 636 octets libres sur breakout.
- **Une large part des en-têtes tient sa promesse.** Vérifiés sans écart : input (dont le 17e bit de `padIsConnected`), interrupt, dma, lzss, `sqrt16`/`atan2_8`/`fixMul`, offsets DSP-1, polarités des fenêtres, VOFS `y-1` de `mode7SetScroll`.

## Points faibles

1. 🟠 **27 écarts code/promesse trouvés en une seule lecture**, dont 12 orange, 2 mesurés sur luna (lignes 1 et 2) et 1 sur luna + corpus (ligne 10). Le défaut du jour est réel : la relecture n'avait jamais été faite systématiquement, et le critère de gel n°7 ne peut pas démarrer.
2. 🟠 **`nmi_budget.py` mesure le temps propre de l'étiquette `NmiHandler`, pas le handler.** `luna profile` attribue chaque PC à son symbole. `tilemapFlush`, `oamDynamicNmiFlush`, `oamVramQueueUpdate`, le callback utilisateur, `ScanMPlay5`, `ReadMouse` et `ReadScope` sont donc hors du chiffre.
   - Mesuré sur rpg : `NmiHandler` vaut 8 170 mclk au pire, `tilemapFlush` seul 17 652 mclk au pire, invisible au gate.
   - Le docstring (« the whole handler ») et le plafond de 12 000 surestiment la marge VBlank réelle.
3. 🟠 **Classe non outillée : réentrance NMI de la lib** (ligne 21 du tableau). Aucun en-tête ne dit quelles fonctions sont sûres dans un callback ; quatre mécanismes distincts corrompent l'état du thread principal. `check_nmi_wram_race` ne couvre que `$2180-$2183`.
4. 🟠 **Moteur de sprites dynamique** : trois défauts (lignes 4, 5, 6). Le paramétrage promis par `OamDynamicConfig` est faux en dehors des valeurs des exemples ; la file déborde sans borne.
5. 🟠 **Les effets HDMA de haut niveau ne sont pas animables comme promis** (lignes 2, 9, 16, D5). Coût caché : un appel `hdmaIrisWipe` de rayon 80 ≈ 2,15 M mclk, soit environ 6 images, avec un `WaitForVBlank` interne non documenté (`hdma.c:464`) ; le ripple prend environ 90 % du CPU (luna profile). Contraire au principe 5.
6. 🟡 **`check_asm_abi` ne scanne que `lib/source`** (`check_asm_abi.py:457,475`). `lib/contrib/object.asm`, 18 fonctions publiques, n'est pas dans le gate (vert quand on le lance à la main).
7. 🟡 **L'ancre 8 de `check_doc_drift.py` rate la prose d'avant A6** formulée autrement que « assumes … bank $00 » (D6, trois fichiers).
8. 🟡 **Doc audio fausse sur l'enveloppe** (D1) : un utilisateur qui suit la doc obtient le contraire de l'effet voulu.
9. 🟡 **Principe 4 assoupli le jour de l'audit** (de 3 à 5 arguments par appel, `PHILOSOPHY.md:128-137`, « until 2026-10-03 »). Restent au-dessus de 5 : `oamDrawMetaFlip` (11, déprécié), `bgInitTileSet` (8), `oamSet`, `oamInitGfxSet`, `oamDrawMeta` et `dsp1Parameter` (7), `dmaTransfer` et `oamMetaDrawDyn` (6). Les fonctions héritées de PVSnesLib sont exemptées ; `oamDrawMetasprite` est conforme (5 arguments variables).
10. 🟡 **Écarts PVSnesLib** : `MIGRATING_FROM_PVSNESLIB.md` ne liste plus les écarts (étape 4 = table de correspondance seulement, l. 133-150). Les manques ne sont ni assumés ni listés, simplement absents de la page.

## Risques

- **Baselines qui figent un bug.** Les captures map_scroll, slope_collision, tiled et mapandobjects contiennent le VOFS `y-2`. La correction fera bouger quatre baselines, et le diff A/B dira « DIFF » à raison : à documenter dans le commit.
- **Classes « valeur limite » non testées** (taille 0, top=0, type≥64, largeur 256) : aucun manifeste ne les exerce. Le fuzz ne couvre que les outils hôtes.
- **`rand()` déterministe au boot** : jeux identiques à chaque partie. Un utilisateur ne le verra pas en test, ses joueurs si.
- **Mode 7 et multiplicateur PPU** : sûrs aujourd'hui par convention d'usage (appel juste après `WaitForVBlank`). Un jeu qui calcule en fin de trame sera faux sans signal.
- **`KNOWN_LIMITATIONS.md` (37 lignes de VBlank ≈ 50 500 mclk) et `nmi_budget.py` (≈ 51 800) ne disent pas le même chiffre**, et le budget pratique de 4 Ko suppose un handler dont le gate ne mesure pas le coût réel (point faible 2).

## Améliorations recommandées

| # | action | sév. | effort | premier pas concret |
|---|---|---|---|---|
| 1 | Corriger le double −1 du module map | 🟠 | S | retirer le `dec a` de `map.asm:870/886` (ou le `sbc` de `:1133`), revérifier `v_scroll` sur luna (1023 partout), re-baseliner les 4 exemples |
| 2 | Cacher les sprites à X=257 (ou à un Y hors de portée selon la taille) dans `oamHide` / `oamClear` | 🟠 | S | aligner `sprite.c:140` et `:208` sur `sprite_dynamic.asm:427` ; manifeste qui compte les slivers de la ligne 0 en `OBJ_SIZE32_L64` |
| 3 | `nmi_budget` inclusif | 🟠 | S | demander à luna un `--budget` inclusif (callees) via `OPEN_luna.md` ; en attendant, ajouter `tilemapFlush`, `oamDynamicNmiFlush` et `oamVramQueueUpdate` à `EXTRA` |
| 4 | Effets HDMA : passer par `hdmaSetTable` (sans réinitialiser NTRL) quand le canal est déjà actif, et documenter le coût et le `WaitForVBlank` | 🟠 | M | `hdma.c:445` : tester `hdmaGetEnabled` avant `hdmaSetup` |
| 5 | Moteur dynamique : borner la file, valider la taille contre la paire OBJSEL, honorer ou retirer `vramLarge/vramSmall` | 🟠 | M | assert de borne à `sprite_dynamic.asm:780` ; en-tête : « only 0x0000/0x1000 supported » en attendant |
| 6 | `gsuSetupHdmaBlanking` : refuser `top=0`, `top>127` et `top+bottom>224`, et documenter que INIDISP est forcé | 🟠 | S | garde en tête de `superfx.asm:347` |
| 7 | `fixLerp` / `fix32Lerp` : différence sur 17 / 33 bits | 🟠 | S | test hôte avec `fixLerp(FIX(-64), FIX(64), 128) == 0` |
| 8 | Graine RNG : latcher (`lda $2137`) avant de lire OPHCT/OPVCT, ou graine sur `frame_count` au premier appui | 🟠 | S | `console.c:77` ; vérifier sur luna que `rand_seed` diffère entre `--power-on random=1` et `=7` |
| 9 | Marquer « NMI-safe : oui / non » dans chaque en-tête et protéger `tcc__r0`/`r9`, `f32_res_lo` et DB | 🟠 | M | `@nmi_safe` Doxygen + extension de `check_nmi_wram_race` aux symboles de la lib connus non sûrs |
| 10 | Mettre `lib/contrib` dans le gate ABI | 🟡 | S | `Makefile:146` : seconde invocation avec `--source lib/contrib` |
| 11 | Corriger D1-D13 | 🟡/🟠 (D1) | S | commencer par D1 (`audio.h:124`) et D2 (`colormath.h:27`) |
| 12 | Élargir l'ancre 8 du sentinel doc | 🟡 | S | ajouter les motifs « must be in bank 0 », « bank \$00 — cc65816 » |
| 13 | Rétablir dans `MIGRATING_FROM_PVSNESLIB.md` une section « ce qui n'existe pas » | 🟡 | S | lister les modules PVSnesLib absents et dire « assumé » ou « prévu » |
| 14 | Une ligne par défaut orange dans `silent_defects_log.md` | — | S | dater au 2026-10-03, colonne Fix vide |

## Verdict

La bibliothèque a fermé la classe la plus dangereuse de septembre : il ne reste plus aucune banque codée en dur ni aucune lecture relative à DB, et les ABI sont vérifiées. Elle n'est pas prête pour le gel : une seule lecture « promesse contre corps » a produit 12 défauts silencieux orange, dont trois mesurés sur luna, tous dans des fonctions publiques que les exemples n'exercent pas à leurs limites. Le critère 7 ne peut pas démarrer avant que ces lignes soient au journal et corrigées, et que le budget NMI mesure le handler entier plutôt que son étiquette.

Captures luna des passes : `/tmp/claude-1000/-home-kobenairb-workspace-opensnes/e33d7b0e-5fea-4b39-854f-2296e1506ce0/scratchpad/{iris,iris2,bright}/`.

Signalements partenaires relevés pendant l'audit, à reporter (non faits, lecture seule) :
- **snes-rag** : la requête « DMA transfer rate master cycles per byte » ne remonte aucun arbitre SNES ; les registres arithmétiques SA-1 (côté d'usage) restent à arbitrer.
- **luna** : un mode `--budget` inclusif des appelés.

---

Le deuxième rapport de la passe math / puces / audio vient d'arriver. Il ajoute quatre écarts au tableau que je t'ai envoyé et en contredit deux, que je n'ai pas encore départagés dans le code. À intégrer au rapport avant de l'enregistrer dans `.claude/notes/reviews/2026-10-03_audit/B_library.md`.

**Quatre lignes à ajouter au tableau « Défauts silencieux candidats ».** Aucune n'est vérifiée sur luna ; elles reposent sur la lecture du code.

| # | fonction | promesse | code | preuve | sév. |
|---|---|---|---|---|---|
| 28 | `audioLoadSample`, quand `size ≡ 1 (mod 256)` (513, 2817… octets) | `audio.h:231-245` : la fin du chargement est sans ambiguïté, même si le dernier octet d'index vaut 0 | le CPU écrit 0 et lit aussitôt l'écho 0 sur `APU_IO0`, puis enchaîne `cmd_send(OP_DIR_SET)` (`audio.c:283-301`). Le driver, encore entre `inc ZP_IDX` et `bne`, voit la commande et attend 0 sans fin. Résultat : `AUDIO_ERR_TIMEOUT`, échantillon jamais enregistré, driver bloqué | course d'environ 30 cycles SPC. Probable, à reproduire sur luna avec un BRR de 513 octets | 🟠 probable |
| 29 | `snesmodLoadEffect` | `snesmod.h:180-187` : rend un identifiant d'effet utilisable | `spc_sfx_next` augmente sans borne (`snesmod.asm:639-643`) et `snesmodPlayEffect` masque avec `and #$000F` (`:1028`). Le 17e effet rejoue l'effet 0, sans erreur | lecture | 🟡 |
| 30 | `fixMul`, `fixDiv` | `math.h:148`, `:160-174` : « 32-bit intermediate for accuracy » | le résultat est tronqué sur 16 bits sans le dire : `fixMul(FIX(20), FIX(20))` rend −112.0 au lieu de 400.0 | arithmétique (`math.asm:139-170`) | 🟡 (doc incomplète) |
| 31 | `gsuSetupHdmaBlanking`, canal HDMA 1 | `superfx.h:421` : la fonction utilise le canal 1 | `hdma.h` ne dit nulle part que ce canal est pris : un `hdmaSetTable(1, …)` le reprend sans signal | lecture | 🟡 (doc) |

**À ajouter à la liste « Doc fausse, code juste ».**
- **`audioLoadSample`, `loopPoint = 0`** : l'en-tête promet « pas de boucle ». En réalité 0 fait seulement boucler sur le début ; c'est le drapeau du dernier bloc BRR qui décide s'il y a boucle (`audio.h:236`, `audio.c:303`).
- **`snesmodGetPosition`** : l'en-tête dit « numéro de ligne », mais `$2143` donne la position dans l'ordre des patterns (corpus `ebd2823ab0a9deb8`, probable). Conséquence hors de mon périmètre : les manifestes qui vérifient `r_music_pos` sur `superfx_game_skeleton` testent une position, pas une ligne.
- **`fix32Sin`** : l'en-tête annonce « about 30 cycles », c'est environ 60.

**Deux contradictions entre les deux rapports de cette passe.**
- **`audioSetVoiceVolume` (ligne 19)** : le premier rapport dit que la fonction ne borne pas le volume (`audio.c:204-215`), le second la range parmi les fonctions conformes. Cinq minutes de lecture suffisent à trancher. Tant que ce n'est pas fait, je marque la ligne « à confirmer ».
- **`fix32Sin` / `fix32Cos` dans un callback NMI (ligne 21)** : le premier rapport dit qu'elles écrivent `f32_res_lo`, la mémoire de travail de `fix32Mul`, le second les déclare conformes. Il faut vérifier si `fixed32.asm:674-711` écrit vraiment `f32_res_lo`.

**Mises à jour du reste du rapport.**
- Total : 31 écarts de code au lieu de 27, dont 13 orange au lieu de 12, en comptant l'orange probable de la ligne 28.
- La ligne 3 de cette passe (`gsuSetupHdmaBlanking` avec `top = 0`) confirme la ligne 7 du tableau. Le second rapport ajoute que le pas de présentation est protégé (`superfx.asm:598-600`) et que seul `gsuDmaFullFrame` est exposé.
- Ajouter une action 15 au tableau des améliorations : reproduire la course de `audioLoadSample` sur luna avec un échantillon de 513 octets, puis corriger. Le driver doit réarmer avant d'accuser réception, ou le CPU doit attendre un accusé distinct de l'écho. Sévérité 🟠, effort S.
