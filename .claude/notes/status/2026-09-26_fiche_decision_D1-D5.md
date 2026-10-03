# Fiche de décision — D1 à D5 et critères de gel de l'API (2026-09-26)

Pour chaque rangée : ce qui est en jeu, les options, le coût mesuré ce jour dans le dépôt, ma recommandation. Tu réponds par une lettre par rangée (ex. « D1 b, D2 a, D3 a, D4 a, D5 comme proposé »). Chaque décision atterrit en un commit : nouveau nom, ancien nom en alias `OPENSNES_DEPRECATED`, appels du dépôt migrés, `make tests` + `diff_corpus` 85/85. Les alias disparaissent à la première release cassante.

**Pourquoi maintenant** : aujourd'hui un alias coûte zéro pour l'utilisateur (un warning). Après le gel 1.0, chaque rangée devient soit une rupture majeure, soit une incohérence permanente.

## Décisions du propriétaire — 2026-10-03

Séance de questions fermées, une réponse par rangée. Toutes vont dans le sens
de la recommandation de la fiche.

| Rangée | Décision | Ce que ça engage |
|---|---|---|
| **D4** globales sans préfixe | **a** : accesseurs + préfixes, les internes sortent des en-têtes publics | à faire en premier ; un commit par module (map, object, text, colormath, mosaic), 9 exemples, `object.md`, `map.md`. La liste triée « public assumé / interne » de la même famille (`oamMemory`, `frame_count`…) est à fournir dans le même passage |
| **D1** `hdmaEnable` | **b, en deux temps** | maintenant : `hdmaEnableMask` / `hdmaDisableMask` ajoutées, `hdmaEnable` / `hdmaDisable` dépréciées, les 31 appels migrés. À la 1.0 : `hdmaEnable(channel)` réintroduite avec le sens canal |
| **D2** `WaitForVBlank` | **a** : gardée, exception documentée | une ligne dans `PHILOSOPHY.md` et dans le guide de migration |
| **D3** maths | **mixte** : `sqrt16`, `atan2_8`, `mul16` gardées ; `ease_in_quad` / `ease_out_quad` deviennent `easeInQuad` / `easeOutQuad` avec alias | un commit, 6 sites |
| **D5** doublons | **appliquer le tableau ; `TM_*` reste** dans `registers.h` comme noms de registre | `getRegion`, `profileGetFrameCount`, `BGMODE_MODEn`, `WINDOW_*`, `COLORMATH_*`, `MOSAIC_*` (masques de couches) dépréciés ; une ligne de doc pour `VBlankCallback` / `VoidFn`, `mode7SetCenter` / `mode7SetPivot`, `gameLoopRun` / `sceneRun` |
| **Associée 1** principe 4 | **natives à 5 arguments ou plus** reçoivent une variante struct, l'ancienne forme est dépréciée ; `oamSet` et les signatures héritées de PVSnesLib restent | la liste triée « native / héritée » est à présenter au propriétaire **avant** d'y toucher |
| **Associée 2** API morte | **déprécier** `audioUpdate`, `consoleInitEx`, `snesmodSetSoundTable`, `snesmodAllocateSoundRegion` ; **renommer `padRaw`** avec alias | un commit |
| **Associée 3** numéro | **la release cassante est la 1.0** | les 0.x déprécient avec alias ; la 1.0 retire tous les alias (N2-N6, D1-D5, les cinq `*Bank`, `dmaTransfer`) et réintroduit `hdmaEnable(channel)` ; rien ne casse ensuite avant 2.0 |
| **Associée 4** échelle Mode 7 | b, décidé le 2026-10-02 | livré (`93ff5e2d`) |

**Compléments décidés le même jour, sur listes présentées.**

- *Les 46 autres variables exportées* : celles qu'un exemple utilise restent
  publiques sans renommage (`oamMemory`, `oam_update_flag`, `oambuffer`,
  `objWorkspace`, `frame_count`, `vblank_flag`, `text_config`, `dsp1_o0-2`,
  `gsu_cfgr`, `gsu_scmr`, `gsu_scbr`, `gsu_dma_src_hi`, `superfx_status`) ;
  celles qu'aucun exemple n'utilise sortent des en-têtes publics vers un
  en-tête interne, sans renommage (`force_blanked`, `current_brightness`,
  `hdma_wave_speed`, les `scope_*` bruts, les dix `lkup*`, `gsu_prog_bank`,
  `gsu_prog_addr`, `gsu_stop_irqs`, `gsu_owns_cart`, `gsu_scmr_live`,
  `gsu_pres_frames`, `gsu_pres_last`, `sine_table`, `ease_quad_table`).
