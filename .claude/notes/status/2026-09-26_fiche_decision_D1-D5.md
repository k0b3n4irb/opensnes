# Fiche de décision — D1 à D5 et critères de gel de l'API (2026-09-26)

Pour chaque rangée : ce qui est en jeu, les options, le coût mesuré ce jour dans le dépôt, ma recommandation. Tu réponds par une lettre par rangée (ex. « D1 b, D2 a, D3 a, D4 a, D5 comme proposé »). Chaque décision atterrit en un commit : nouveau nom, ancien nom en alias `OPENSNES_DEPRECATED`, appels du dépôt migrés, `make tests` + `diff_corpus` 85/85. Les alias disparaissent à la première release cassante.

**Pourquoi maintenant** : aujourd'hui un alias coûte zéro pour l'utilisateur (un warning). Après le gel 1.0, chaque rangée devient soit une rupture majeure, soit une incohérence permanente.

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

## Critères de gel proposés

La 1.0 part quand toutes ces lignes sont vraies, chacune prouvée dans le dépôt :

1. D1–D5 et les trois décisions associées ont atterri.
2. Aucun 🔴 ouvert dans l'état des lieux du 2026-09-26, sauf les assets PVSnesLib, reportés par décision du propriétaire et inscrits au ROADMAP.
3. Le zip de release est construit et testé par la CI depuis l'artefact lui-même.
4. Le sentinel de dérive couvre les noms de fonctions cités en doc.
5. Une session console a rempli au moins les rangées 1 à 7 du protocole matériel.
6. Le chantier Super FX est soit terminé (phases C–F), soit borné et documenté comme expérimental, avec des signatures `gsu*` qui ne gèleront pas un pipeline de démo.
7. Deux semaines sans nouvelle entrée 🟠 dans `KNOWN_LIMITATIONS.md`.
