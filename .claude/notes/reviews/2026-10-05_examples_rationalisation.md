# Les 91 exemples : inventaire, doublons, proposition (2026-10-05)

Demandé par le propriétaire avant la migration des exemples sur les fichiers
de réglages (`docs/tools/CONVENTIONS.md`) : « c'est l'occasion de revoir
tous ces exemples ; 30 % sont des répétitions ». Lu sur `develop` à
`2fbceeef`. Rien n'est supprimé ici : c'est la proposition, à trancher par
le propriétaire, et la méthode pour l'appliquer sans casser ce que les
exemples tiennent.

## 1. Ce que l'inventaire mesure

Pour chacun des 91 : la catégorie, le titre et les « SNES Concepts » du
README, les modules liés, les lignes de `main.c`, le nombre de manifestes
luna qui le jouent, le nombre de pages de `docs/` qui le citent, et quatre
contraintes dures :

| Contrainte | Source | Ce qu'elle impose |
|---|---|---|
| **Grille console** | `docs/HARDWARE_VERIFICATION.md` (26 ROM) | un exemple de la grille ne se supprime pas ; il peut absorber un autre |
| **Seul exécutant** d'une fonction publique | `testing/ROM_COVERAGE.md`, table « one ROM only » | supprimer l'exemple découvre la fonction : son code passe dans l'exemple qui l'absorbe ou dans une fixture `libtests*` |
| **Manifestes et baselines** | `testing/manifests/`, `baselines/` | un manifeste suit son ROM (renommé ou fusionné) ou disparaît avec lui ; les baselines WRAM / fbhash / audio se recapturent |
| **Références des docs** | tutoriels (`@ref examples_*`), `examples/README.md` (échelle numérotée et table par catégorie), `KNOWN_LIMITATIONS.md` | le sentinel (ancres 3, 5, 6) refuse un chemin mort et un compte faux ; chaque référence se réécrit vers l'exemple absorbant |