- *Associée 1 précisée* : variante struct pour les natives à **6 arguments et
  plus** seulement — `oamDrawMetaFlip` (11), `oamDrawMeta` (7),
  `dsp1Parameter` (7), `oamMetaDrawDyn` (6) ; `dmaTransfer` (6) suit son plan
  1.0. Les dix natives à 5 arguments restent (`rectInit`, `panelDraw`,
  `panelClear`, `hdmaSetupIndirect`, `hdmaSetupBank`, `hdmaIrisWipe`,
  `collideTileEx`, `bgLoad`, `audioSetADSR`, `audioPlaySampleOn`), et
  `PHILOSOPHY.md` dira « plus de 5 » au lieu de « plus de 4 ». Héritées de
  PVSnesLib, inchangées : `bgInitTileSet` (8), `oamSet` (7), `oamInitGfxSet` (7).

- *Les variables qu'une fonction `inline` d'en-tête utilise* (constaté en
  appliquant D4 : 16 des variables à sortir sont lues par des corps `inline`
  publics, donc doivent rester visibles tant que l'inline existe). Décision :
  **supprimer l'inline** — `textSetPos`, `colorMathInit`,
  `colorMathSetLayers`, `colorMathDisable`, `mosaicInit`, `setScreenOn`,
  `setScreenOff`, `getBrightness`, `hdmaWaveSetSpeed`, `scopeCalibrate`,
  `scopeSetHoldDelay`, `fixSin`, `fixCos` et les deux `ease*` deviennent de
  vraies fonctions de la lib ; leurs variables quittent les en-têtes et
  prennent le préfixe de leur module (un global non préfixé de la lib
  entrerait en collision à l'édition de liens avec un global homonyme du
  jeu, en-tête ou pas). Coût accepté : un appel de fonction par usage ; à
  mesurer et à écrire dans `docs/PERF.md` (`fixSin` est appelée par trame).
  Les 17 variables qu'aucun inline n'utilise (`lkup*`, sept `gsu_*`) sortent
  des en-têtes comme prévu.

**Avancement.** D4 module carte : fait le 2026-10-03 (`mapGetCameraX()` / `mapGetCameraY()`, `map_cam_x` / `map_cam_y`). Module objet : fait le 2026-10-03 (`objGetCurrentId()`, `objKillCurrent()`, `objGetPointer()` pour `objptr`). Suppression de l'inline et préfixes (text, colormath, mosaic, console, hdma, scope, math) : faits le 2026-10-03, avec D3 (`easeInQuad` / `easeOutQuad`). Coût mesuré : `fixSin` en C compilé +23 % sur `mode2`, donc `fixSin` / `fixCos` écrites en assembleur, +4,9 %. Reste pour D4 : les 17 variables qu'aucun inline n'utilisait (`lkup*`, sept `gsu_*`), toutes déjà préfixées et documentées dans leurs en-têtes — à sortir dans un commit à part. D5 : fait le 2026-10-03 (`getRegion`, `profileGetFrameCount`, `BGMODE_MODEn`, les bits de couche `WINDOW_*` / `COLORMATH_*` / `MOSAIC_*` dépréciés ; une constante dépréciée passe par `#pragma clang deprecated`). **Correction de la fiche** : `mode7SetPivot(u8,u8)` écrit les mêmes registres M7X / M7Y que `mode7SetCenter(s16,s16)` (`lib/source/mode7.c`) — c'est un doublon restreint à 0-255, pas une fonction en coordonnées écran comme la rangée D5 le disait. Sa documentation est corrigée ; la déprécier reste à décider par le propriétaire. Associée 2 (API morte) : faite le 2026-10-03 — `audioUpdate`, `consoleInitEx`, `snesmodSetSoundTable`, `snesmodAllocateSoundRegion` dépréciées ; `padRaw` **dépréciée au profit de `padHeld`** au lieu d'être renommée, car le NMI (`crt0.asm`, lecture des manettes) met déjà à 0 tout mot qui n'est pas celui d'une manette : les deux fonctions rendent la même valeur. D1, premier temps : fait le 2026-10-03 — `hdmaEnableMask` / `hdmaDisableMask` ajoutées (mêmes points d'entrée, deux étiquettes), `hdmaEnable` / `hdmaDisable` dépréciées, tous les appels migrés, vecteur `r_hdma_names` pour les anciens noms. Second temps à la 1.0 : `hdmaEnable(channel)`. D2 : fait le 2026-10-03 (`PHILOSOPHY.md`, guide de migration, `console.h`). D4, fin : faite le 2026-10-03 — les dix `lkup*` et `gsu_pres_frames` / `gsu_pres_last` sortent des en-têtes. **Écart par rapport à la liste décidée** : `gsu_stop_irqs`, `gsu_owns_cart`, `gsu_prog_bank`, `gsu_prog_addr`, `gsu_scmr_live` restent publiques. Le critère « aucun exemple ne l'utilise » les classait internes, mais `docs/tutorials/superfx.md` (l. 157-316, 587) les donne comme le contrat du code qui lance un travail GSU lui-même, et la fixture `libtests_gsu` les utilise ainsi. À confirmer par le propriétaire. `PHILOSOPHY.md` dit « plus de cinq arguments ».

