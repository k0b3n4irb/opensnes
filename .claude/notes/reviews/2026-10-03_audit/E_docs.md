# Audit — documentation et accueil (2026-10-03, campagne de chasse avant la 1.0)

Rapport de l'agent `audit-docs`, non retouché (l'agent n'a pas pu écrire le fichier lui-même ; copié depuis sa sortie).

## Les 47 noms dépréciés

Ils ont été relevés par `grep -rn -A1 OPENSNES_DEPRECATED lib/include/snes` (28 fonctions) et `grep -rn "pragma clang deprecated" lib/include` (19 constantes). Les 28 remplaçants existent tous. Pour chaque nom, j'ai lu les deux implémentations dans `lib/source`.

| Ancien | Nouveau | Message | Vérifié |
|---|---|---|---|
| `sa1Init` | `sa1IsReady` | crt0 boots the SA-1; this only reads its status | oui (sa1.c:19 appelle sa1IsReady) |
| `ease_in_quad` / `ease_out_quad` | `easeInQuad` / `easeOutQuad` | use … | oui (math_ease.c:59-65, même table) |
| `hdmaSetupBank` | `hdmaSetup` | takes the bank from the table pointer | oui (hdma.asm:75-82 lit l'octet de banque à 7,s) |
| `hdmaEnable` / `hdmaDisable` | `hdmaEnableMask` / `hdmaDisableMask` | at 1.0 … will take a channel number | oui, même point d'entrée (hdma.asm:357, 378). **Ces deux noms ne sont pas retirés, ils changent de sens.** |
| `LzssDecodeVram` | `lzssDecodeVram` | lower-case l | oui, deux étiquettes pour une adresse (lzss.asm:88-89) |
| `dmaCopyVramBank` / `dmaCopyCGramBank` | `dmaCopyVram` / `dmaCopyCGram` | takes the bank from the source pointer | oui |
| `colorMathEnable` | `colorMathSetLayers` | replaces the layer set | oui (colormath.c:61 délègue) |
| `nmiSetBank` / `irqSetBank` | `nmiSet` / `irqSet` | takes the bank from the pointer | oui, avec une nuance : `nmiSetBank(NULL, b)` installe un saut vers une cible nulle alors que `nmiSet(NULL)` désactive (console.c:290-308). Le remplaçant est plus sûr. |
| `profileGetFrameCount` | `getFrameCount` | same counter | oui (`frame_count`, profile.asm:238 et console.h:274) |
| `mosaicEnable` | `mosaicSetLayers` | replaces the BG set | oui |
| `audioUpdate` | (retirer l'appel) | does nothing | oui (audio.c:164, corps vide) |
| `mode7SetPivot(u8,u8)` | `mode7SetCenter(s16,s16)` | same registers | oui (M7X/M7Y, mode7.c:154 et 178) |
| `padRaw` | `padHeld` | never returned the raw register | vrai, mais **les deux ne sont pas équivalents** : `padHeld` rend 0 quand l'état vaut $FFFF, `padRaw` rend $FFFF (input.c:55-75). À écrire dans UPGRADING. |
| `scopeButtonsDown` | `scopeButtonsHeld` | "currently down" | oui (même `scope_down`) |
| `snesmodSetSoundTable` | (retirer) | table never read | oui (`SoundTable` n'est écrit qu'aux lignes 283 et 1226 de snesmod.asm, jamais lu) |
| `snesmodAllocateSoundRegion` | (retirer) | region never used | presque : l'appel envoie bien `CMD_SSIZE` au driver SPC (snesmod.asm:1241-1270). Le retirer est sans danger, mais ce n'est pas un appel sans effet. |
| `dsp1Parameter` | `dsp1SetCamera` | seven args → `Dsp1Camera` | oui (sortie partagée `_dsp1_param_out`, dsp1.asm:371) |
| `dsp1Present` (u16) | `dsp1IsPresent` (u8) | — | oui, même routine, rend 1 ou 0 (dsp1.asm:665-666) |
| `oamDrawMeta` / `oamDrawMetaFlip` | `oamDrawMetasprite` | style struct / 11 args, 8/16 px | oui. **Point de mise en œuvre pour la 1.0** : `oamDrawMetasprite` appelle `oamDrawMeta` (sprite.c:335), il faudra déplacer ce corps avant de supprimer le nom. |
| `consoleInitEx` | `consoleInit` | argument ignored | oui (console.c:117) |
| `getRegion` | `isPAL` | same value | oui (console.c:200-206) |
| `rand` / `srand` | `rngNext` / `rngSeed` | not libc's | oui (console.c:227-233) |
| `COLORMATH_BG1..4`, `COLORMATH_OBJ` | `LAYER_*` | use LAYER_* | oui, valeurs `BIT(0..4)` = 0x01..0x10 (video.h:108-112) |
| `MOSAIC_BG1..4` | `LAYER_BG*` | | oui (0x01..0x08) |
| `WINDOW_BG1..4`, `WINDOW_OBJ` | `LAYER_*` | | oui |
| `BGMODE_MODE0/1/2/3/7` | `BG_MODE0/1/2/3/7` | | oui (mêmes valeurs, video.h:34-41) |

Le compte exact est donc 45 noms retirés et 2 noms redéfinis (`hdmaEnable`, `hdmaDisable`).

## Promesses de doc que le code ne tient pas

| page:ligne | promesse | code | sévérité |
|---|---|---|---|
| docs/GETTING_STARTED.md:10 et 23 | Parcours A : « You only need `make` » | Python 3 est requis à chaque build : `ROMSIZE` est calculé par python3 (make/common.mk:126), et `symmap.py`, `check_bank_reads.py`, etc. tournent après chaque édition de liens (lignes 603-670). `opensnes doctor` ne vérifie pas la présence de python (aucune occurrence dans bin/opensnes). | rouge |
| lib/include/snes/types.h:310 | « the clang pre-pass that every C file goes through reports each use » | La passe clang est sautée en silence si clang est absent (`if command -v clang`, common.mk:476). Une installation du parcours A (sans clang) ne voit aucun avertissement de dépréciation. À la 1.0, un `hdmaEnable(0x40)` deviendra alors « canal 64 » sans aucun signal. | rouge (pour la 1.0) |
| docs/GETTING_STARTED.md:89 | « You should see "Hello World!" » | examples/text/print_string/main.c:47 affiche `"TEXT MODULE TEST"`, et `Hello` n'apparaît nulle part dans l'exemple | orange |
| docs/GETTING_STARTED.md:106, README.md:144 | `opensnes run` « launches your emulator » | `find_emulator` ne cherche que dans le PATH (bin/opensnes:118). Or `install-luna.sh` pose `luna-gui` dans `tools/luna-test/bin/`, hors du PATH, et le message d'échec propose « Mesen, bsnes, or snes9x » sans citer luna-gui (ligne 406) | orange |
| lib/include/snes/hdma.h:318 | « HDMA will start on the next frame » | `hdmaEnableMask` écrit `$420C` tout de suite (hdma.asm:368). Les arbitres disent qu'activer un canal après la ligne 0 sans initialiser les registres d'état donne une image corrompue (snesdev-wiki, chunks `5a727497a9a02ed1` et `b5e8199cc9d6badd`). anomie-regs `eabf5855cb52cd3b` précise que l'adresse et le compteur doivent alors être initialisés à la main. | orange |
| docs/TROUBLESHOOTING.md:156 | « The NMI handler calls oamUpdate() automatically » | Le transfert n'a lieu que si `oam_update_flag` est levé (crt0.asm:2191). Une écriture directe dans `oamMemory[]` n'est donc jamais envoyée, ce que crt0.asm:1489 dit mais pas la page. | jaune |
| docs/README.md:147-148, 172 | input.h « pad, mouse, Super Scope … undocumented », interrupt.h « partial », puis « Every public header is now covered » | input.md cite 23 fonctions pad, mouse et scope. interrupts.md (102 lignes) existe mais n'est pas dans la carte. `tile.h` est absent de la carte. | jaune |
| KNOWN_LIMITATIONS.md:206 | `data_init_end.o` … « (enforced) » | L'ordre ne tient qu'à la construction de `LINK_OBJS +=` (common.mk:568). Aucun contrôle ne le vérifie. | jaune |
| docs/tutorials/framework.md:309, object.md:533 | `SEMISUPERFREE BANKS 7-1` | La plage vaut `ROM_BANKS-1` à 1 depuis le 2026-09-24 (templates/assets.inc:74) | jaune |
| KNOWN_LIMITATIONS.md:38, docs/PERF.md:9 | VBlank « ~50,500 » mclk, puis « about 51,800 » | Deux chiffres différents d'une page à l'autre. L'arbitre donne 37 × 1324 = 48 988 mclk utiles (snesdev-wiki `c0c7157e69e13ebf`). | jaune |
| CLAUDE.md:68 | « examples/ — 54 ROMs … (text, graphics, …) » | Il y en a 89, et la catégorie `graphics/` n'existe pas. Le sentinel ne lit pas CLAUDE.md. | jaune |
| lib/source/hdma.c:460 (commentaire) | « HDMA initialize happens at start of VBlank » | anomie-timing `63d5b98d1d3e494f` : initialisation vers V=0 H=6, et canaux désactivés au début du VBlank → aspect bibliothèque | jaune |

## Périmètre couvert

- Lu : GETTING_STARTED (suivi à la lettre), docs/README.md (index et carte), en-têtes dépréciés et leurs implémentations, KNOWN_LIMITATIONS, HARDWARE_VERIFICATION, MIGRATING, PERF, parties de TROUBLESHOOTING, hdma.md, scrolling.md et text.md, bin/opensnes, cibles release et docs du Makefile, make/common.mk.
- Lancé : `make lint-docs` (OK, 0.47.0, 89 exemples), `make docs-strict` (sortie 0, 1085 pages HTML), `check_doc_render.py` (OK), `make -C examples/text/print_string` (OK), `luna --version` (1.31.0).
- Scripts : couverture des fonctions publiques dans docs/ et examples/, README d'exemples (capture et Modules), sévérités de KNOWN_LIMITATIONS, mots français, chemins `.claude/`.
- Corpus : 4 appels `snes_verify` avec `exclude_sources=["opensnes-docs","opensnes-notes-tech"]` (débit DMA, VBlank NTSC, effet d'une écriture de scroll, moment d'initialisation HDMA). Je n'ai lu que les sentences, jamais le verdict.
- Non fait : lecture intégrale des 28 tutoriels et des 89 README, revue manuelle des pages orphelines du site.

## Points forts

- Le sentinel est vert : `make lint-docs` donne « OK: no doc drift detected », et `docs-strict` comme `check_doc_render` passent.
- La couverture de l'API est presque totale : 326 des 328 fonctions publiques non dépréciées sont citées dans docs/. Les deux exceptions sont `gsuFrameBytes`, citée nulle part (superfx.h:341, commit 556404fb), et `gsuPresentWait`, citée seulement dans les exemples.
- Les 89 README d'exemples ont une section « Modules ». 80 ont leur capture. Les 9 qui n'en ont pas sont dans examples/audio/, et la règle (new_example.md:91-93) les exempte.
- Les noms dépréciés sont toujours cités comme tels : les 8 mentions en forme d'appel dans docs/ et examples/ portent toutes « Deprecated » (dma.md:222, 226 ; audio.md:257, 431 ; hdma.md:59, 356).
- Les 47 dépréciations disent vrai. Les remplaçants existent, les constantes ont des valeurs identiques, et les alias asm partagent l'adresse.
- Les deux entrées rouges de KNOWN_LIMITATIONS ont une mitigation (lignes 33 et 46). Leurs chiffres de base sont confirmés par une sentence d'arbitre : 8 mclk par octet (anomie-regs `9e58fa5d6dd2213f`), 37 lignes de VBlank (snesdev-wiki `c0c7157e69e13ebf`).
- MIGRATING : toutes les fonctions de la colonne OpenSNES existent (vérifié par script, puisque le sentinel exempte cette page à check_doc_drift.py:758).

## Points faibles

1. **Rouge — dépendance Python cachée dans le parcours A.** Preuve : common.mk:126 et 603. Conséquence : sous MSYS2 avec `pacman -S make` seul, comme la page le prescrit, la première édition de liens échoue.
2. **Rouge — les dépréciations n'atteignent pas les utilisateurs sans clang**, et le changement de sens de `hdmaEnable` n'est annoncé que dans CHANGELOG.md:185-191 et l'en-tête. Aucune page publique (ROADMAP, FAQ, README) ne mentionne une politique de dépréciation : 0 occurrence de « deprecat » dans ces fichiers. `docs/UPGRADING.md` n'existe pas. Conséquence : le passage à la 1.0 cassera en silence le code qui passe encore un masque.
3. **Orange — MIGRATING_FROM_PVSNESLIB ignore les renommages.** `rngNext`, `lzssDecodeVram`, `isPAL`, `oamDrawMetasprite` et `hdmaEnableMask` y ont 0 occurrence, alors que `rand()` et `LzssDecodeVram` sont des noms PVSnesLib. Un portage garde donc les noms qui disparaîtront à la 1.0.
4. **Orange — le premier contrôle du débutant échoue** (« Hello World! ») et `opensnes run` ne trouve pas l'émulateur que la page vient de faire installer. Les preuves sont dans le tableau ci-dessus.
5. **Orange — `hdmaEnableMask` promet un départ à l'image suivante mais agit tout de suite.** Preuve : tableau ci-dessus. Conséquence : une image corrompue si l'appel tombe en plein écran.
6. **Jaune — aucune image dans les 28 tutoriels** (et non 27). Même mode7.md, colormath.md, window.md et mosaic.md n'ont aucune illustration. Un seul tutoriel, mode7.md, contient un avant/après.
7. **Jaune — vocabulaire interne dans les docs publiques.** « chantier » apparaît 13 fois dans KNOWN_LIMITATIONS.md et 11 fois dans ROADMAP.md (il a été retiré de docs/ seulement). Il reste aussi 11 lignes de docs/ qui renvoient à `.claude/…` (dma.md:363 et 365, dsp1.md:230, luna.md:7, …).
8. **Jaune — trous sur le site Doxygen.** docs/Doxyfile:25-48 n'inclut ni docs/README.md (qui porte la carte en-tête → tutoriel), ni PHILOSOPHY.md, ni BENCHMARK.md, ni ROADMAP.md, alors que PERF.md renvoie vers BENCHMARK.
9. **Jaune — KNOWN_LIMITATIONS compte 38 entrées** : 2 rouges, 1 orange, 10 jaunes, 25 vertes. Parmi les vertes, une n'a pas de date (data_init_end, ligne 206). Huit ne nomment aucun test qui les vérifie (lignes 108, 206, 288, 314, 403, 433, 595 et 751) ; pour la ligne 288, le seul filet est l'auto-test de crt0 au démarrage.
10. **Jaune — HARDWARE_VERIFICATION est complet mais vide de résultats** : « No session recorded yet » (ligne 158), alors que c'est un indispensable de la 1.0.
11. **Jaune — une affirmation matérielle non arbitrée.** scrolling.md:262 dit que les écritures de scroll prennent effet « from the next scanline ». Aucune sentence d'arbitre ne l'énonce : les cinq passages renvoyés ne parlent que de write-twice. Il faut la reformuler en observation ou citer une source.

## Risques

- Si la 1.0 redéfinit `hdmaEnable(channel)` en une seule étape, un appel `hdmaEnable(1<<6)` compile toujours, n'avertit pas sans clang, et vise le canal 64. Sauf contrôle de plage, il écrit hors de $43x0.
- `docs/PERF.md` date du 2026-09-26 (v0.45) et aucun garde ne le vérifie, contrairement à BENCHMARK (anchor 13). Il va dériver à chaque changement de lib.
- La carte en-tête → tutoriel se dit « générée » (docs/README.md:131) mais n'est vérifiée par aucun script. Elle est déjà fausse sur trois lignes.
- CLAUDE.md sort du périmètre du sentinel alors que les agents le lisent en premier.

## Améliorations recommandées

| # | action | sévérité traitée | effort | premier pas concret |
|---|---|---|---|---|
| 1 | Déclarer python3 (et clang, conseillé) dans le parcours A, et les faire vérifier par `opensnes doctor` | rouge 1 | S | ajouter `python3` à GETTING_STARTED.md:10, 23 et 44 (`pacman -S make python`) et un `check_cmd python3` dans bin/opensnes |
| 2 | Avertir des dépréciations sans clang : un scan grep ou python des noms `OPENSNES_DEPRECATED` lancé par common.mk quand clang manque | rouge 2 | S | générer la liste depuis les en-têtes et la passer à un `grep -wn` sur `$(CSRC)` |
| 3 | `hdmaEnable` en deux temps : retiré en 1.0, réintroduit en 1.1 avec un `channel < 8` vérifié (assert) | rouge 2 | S | inscrire la décision dans ROADMAP et dans l'en-tête |
| 4 | Écrire `docs/UPGRADING.md` à partir du premier tableau, avec les deux nuances (`padRaw`/$FFFF, `nmiSetBank(NULL)`) et l'ajouter au Doxyfile | rouge 2, orange 3 | S | reprendre le tableau tel quel |
| 5 | Ajouter une section « Renamed in OpenSNES » à MIGRATING | orange 3 | S | `rand`→`rngNext`, `LzssDecodeVram`→`lzssDecodeVram`, `getRegion`→`isPAL` |
| 6 | Corriger GETTING_STARTED:89 et faire chercher `tools/luna-test/bin/luna-gui` par `find_emulator` | orange 4 | S | une ligne de doc et deux lignes de shell |
| 7 | Faire écrire `$420C` par le NMI (via un shadow) ou corriger hdma.h:318 | orange 5 | S/M | sinon, a minima, écrire « call during VBlank » dans hdma.h |
| 8 | Ajouter au sentinel la carte en-tête → tutoriel et le compte d'exemples de CLAUDE.md | jaune 7, 9 | S | ajouter un `check_header_map` dans check_doc_drift.py |
| 9 | Retirer « chantier » et les chemins `.claude/` de KNOWN_LIMITATIONS, ROADMAP et docs/ | jaune 7 | S | lancer `grep -nw chantier` puis réécrire |
| 10 | Une capture par tutoriel visuel (mode7, colormath, window, mosaic, hdma) | jaune 6 | M | réutiliser les PNG des exemples déjà référencés |

## Verdict

La documentation est exacte sur ce qu'elle vérifie, et elle en vérifie beaucoup (13 points d'ancrage, couverture d'API à 99 %, README complets, Doxygen strict vert). L'écart avec une 1.0 est l'accueil et la migration. Le parcours A promet « seulement make » alors qu'il faut Python, son premier contrôle échoue et `opensnes run` ne trouve pas luna. Le retrait des 45 noms et le changement de sens de `hdmaEnable` n'ont encore ni guide public ni signal pour qui compile sans clang. C'est l'essentiel du travail à faire avant la 1.0 : environ une semaine, pas un chantier de fond.

## Suivi 2026-10-05 (session)

- **Rec 8** : `check_header_map` (ancre 15) dans `check_doc_drift.py` —
  chaque en-tête public a sa ligne, chaque en-tête et chaque tutoriel
  nommés existent ; contrôle négatif (`animx.h` / `animatio.md`) → 3
  dérives. La table était déjà cohérente ; sa phrase « generated » a été
  remplacée par ce que le sentinel vérifie. `CLAUDE.md` entre dans les
  fichiers du compte d'exemples (motif « N ROMs organized »).
- **Jaune 8** (Doxygen) : `PHILOSOPHY.md` et `BENCHMARK.md` étaient entrés
  le 10-04 ; `PHILOSOPHY.md` a tenu le job doc-render au rouge jusqu'au
  10-05 (code dans une citation), corrigé.
- **PF9 (entrées vertes sans test ni date)** : les sept entrées (108, 206,
  288, 314, 403, 433, 595) nomment leur test ; l'entrée `data_init_end`
  est réécrite d'après les faits : l'ordre d'édition de liens ne décide
  pas de la place du terminateur (tri de wlalink par taille), et un
  contrôle `symmap.py --check-data-init` existe désormais après chaque
  édition de liens (contrôle négatif sur un `.sym` forgé). La ligne 751
  n'est pas une entrée (paragraphe de la section ABI) : rien à dater.