Treize README n'ont pas de leçon (ils commencent par `cd $OPENSNES_HOME &&
make`) : `snesmod_music_large`, `snesmod_sfx`, `collision_demo`,
`sa1_hello`, `sa1_starfield`, `superfx_3d`, `superfx_hello`,
`transparency`, `text_glyphs`, `breakout`, `likemario`,
`continuous_scroll`, `dynamic_metasprite`. Qu'ils restent ou non, ceux qui
restent reçoivent un README qui dit ce qu'on apprend.

## 2. Les doublons, catégorie par catégorie

Le critère : **un exemple survit s'il enseigne quelque chose qu'aucun autre
n'enseigne, ou s'il est le seul à exécuter une fonction de la lib, ou s'il
est dans la grille console**. Deux exemples qui enseignent la même chose à
deux tailles fusionnent dans le plus complet, avec un bouton pour passer
d'une variante à l'autre quand les deux valent d'être vues.

### Premier niveau : les fusions sûres (19 exemples, 91 → 72)

| Catégorie | Disparaît | Absorbé par | Pourquoi | À déplacer |
|---|---|---|---|---|
| audio | `snesmod_music_large` | `snesmod_music` | même leçon avec un module plus gros ; le franchissement de banque devient un second module du même exemple | ses 2 manifestes |
| audio | `speech_synth`, `play_noise`, `pitch_mod`, `echo` | un seul `audio/dsp_effects` (quatre tours du S-DSP sous quatre boutons) | quatre portages krom d'une astuce DSP chacun, aucun cité par les docs ; un studio veut les voir côte à côte | `audioDisableEcho` (seul exécutant : `echo`), les 9 manifestes, 4 baselines audio |
| backgrounds | `mode1_bg3_priority` | `mode1` | un drapeau (`BG3_MODE1_PRIORITY_HIGH`) : un bouton dans `mode1` | — |
| backgrounds | `mode5` | `mode5_hires` | deux démos du Mode 5 ; `mode5_hires` est dans la grille et va plus loin (entrelacé 448) | — |
| backgrounds | `mode4` | `mode2` | la trilogie offset-per-tile (2/4/6) en deux : `mode2` montre 2 et 4 par bouton, `mode6` (grille) reste | — |
| basics | `timer` | `text/print_string` enseigne déjà le rythme VBlank ; `game_skeleton` le compteur | la plus petite leçon du corpus, déjà donnée deux fois | — |
| basics | `random` | `game_skeleton` | le RNG s'apprend dans le squelette de jeu, où il sert | — |
| basics | `fix32_orbit` | `aim_target` | deux démos de maths ; la cible qui orbite devient le mobile d'`aim_target` | — |
| chips | `sa1_hello` | `sa1_starfield` | « ça démarre » est la première minute de la vitrine (grille) | son manifeste |
| color | `hicolor_blend` | `hicolor_1792` | deux portages krom « plus de couleurs » ; `hicolor_1792` est dans la grille | — |
| color | `gradient_9bit` | `hdma/gradient_colors` | deux dégradés de fond par HDMA | — |
| games | `mapandobjects` | `games/likemario` et `maps/slope_collision` couvrent carte + moteur d'objets | 147 lignes, pas un jeu | ses tables `objfct*` servent de classe de dérive à l'oracle WRAM : vérifier qu'un autre ROM les porte |
| input | `move_sprite` | le `starter/` | c'est exactement le programme du starter | — |
| maps | `map_scroll` | `maps/tiled` | deux cartes Tiled qui défilent ; Mario rejoint le niveau de `tiled` | ses 2 manifestes |
| mode7 | `perspective` | `rotate_scale` (un bouton passe de la rotation plate au sol en perspective) | `perspective_rotate` (grille) garde la matrice complète | — |
| scrolling | `mixed_scroll` | `parallax_scroll` | deux couches à des vitesses différentes : la parallaxe le fait avec trois | — |
| sprites | `dynamic_metasprite` | `dynamic_sprite` (variante métasprite sous un bouton) | la combinaison de deux leçons déjà données ; README nu | ses 2 manifestes |

### Second niveau : à trancher (6 de plus, 72 → 66, soit 27 % en tout)

| Disparaît | Absorbé par | Ce qui plaide pour, ce qui retient |
|---|---|---|
| `sprites/sprite_sizes` | `simple_sprite` (un bouton fait défiler les six tailles OBJSEL) | pour : un menu de 252 lignes pour six valeurs ; contre : la référence visuelle des six combinaisons est pratique telle quelle |
| `transitions/mosaic` | `fading` → un seul `transitions` | pour : les deux effets se combinent déjà dans `mosaic` ; contre : `mosaic` est cité par six pages |
| `chips/superfx_hello` | `superfx_3d` | pour : « ça démarre » ; contre : ses deux manifestes épinglent le chemin de job GSU minimal (`libtests_gsu` en couvre une partie) |
| `hdma/hdma_indirect_gradient` | `gradient_colors` (le mode indirect sous un bouton) | pour : troisième dégradé ; contre : seul exécutant de `hdmaSetupIndirect`, portage krom documenté |
| `text/scroll_message` | `print_string` | pour : « déplacer le texte » tient en une ligne de `bgSetScroll` ; contre : c'est la marche 2 de l'échelle, lue par les débutants |
| `maps/dynamic_map` | — | pour : un moteur maison de « sprites en tilemap » (474 lignes) qui n'est pas celui de la lib ; contre : six pages le citent, et il montre un vrai usage de BG1 comme grille |

Ce que je garde sans hésiter, parce que rien d'autre ne l'enseigne : les
sept jeux restants, la grille console entière, les seuls exécutants
(`shadow_tint`, `direct_color`, `dsp1_cube`, `dsp1_ground`, `superfx_3d`,
`hdma_helpers`, `parallax_scroll`, `apu_switch`), `soundboard` et
`sfx_from_wav` (le moteur audio v2 complet et sa première marche),
`collision_demo` (seule démo du module, README à écrire), `text_glyphs`
(le niveau « sous le capot »), `pseudo_hires` (la question du Mode 6 pour
luna et snes-rag, rangée 23), `aseprite_pipeline` (la vitrine de la
famille d'outils).

## 3. Ce que ça donne

| | Aujourd'hui | Premier niveau | Les deux niveaux |
|---|---|---|---|
| Exemples | 91 | 72 | 66 |
| README sans leçon | 13 | 6 (à écrire) | 6 |
| Portages krom isolés | 10 | 5 | 4 |
| Fonctions découvertes | 0 | 0 (déplacées) | 0 (déplacées) |

Les catégories restent les mêmes ; `games/` passe à 7, `audio/` à 6,
`basics/` à 5.

## 4. La méthode, prudente

Un commit par catégorie, dans cet ordre, et chaque commit vérifié par la
même grille :

1. **Fusionner d'abord, migrer ensuite.** La fusion (deux `main.c` en un,
   un bouton) se fait sur le `data.asm` existant ; la migration vers les
   fichiers de réglages vient quand la catégorie est stable. Deux
   changements dans un commit, c'est une régression qu'on ne sait plus
   attribuer.
2. **Avant de supprimer** : la table du §1 pour l'exemple — grille (non),
   seul exécutant (déplacé où ?), manifestes (renommés ou supprimés),
   références (réécrites) ; `rom_coverage.py` doit rester à 325/325 sans
   ligne nouvelle dans `never_executed.txt`.
3. **Pour chaque catégorie** : `make clean && make` du dossier, `luna_runner
   --compare --only <cat>` et `wram_regress --only <cat>` sur les survivants,
   `make test-manifests`, `make lint-docs` (ancres 3, 5, 6, 9 : chemins,
   comptes, modules des README), `make docs-strict`. Les baselines d'un
   exemple fusionné sont recapturées **une fois**, avec la raison dans le
   commit, comme `testing.md` le demande.
4. **La migration elle-même**, par catégorie : un `<asset>.toml` par source,
   le `data.asm` supprimé, les `extern` du `main.c` alignés sur les noms que
   l'outil génère (`<nom>_til`, `<nom>_tilend`, `<nom>_pal`). Un ROM migré
   doit donner **la même image et le même flux WRAM** qu'avant (la section
   change de nom, pas le contenu) ; une différence de fbhash est à expliquer
   avant toute recapture.
5. **Les treize README qui commencent par la commande de build** sont
   remis dans l'ordre de `new_example.md` au passage (ce qu'on apprend
   d'abord, la commande ensuite) : relecture faite, ils ont tous une section
   « What You'll Learn », placée après `Build & Run` — c'est l'ordre qui
   était en cause, pas l'absence de leçon.

Ordre proposé : `text`, `input`, `basics`, `sprites`, `backgrounds`,
`scrolling`, `hdma`, `color`, `windows`, `transitions`, `mode7`, `maps`,
`memory`, `audio`, `chips`, `games` — du plus simple au plus lourd, les
jeux en dernier parce que leurs assets sont les plus nombreux.

## 5. Ce qui n'est pas dans cette proposition

- Les **assets hérités de PVSnesLib** (décision reportée le 2026-09-26) :
  la migration ne change pas leur provenance ; `ATTRIBUTION.md` reste à
  jour fichier par fichier.
- Le **compte « 91 exemples »** apparaît dans `CHANGELOG.md` (gelé) et dans
  les docs actives : le sentinel (ancre 3) fait suivre les secondes.
- `examples/README.md` a deux structures (la table par catégorie et
  l'échelle numérotée avec ses marches `15c…15n`, `22b`, `42c…42g`) :
  l'échelle est à renuméroter d'un bloc une fois les fusions faites, pas
  marche par marche.

## 6. Le défi (même jour) : ce que les données ont contredit

Le propriétaire a validé la proposition en me demandant de la remettre en
cause. Je l'ai fait avec des mesures : pour chaque fusion, les appels de la
lib que le candidat fait et que l'absorbant ne fait pas, ses manifestes et
ce qu'ils affirment, les pages qui le citent. Verdict : **huit des vingt-
cinq fusions ne tiennent pas**, et une change d'absorbant.

| Fusion proposée | Ce que les données disent | Décision |
|---|---|---|
| `speech_synth`, `play_noise`, `pitch_mod` → `soundboard` | ils tournent sur le chemin APU **brut** (`apuUpload` / `apuExecute`, un programme SPC700 à eux) ; `soundboard` est le moteur audio v2 en C pur. Trois fonctions du S-DSP différentes, pas une répétition | **retirée** ; ils sont du niveau « sous le capot » et pourraient rejoindre `fundamentals/` un jour, c'est une autre question |
| `echo` → `soundboard` | `echo` est bien sur le moteur v2 (`audioPlaySample`) et `soundboard` annonce déjà l'écho dans ses concepts | **gardée** |
| `timer` → `game_skeleton` | `timer` est la démo du module `gameloop` (`gameLoopRun`), cité par `tutorials/framework.md` et `API_INDEX.md` ; `game_skeleton` ne l'utilise pas | **retirée** |
| `mapandobjects` → `likemario` | `likemario` n'utilise pas le moteur de carte ; mais `maps/slope_collision` fait exactement les mêmes appels de la lib (ensemble identique) et ajoute les pentes | **gardée, absorbant corrigé** : `slope_collision` ; `tutorials/object.md` à réécrire vers lui |
| `map_scroll` → `tiled` | sept pages citent `map_scroll` (le tutoriel `map`, `camera`, `tiles-to-levels`, l'index d'API) ; sept citent `tiled` | **retirée** : deux leçons canoniques, la fusion coûterait quatorze réécritures pour un gain d'un dossier |
| `mixed_scroll` → `parallax_scroll` | `mixed_scroll` est le défilement à deux couches par `bgSetScroll`, `parallax_scroll` le fait par HDMA : deux mécanismes, et cinq pages citent le premier | **retirée** |
| `dynamic_metasprite` → `dynamic_sprite` | seule démo de `oamMetaDrawDyn` / `oamDynamicDrainQueue` ; deux manifestes épinglent OBSEL et INIDISP | **retirée** ; son README nu est à écrire |
| second niveau : `sprite_sizes`, `mosaic`, `superfx_hello`, `hdma_indirect_gradient`, `scroll_message` | chacun a une API ou un manifeste que personne d'autre ne porte, ou une place dans l'échelle lue par les débutants | **retirées**, toutes les six (`dynamic_map` incluse) |

### La liste révisée (13 fusions, 91 → 78)

`snesmod_music_large` → `snesmod_music` · `echo` → `soundboard` ·
`mode1_bg3_priority` → `mode1` · `mode5` → `mode5_hires` · `mode4` →
`mode2` · `random` → `game_skeleton` · `fix32_orbit` → `aim_target` ·
`sa1_hello` → `sa1_starfield` · `hicolor_blend` → `hicolor_1792` ·
`gradient_9bit` → `gradient_colors` · `mapandobjects` → `slope_collision` ·
`move_sprite` → le `starter/` · `perspective` → `rotate_scale`.

C'est 14 %, pas 30 %. L'intuition du propriétaire visait juste sur la
*forme* (treize README qui ouvrent sur la commande de build, dix portages krom posés côte à côte,
des catégories qui ont poussé par accrétion) ; la mesure dit que le
*contenu* se recoupe moins qu'il n'y paraît, parce que les exemples qui se
ressemblent exercent souvent des fonctions différentes de la lib. Le vrai
gain de lisibilité viendra autant des README écrits et de l'échelle
renumérotée que des suppressions.

### Journal

- `move_sprite` retiré (le `starter/` est ce programme) : dossier, manifeste
  `movement_move_sprite.toml`, baselines fbhash et WRAM, les six pages et
  le compte (`91` → `90` dans huit fichiers, le sentinel en a trouvé trois
  de plus dans `ROADMAP.md`).

## 7. Second tour du défi : les fusions de code

Les cinq retraits sans code sont faits (`move_sprite`, `hicolor_blend`,
`gradient_9bit`, `mode5`, `random` : 91 → 86). Avant d'écrire du code pour
les huit fusions restantes, chacune relue avec les deux programmes, leurs
manifestes et leurs citations sous les yeux :

| Fusion | Ce que la lecture a montré | Décision |
|---|---|---|
| `mode1_bg3_priority` → `mode1` | pas « un drapeau » : trois couches, trois banques de palette, le bit de priorité ; un manifeste qui épingle neuf destinations VRAM / CGRAM ; deux pages d'artisanat ; la marche 6 de l'échelle. `mode1` est la marche 5, le fond le plus simple, avec son propre manifeste (le `bgLoad` en trois transferts) | **retirée** |
| `mode4` → `mode2` | la trilogie offset-per-tile (2/4/6) est une échelle voulue (« after mode2 », « after mode2 and mode4 ») ; les trois modes diffèrent par la profondeur et le nombre de couches, un studio choisit le sien | **retirée** |
| `fix32_orbit` → `aim_target` | c'est la démo du module `fixed32`, citée par `tutorials/math.md` — le même critère qui a gardé `timer` pour `gameloop` | **retirée** |
| `sa1_hello` → `sa1_starfield` | chaque puce a sa trilogie « ça démarre / la vitrine / la sauvegarde », et le manifeste de `sa1_hello` est la preuve de démarrage (statut SA-1, trace) que lit le tutoriel | **retirée** |
| `perspective` → `rotate_scale` | marches 21 et 22 de l'échelle, quatre pages citent `perspective` ; c'est la leçon canonique de l'écran coupé par HDMA | **retirée** |
| `mapandobjects` → `slope_collision` | mêmes appels de la lib, mais `tutorials/object.md` est écrit sur son code (cinq passages) | **reportée** : se fait avec la réécriture du tutoriel, pas avant |
| `snesmod_music_large` → `snesmod_music` | son manifeste d'entrée (84 lignes) mesure le FIFO du pilote **sur le téléversement de 56 Ko au démarrage** (la pression doit venir après la trame 85) ; fusionné, soit cette propriété disparaît (petit module au boot), soit les six manifestes et la baseline audio de `snesmod_music` changent. Un dossier de moins ne vaut pas un test mesuré de moins | **retirée** |
| `echo` → `soundboard` | deux manifestes sec / mouillé qui épinglent les registres d'écho du DSP (`EDL`, `EFB`, `EVOL`, `FIR0`) à des trames précises, et `audioDisableEcho` n'a pas d'autre exécutant ; le portage dans `soundboard` les remesure tous pour un dossier | **retirée** |

### Bilan honnête

Cinq retraits au total sur vingt-cinq proposés : **91 → 86, soit 5 %**, pas
30 %. Le corpus se ressemble plus qu'il ne se répète : des
exemples voisins exercent des fonctions différentes de la lib, portent des
manifestes différents, et occupent des marches différentes de l'échelle que
les débutants lisent. Ce que l'intuition du propriétaire a bien vu, et qui
reste à faire, est la **forme** : treize README qui ouvrent sur la commande de build, une échelle
numérotée par accrétion (`15c…15n`, `22b`, `42c…42g`), des catégories dont
la table ne dit plus ce qu'on apprend. Le gain de lisibilité est là, et dans
la migration vers les fichiers de réglages qui retire 54 `data.asm`.

### Journal (suite)

- L'échelle d'`examples/README.md` est renumérotée en séquence (1 à 55) : les
  marches `15c…15n`, `22b`, `22c`, `23b`, `42c…42g` étaient l'historique des
  ajouts, pas un ordre.
- Correction d'une erreur de lecture du §1 : les treize README « nus »
  ont tous une section « What You'll Learn » — après `Build & Run`. Le
  défaut est l'ordre, pas l'absence.
- La CI a refusé le premier push des retraits : `docs/Doxyfile` listait les
  cinq dossiers un par un, et localement leurs répertoires survivaient
  comme sorties de build non suivies, donc `docs-strict` passait ici et pas
  là-bas. Corrigé ; `make clean` d'un dossier retiré avant son `git rm` est
  la bonne séquence.

### Journal — la migration vers les fichiers de réglages (2026-10-05)

- **Les `.inc` générés parlent la langue d'`asset.h`** : `opensnes-sprite`
  et `opensnes-tileset` écrivent `<nom>_tiles` / `_pal` / `_map` (chacun
  avec `_end`) et un `DECLARE_GFX_ASSET` / `DECLARE_BG_ASSET` prêt, donc
  `#include "res/town.inc"` puis `bgLoad(0, &town, …)` est tout le
  chargement. Un fragment `_data.as` porte une `ASSET_SECTION` par bloc,
  et un bloc au-dessus de 32 Ko est découpé en parts (`_tiles`, `_tiles_1`),
  ce que `mode3` et `hdma_wave` faisaient à la main. Un tileset LZ77 ou
  découpé reçoit des `extern` nus, pas de bundle.