**Reste : associée 1 (variantes à structure), en attente d'un choix de forme.** En l'appliquant, deux constats : (1) `oamDrawMeta` reçoit à chaque trame un pointeur de trame différent (`animTickMeta(...)`), donc une structure « métasprite » contenant ce pointeur devrait être modifiée à chaque trame ; la coupe naturelle est une structure de *style* constante (tuile de base, palette, taille, largeur, hauteur) et les arguments par appel (id, x, y, trame, retournement), soit 6 arguments au lieu de 7 et 11. (2) `dsp1Parameter` : `dsp1_ground` garde `cam_x`, `cam_y`, `cam_aas` comme variables que son manifeste vérifie par symbole ; une `Dsp1Camera` les regroupe proprement mais oblige à réécrire le manifeste par adresses. Formes proposées au propriétaire avant d'écrire le code.

Ordre d'application : D4, puis D3, D5, associée 2, D1 (les petites d'abord
après D4), D2 (doc), associée 1 (après validation de la liste). Une rangée
par commit, alias `OPENSNES_DEPRECATED` partout où c'est possible,
`make tests` et `make lint` à chaque fois.

## D1 — `hdmaEnable(mask)` contre toutes les autres fonctions hdma qui prennent un canal

- **Constat.** 21 fonctions de `hdma.h` prennent `u8 channel` (0–7). `hdmaEnable` et `hdmaDisable` prennent un masque de bits. Le dépôt l'appelle sous 14 formes différentes : `1 << HDMA_CHANNEL_6` (6), `channel_mask(channel)` (5), `1 << HDMA_CHANNEL_0` (5), `1 << 6` (2), `0x0F` (2), `0xFF` (2), `0x40` (1)… La skill de port classe ce point comme piège n° 1.
- **Option a.** Garder le masque, ajouter une macro `HDMA_MASK(ch)`. Aucun appel ne change de sens. L'incohérence reste gravée dans 1.0.
- **Option b.** `hdmaEnable(channel)` et `hdmaDisable(channel)` prennent un canal ; `hdmaEnableMask(mask)` et `hdmaDisableMask(mask)` gardent le comportement actuel. Risque : un appel existant `hdmaEnable(0x40)` compile toujours mais change de sens. Mitigation : une release intermédiaire où `hdmaEnable` est dépréciée au profit de `hdmaEnableMask`, puis la 1.0 réintroduit `hdmaEnable(channel)`.
- **Coût.** 31 appels dans le dépôt.
- **Recommandation : b, en deux temps.** 0.45 : `hdmaEnableMask`/`hdmaDisableMask` ajoutées, `hdmaEnable`/`hdmaDisable` dépréciées, les 31 appels migrés. 1.0 : `hdmaEnable(channel)` réintroduite avec le sens canal. C'est l'API la plus lue par les débutants en effets ; elle doit suivre ses 21 sœurs.

## D2 — `WaitForVBlank`, la seule fonction en PascalCase

- **Constat.** 493 occurrences dans le dépôt. C'est le nom avec lequel arrive chaque port PVSnesLib.
- **Option a.** Le garder, documenté comme l'exception délibérée.
- **Option b.** `waitForVBlank` + alias.
- **Recommandation : a.** La compatibilité de port vaut plus que la cohérence d'une seule fonction, et un alias sur 493 sites ne ferait que du bruit. Une ligne dans `PHILOSOPHY.md` et dans le guide de migration suffit.

## D3 — `sqrt16`, `atan2_8`, `mul16`, `ease_in_quad`, `ease_out_quad`