- **Ordre tenu** : sprites (6), backgrounds (5), six catégories (14), mode7
  (4), maps (4) + `sfx_from_wav`, games (7). Par exemple : la sortie
  régénérée comparée octet à octet à celle de gfx4snes (`cmp` sur l'arbre
  précédent), les pixels (fbhash), les manifestes, et pour chaque baseline
  WRAM recapturée le diff octet par octet des deux ROM sur 300 trames
  (`luna wram-trace`, toutes les pages que l'oracle hache, nommées par le
  `.sym`) : à chaque fois des copies d'adresses d'assets déplacés
  (`tcc__r9`, `oambuffer[].gfx`, `oamQueueEntry`, `sprit_val2`, le pointeur
  de tileset du module map en bande FAR à $3809). Rien d'autre.
- **Un défaut du compilateur trouvé par la migration** : deux fichiers C
  d'un même projet ne pouvaient pas définir le même `static` de portée
  fichier (label WLA global, « defined more than once ») ; `slope_collision`
  l'a révélé en incluant `mario_sprite.inc` depuis deux fichiers. Corrigé
  dans cproc (`name.<source>`), prouvé par 85 ROM identiques octet à octet,
  une fixture liée (`static_dup`) et un cas de `test-compiler`.
- **Deux erreurs de méthode corrigées en route** : la première version du
  script de diff WRAM ignorait les pages au-dessus de la pile (il ne voyait
  pas la bande FAR), et la justification écrite dans un message de commit
  (« zéro trame diffère ») était fausse pour `aseprite_pipeline` : amendé
  avant le push, et le script refait pour couvrir exactement ce que
  l'oracle hache.
- **Restent en `data.asm`, par nature** : tables HDMA et helpers asm
  (`gradient_colors`, `perspective`, `hdma_wave`, les deux jeux Mode 7),
  tables de sinus, cartes `.m16/.t16/.b16/.o16` (tmx2snes n'est pas encore
  de la famille : `opensnes-level`), images SPC700 (`*.spc700.bin`), `.brr`
  sans `.wav` source, `.dat` de breakout, polices binaires, sections RAM
  de tetris, et les trois exemples sans source de conversion
  (`mode5_hires`, `hicolor_1792`, `echo`).