- **Constat.** Leurs voisines dans `math.h` sont en `fixXxx` camelCase (`fixMul`, `fixSin`, `fixLerp`…). Occurrences : `atan2_8` 31, `sqrt16` 28, `mul16` 13, les deux `ease_*` 3 chacune. Ce ne sont pas des fonctions à virgule fixe : `sqrt16` et `mul16` sont entières, `atan2_8` renvoie un angle 8 bits.
- **Option a.** Les garder : elles se lisent comme les noms libm qu'elles imitent, et `fix` serait faux pour trois d'entre elles.
- **Option b.** Renommer en camelCase sans préfixe `fix` : `isqrt16`, `atan2Angle8`, `mul16`, `easeInQuad`, `easeOutQuad`.
- **Recommandation : a pour `sqrt16`, `atan2_8`, `mul16` ; b pour les deux `ease_*`** (`easeInQuad`, `easeOutQuad`), qui sont les seules en snake_case avec underscore de mot et n'ont que 6 appels. Coût : un commit, 6 sites.

## D4 — les globales exportées sans préfixe

C'est la seule rangée **non aliasable** : on ne peut pas renommer une variable par une macro sans casser un identifiant utilisateur homonyme (`x_pos` est un nom qu'un utilisateur écrira). Différer D4 après 1.0, c'est la figer.

| Globale | En-tête | Utilisée par des exemples | Nature |
|---|---|---|---|
| `x_pos`, `y_pos` | `map.h` | 5 exemples, 3 lignes de doc | caméra de la carte, lue et écrite par l'utilisateur |
| `objgetid` | `object.h` | 4 exemples, 7 lignes de doc | id retourné par l'engine objets |
| `objptr` | `object.h` | 0 exemple, 3 lignes de doc | curseur interne |
| `cursor_x`, `cursor_y` | `text.h` | 0 | état interne du module texte |
| `cgwsel`, `cgadsub` | `colormath.h` | 0 | ombres de registres |
| `mosaic_size`, `mosaic_bg_mask` | `mosaic.h` | 0 | ombres de registres |
| `objtokill` | `object.h` | 0 exemple, cité par `object.md` | drapeau interne |

- **Option a.** Deux catégories. Celles qu'un utilisateur lit ou écrit (`x_pos`, `y_pos`, `objgetid`) passent derrière des accesseurs (`mapGetCamera(&x,&y)` / `mapSetCamera(x,y)`, `objGetCurrentId()`), les variables sont renommées avec préfixe (`map_cam_x`…) et restent visibles pour l'ASM de la lib. Les internes (`cursor_*`, `cgwsel`, `cgadsub`, `mosaic_*`, `objptr`, `objtokill`) sortent des en-têtes publics : renommées avec préfixe de module et déclarées dans un en-tête interne.
- **Option b.** Tout garder et documenter comme API figée.
- **Recommandation : a.** Coût M : un commit par module, 9 exemples à migrer, le tutoriel `object.md` et `map.md`. C'est la rangée à faire en premier.
- **À noter, hors D4 mais même famille** : `oamMemory`, `frame_count`, `vblank_flag`, `oam_update_flag`, `oambuffer`, `force_blanked`, `current_brightness`, `text_config`, `objWorkspace`, les `lkup*` de `sprite.h`, les `gsu_*` de `superfx.h`, `dsp1_o0-2`, `scope_*`, `hdma_wave_speed`. `oamMemory` est public par choix (principe 1, échappatoire). Les autres sont à trier en « public assumé » ou « interne » dans le même passage. Je ferai la liste triée avec la proposition si tu valides l'option a.

## D5 — les doublons

| Paire | Occurrences | Proposition |
|---|---|---|
| `getRegion()` / `isPAL()` | 1 / 1 | renvoient la même valeur depuis N1. Garder `isPAL()`, déprécier `getRegion()` |
| `getFrameCount()` / `profileGetFrameCount()` | 7 / 1 | garder `getFrameCount()`, déprécier la version `profile` |
| `VBlankCallback` / `VoidFn` | 0 / 0 dans les exemples | garder `VBlankCallback` là où il documente le rôle ; `VoidFn` reste le type générique. Pas de dépréciation, une ligne de doc |
| `BG_MODE0-7` / `BGMODE_MODE0-7` | 102 / 1 | garder `BG_MODEn`, déprécier `BGMODE_MODEn` (`registers.h`) |
| `mode7SetCenter(s16,s16)` / `mode7SetPivot(u8,u8)` | 9 / 3 | ce ne sont pas des doublons : `SetCenter` pose le centre dans l'espace carte, `SetPivot` en coordonnées écran. Garder les deux, et dire la différence dans `mode7.h` |
| `gameLoopRun` / `sceneRun` | 24 / 7 | pas des doublons : `sceneRun` est la version multi-scènes, documentée comme telle. Garder les deux |
| cinq jeux de constantes de masque de couches de même valeur : `TM_BG1…`, `LAYER_BG1…`, `WINDOW_BG1…`, `COLORMATH_BG1…`, `MOSAIC_BG1…` | 39 / 25 / 6 / 5 / 1 | garder `LAYER_*` (nom générique, `video.h`), déprécier les quatre autres ; `TM_*` reste comme nom de registre dans `registers.h` si tu préfères |

- **Recommandation : appliquer le tableau.** Coût S, un commit.

## Décisions associées, à trancher dans la même session

1. **Principe 4 (≥ 4 arguments → struct).** ~37 fonctions publiques le violent, dont `oamDrawMetaFlip` (11 arguments) et `bgInitTileSet` (8). Proposition : `oamSet` et les signatures héritées de PVSnesLib restent (compatibilité de port, principe 1) ; les API natives OpenSNES à ≥ 5 arguments reçoivent une variante struct, l'ancienne forme est dépréciée. Je fournis la liste triée « native / héritée » avant d'y toucher.
2. **API morte.** Déprécier `audioUpdate` (no-op), `consoleInitEx` (ignore son argument), `snesmodSetSoundTable` et `snesmodAllocateSoundRegion` (aucun chemin de streaming) ; renommer `padRaw` qui renvoie la valeur filtrée.
3. **La release cassante est-elle la 1.0 ?** Proposition : oui. La 1.0 retire tous les alias dépréciés (N2–N6, D1–D5, les cinq `*Bank`, la signature de `dmaTransfer`), et rien ne casse ensuite avant 2.0.

4. **L'échelle de Mode 7 (ajouté le 2026-10-02).** `mode7.h` documentait
   `mode7SetScale(0x0100)` comme l'échelle 1.0 ; le code la divise par deux
   (`mode7SetAngle` multiplie par un cosinus d'amplitude 127 et décale de 8,
   donc A = $7F) : `0x0100` grossit deux fois, `0x0200` est le 1:1, et
   `mode7Transform(deg, 100)` grossit deux fois aussi. Et `mode7Init()`
   écrit une matrice identité ($0100) que le premier `mode7SetAngle(0)`
   transforme en ×2. La doc est corrigée pour dire ce que fait le code (pas
   de changement de rendu). **Option a** : garder ce contrat (`0x0200` =
   1:1), documenté. **Option b** : corriger le code pour que `0x0100` = 1:1
   (décaler de 7) et diviser par deux les constantes de `rotate_scale`,
   `mode7_racing` (qui garde l'échelle par défaut) et `mode7_flying` pour
   garder leurs images ; un projet utilisateur qui passait `0x0200` voit son
   plan dézoomé. **Recommandation : b avant le gel** — c'est le contrat que
   la doc promettait, et `mode7Init` + `mode7SetAngle(0)` deviennent
   cohérents. **Décidé b le 2026-10-02** (« oui on est parti ») : le code
   double l'échelle dans `mode7SetAngle`, `rotate_scale`, `mode7_racing`,
   `mode7_flying` et `extbg` passent des échelles moitié, images
   identiques (`diff_corpus`).

## Critères de gel proposés

La 1.0 part quand toutes ces lignes sont vraies, chacune prouvée dans le dépôt :

1. D1–D5 et les trois décisions associées ont atterri.
2. Aucun 🔴 ouvert dans l'état des lieux du 2026-09-26, sauf les assets PVSnesLib, reportés par décision du propriétaire et inscrits au ROADMAP.
3. Le zip de release est construit et testé par la CI depuis l'artefact lui-même.
4. Le sentinel de dérive couvre les noms de fonctions cités en doc.
5. Une session console a rempli au moins les rangées 1 à 7 du protocole matériel.
6. Le chantier Super FX est soit terminé (phases C–F), soit borné et documenté comme expérimental, avec des signatures `gsu*` qui ne gèleront pas un pipeline de démo.
7. Deux semaines sans nouvelle entrée 🟠 dans `KNOWN_LIMITATIONS.md`.
