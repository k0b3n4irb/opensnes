Expert 13 — Licences, provenance et conformité · 2026-10-17 · HEAD audité `develop` @ `73a3fdfe` (worktree `/home/kobenairb/workspace/opensnes-audit`)

> Je ne suis pas avocat. Ce rapport décrit des risques et des écarts entre ce que le dépôt livre et ce que les licences demandent. Ce n'est pas un avis juridique. Avant une 1.0 « commercial-grade », la question des assets et des marques mérite une relecture par un juriste.

## Périmètre couvert

- **Licences des composants livrés.** Lu : `LICENSE`, `ATTRIBUTION.md` (247 l.), `docs/THIRD_PARTY.md` (103 l.), `tools/third_party/README.md` et les `LICENSE` des trois sous-modules (`compiler/cproc` ISC, `compiler/qbe` MIT, `compiler/wla-dx` **GPL-2.0-or-later**). Lu aussi les en-têtes de `tools/lib/{gfx4snes,smconv,tmx2snes}/src`, de `tools/third_party/*` et de 60 sources de `lib/source`, puis `templates/crt0.asm`, `templates/hdr.asm`, `lib/source/runtime.asm` et `lib/source/sm_spc.asm`.
- **Le zip.** J'ai lu la recette `release` (`Makefile:628-686`), `release-examples` (`Makefile:690-699`) et la matrice de `.github/workflows/release.yml:41-54`. `file bin/*` donne 19 binaires dans `bin/`, **tous liés statiquement**.
- **Ce qui finit dans la ROM d'un jeu fermé.** La lib, crt0, le runtime, le pilote SNESMOD et l'en-tête de ROM.
- **Provenance des assets.** J'ai comparé les md5 de tous les fichiers non-source suivis de `examples/` et `starter/` à un clone local de PVSnesLib (`~/workspace/pvsneslib`, `fa758c9b`, 2025-12-28, 3649 fichiers). J'ai lu les chaînes des sept modules `.it` suivis et les README d'exemples qui citent un auteur tiers (krom, Kenney, PVSnesLib).
- **Marques.** J'ai cherché « Nintendo », « SNES », « Mario », « Tetris » et « 1942 » dans les noms, les `ROM_NAME`, le README, la page d'accueil et `docs/`.
- **Le suivi.** `2026-10-04_reste_a_traiter.md` (F PF1, H « risque GPL », la décision du propriétaire du 10-05) et `H_governance.md` §16-17 et §101.
- **Non couvert.**
  - Le texte actuel de la licence de `mukunda-/snesmod`. Je n'avais pas de réseau : l'affirmation « MIT » d'`ATTRIBUTION.md:44` n'a pas été revérifiée aujourd'hui.
  - Les dépôts de luna et des forks sur GitHub (leur visibilité publique n'a pas été vérifiée).
  - Aucune affirmation matérielle n'est faite ici, donc il n'y a pas eu de requête cartouche.

## Points forts

- **Rien de copyleft n'entre dans la ROM d'un utilisateur.**
  - La lib, `templates/` et le runtime sont MIT (OpenSNES, Alekmaul) ou zlib (dérivés PVSnesLib).
  - Le runtime (`lib/source/runtime.asm:16-17`, « License: MIT ») ne reprend pas `libtcc.asm` ni `libm.asm` de snes-sdk/PVSnesLib.
  - wla-dx (GPL) est un programme séparé : sa sortie n'est pas une œuvre dérivée de son code. Pour un jeu fermé, c'est la propriété la plus importante, et elle tient.
- **L'en-tête de ROM ne revendique pas Nintendo.** `templates/hdr.asm:58` contient `LICENSEECODE $00 ; Unlicensed`.
- **Le firmware DSP-1 n'est pas livré.** `docs/tutorials/dsp1.md:193-195` le dit « copyrighted, not shipped ». `git ls-files` ne contient aucun `dsp1*.bin` ni `*.rom`. Les tests DSP-1 sont ignorés en CI sans le firmware.
- **La GPL de wla-dx a été traitée depuis le 03/10.** Le zip contient `LICENSE-wla-dx` et `TOOLCHAIN-SOURCES.md`, qui donne l'URL du fork et le SHA (`Makefile:673-677`).
- **cmdparser (Apache-2.0).** Son texte et sa notice sont livrés (`Makefile:671-672`).
- **luna (MPL-2.0) n'est pas dans le zip.** Seul l'installateur y est (`scripts/install-luna.sh`). La MPL ne touche pas les ROM qu'il exécute.
- **Plusieurs provenances sont documentées honnêtement.**
  - Kenney CC0 pour `shmup_1942`.
  - Le violoncelle de krom, avec la réserve « revisit before any commercial redistribution » (`ATTRIBUTION.md:207-212`).
  - Les phonèmes, `starter/res/player.png`, `hicolor_1792` et `mode5_hires` sont déclarés originaux.
  - modlib (MIT), dont le texte complet est reproduit.
- **La page d'accueil a une mention de marque correcte** (`docs/landing/index.html:563-566` : « not affiliated with, sponsored by, or endorsed by Nintendo… »).
- **Les sources « Nintendo dev manual » sont seulement citées.** La documentation renvoie à un numéro de section (`docs/hardware/REGISTERS.md:602`, `APU.md:67`, `tutorials/superfx_advanced.md:13,46,87`). Je n'ai trouvé aucune reproduction verbatim.

## Points faibles

**13-1 🔴 Assets PVSnesLib et personnages Nintendo livrés sans attribution, et `ROADMAP.md` affirme le contraire.** Effort M (attribuer) / L (refaire).

- **Ce qui est dans l'arbre.**
  - Mesure du jour : **56 fichiers dans 19 exemples ont un md5 identique** à un fichier de PVSnesLib.
  - La ROADMAP compte 66 fichiers dans 23 exemples, en incluant les dérivés.
  - En plus, il y a des dérivés non identiques mais nommés Mario : `games/likemario/res/mario_sprite.png` et `maps/slope_collision/res/mario_sprite.png`.
- **Les personnages et éléments Nintendo.**
  - `games/mapandobjects/res/{mario,goomba,koopatroopa,tilesMario}.png` et `tiledMario.{b16,o16,t16}`.
  - `maps/map_scroll/res/{mario,tilesMario}.png` et `tiledMario.*`.
  - `games/likemario/res/{tiles.png,mariojump.brr,overworld.it}`.
  - `maps/slope_collision/res/tiles.png`.
- **Le reste des fichiers PVSnesLib.** `breakout` (8), `mode0` (4), `transparency`, `mode3`, `perspective`, `dsp1_ground`, `window`, `transparent_window`, `parallax_scroll`, `superscope`, `aseprite_pipeline`, `dynamic_sprite`, `dynamic_metasprite`, et `snesmod_music`/`snesmod_sfx` (modules `.it`).
- **Ce qu'en disent les documents.**
  - `grep -ciE 'nintendo|mario|goomba|koopa' ATTRIBUTION.md` donne **0**.
  - Pourtant `ROADMAP.md:53` affirme : « they are listed in `ATTRIBUTION.md` until then ». C'est faux.
  - `docs/ASSET_PROVENANCE.md` n'existe pas.
- **Où cela part.**
  - `release-examples` (`Makefile:690-699`) met ces sources **et les ROM construites** dans `opensnes-examples_<v>.zip`, sous `LICENSE` MIT.
  - Elles sont aussi sur le dépôt public.
- **Pourquoi c'est 🔴.**
  - `ROADMAP.md:53` place « Asset provenance » dans les **Must-have** de la porte 1.0 (« Every asset redone … before a 1.0 release candidate »).
  - La note du 10-05 (« mis de côté, pas une porte de la 1.0 ») contredit la ROADMAP actuelle. Il faut trancher par écrit.
  - Note : l'arbre principal montre aujourd'hui des suppressions non commitées de `examples/maps/map_scroll/res/tiledMario.{b16,o16,t16}`. Un travail semble commencé hors de HEAD. Il n'est pas audité ici.

**13-2 🟠 Modules musicaux tiers sans licence ni attribution.** Effort S (retirer ou remplacer) / M (recomposer).

- **`examples/games/tetris/music/tetris_theme.it`.**
  - Ses chaînes donnent « TETRIS NOW! », « @CheapAlert », « @9-25-05 », « @nintendo rulez :D ».
  - C'est un module tiers de 2005, d'auteur identifié, sans licence connue.
  - Il est absent d'`ATTRIBUTION.md` et livré dans le zip d'exemples et dans la ROM `tetris`.
- **`games/likemario/res/overworld.it`** (PVSnesLib). Le titre et l'exemple laissent penser à une reprise du thème de Super Mario Bros. C'est probable, pas vérifié à l'écoute. Si c'est le cas, la composition appartient à Nintendo.
- **`audio/snesmod_sfx/sfx/effectssfx.it`** (PVSnesLib). Il contient les échantillons « tada », « Hall Strings », « Honky Tonk Piano » et « Marimba 1 ». Les noms suggèrent des banques General MIDI et le son Windows. La provenance n'est pas documentée.
- **`testing/fixtures/retired_examples/snesmod_music_large/music/whatislove.it`.** Une reprise de « What Is Love » (« By Jogeir Liljedahl ») : la composition appartient à des tiers. Le fichier est dans le dépôt public mais pas dans les zips.
- **`pollen8.it`** (« intro song by coda », exemple SNESMOD). Il est dupliqué dans 3 dossiers et 3 crashs de fuzz. Sa licence suit celle, non vérifiée, de la distribution SNESMOD.

**13-3 🟠 Marques de tiers dans les noms d'exemples et de ROM.** Effort S.

- **Tetris.**
  - L'exemple s'appelle `examples/games/tetris` et sa ROM `ROM_NAME := OPENSNES TETRIS`. Le README dit « Classic Tetris game ».
  - « Tetris » est une marque de The Tetris Company, qui défend aussi l'apparence du jeu (affaire Tetris Holding c. Xio, 2012).
- **Mario.** `ROM_NAME` vaut `OPENSNES LIKEMARIO` et `OPENSNES SLOPMARIO`, et le README s'intitule « Slope Mario ».
- **1942.** `ROM_NAME := SHMUP 1942` reprend un titre Capcom ; c'est le cas le plus léger.
- **La correction.** Les noms descriptifs (« falling blocks », « platformer », « vertical shmup ») coûtent un renommage.

**13-4 🟠 Les zips Linux embarquent la glibc liée statiquement sans rien de ce que la LGPL demande.** Effort S-M.

- **Le constat.** `file bin/*` donne « statically linked » pour les 19 binaires. `strings bin/cc65816` contient « Fatal glibc error… ».
- **D'où ça vient.** `compiler/Makefile:41` (`STATIC_LDFLAGS := -static` hors Darwin) et `tools/tool.mk:38`. `release.yml:43-48` construit les zips Linux sur `ubuntu-latest` et `ubuntu-24.04-arm`, avec la glibc de la distribution.
- **Ce que demande la LGPL.** La glibc est LGPL-2.1-or-later. En liaison statique, la LGPL demande :
  - une mention que la bibliothèque est utilisée,
  - son texte,
  - et un moyen de relier le programme à une version modifiée : les objets, ou une offre écrite.
- **Ce que livre le zip :** rien de tout cela.
- **Qui est touché.** Le distributeur du SDK (le zip), pas les jeux.
- **Les voies.** Lier statiquement contre musl (MIT) dans un conteneur Alpine, ou lier la glibc dynamiquement. À défaut, livrer la notice LGPL et les `.o` de chaque outil.

**13-5 🟠 Les notices MIT/ISC de QBE et cproc ne sont pas livrées avec les binaires qui les contiennent.** Effort S.

- `bin/qbe`, `bin/cproc-qbe` et `bin/cc65816` sont des copies de QBE (« © 2015-2025 Quentin Carbonneaux », MIT) et de cproc (« Copyright © 2019-2024 Michael Forney », ISC).
- Ces deux licences demandent la notice « in all copies ». Or `Makefile:668-677` ne copie que `LICENSE`, `ATTRIBUTION.md`, `LICENSE-cmdparser` et `LICENSE-wla-dx`.
- `ATTRIBUTION.md:27-38` nomme ces licences sans en donner le texte ni le copyright.
- Même manque pour lodepng (zlib : pas d'obligation sur un binaire, simple courtoisie) et pour le code Alekmaul lié dans `opensnes-tileset`/`-sprite` : son MIT est couvert par `LICENSE`.
- Déjà signalé côté build par 12-12. Je le classe ici comme écart de licence.

**13-6 🟠 Aucune page ne dit au développeur d'un jeu fermé ce que sa ROM contient et quelle notice il doit porter.** Effort S (doc) / M (licence d'exécution).

- **Ce que contient une ROM.**
  - La lib et `templates/crt0.asm` : MIT OpenSNES et MIT/zlib Alekmaul.
  - Le moteur de carte : « Original: undisbeliever's castle_platformer » (`lib/source/map.asm`), absent d'`ATTRIBUTION.md`.
  - Le pilote SNESMOD : 5 522 octets de Mukunda Johnson (`lib/source/sm_spc.asm`, sans texte de licence). PVSnesLib distribuait son source sous notice zlib, `ATTRIBUTION.md` le dit MIT.
- **Ce qui en découle.** Le MIT exige la notice dans « all copies or substantial portions ». Un studio qui suit le SDK à la lettre doit donc mettre au moins deux notices MIT dans les crédits ou le manuel de son jeu. Rien ne le lui dit :
  - `grep -niE 'licen|commercial|notice' docs/FAQ.md docs/GETTING_STARTED.md` ne donne rien d'utile ;
  - `starter/README.md:74-77` dit seulement « MIT ».
- **Pourquoi c'est bloquant.** Pour une 1.0 qui vise « commercial-grade maturity » (`ROADMAP.md:22-23`), c'est la première question d'un studio.
- **Les voies.**
  1. Une page « Licensing your game » qui liste ce qui entre dans la ROM et les notices à porter.
  2. Ou une exception d'exécution, comme celle de libgcc : la lib, `templates/` et `starter/` passeraient sous zlib ou 0BSD pour le code OpenSNES, sans notice exigée dans un binaire. Les parties Alekmaul restent sous leur licence.

**13-7 🟠 Notices zlib retirées des sources dérivées de PVSnesLib.** Effort S.

- **La règle.** 29 fichiers de `pvsneslib/source` et `include/snes` portent la notice zlib : « 3. This notice may not be removed or altered from any source distribution ».
- **Ce qu'il en reste.** Dans `lib/`, seuls `lib/source/lzss.asm` et `lib/source/snesmod.asm` la gardent.
- **Ce qu'elle est devenue ailleurs.**
  - `lib/source/console.c` dit « Based on PVSnesLib console.c … License: zlib (compatible with MIT) ».
  - `lib/source/map.asm` et `lib/contrib/object.asm` disent la même chose.
  - `lib/source/sprite_dynamic.asm` dit « License: MIT (original code zlib by Alekmaul) ».
  - Partout, sans le texte.
- **Pourquoi c'est un écart.** C'est un manquement à la clause 3 pour toute distribution de source : le dépôt, et le zip d'exemples si un jour il contient la lib. La correction est mécanique : un `LICENSE-pvsneslib` avec la notice, cité dans chaque en-tête dérivé.
- **Côté outils.** `tools/lib/gfx4snes/src/*.c` et `tools/lib/tmx2snes/src/level.c` gardent la notice.
- **smconv.** Les sources de `tools/lib/smconv/src/*.c` n'ont **aucun** en-tête, alors que le code est de Mukunda Johnson (« (c) 2009 », via PVSnesLib).

**13-8 🟠 krom (PeterLemon/SNES) : 13 exemples et une routine de la lib s'appuient sur un dépôt sans licence ; 4 sont attribués.** Effort S (demander) / M (remplacer).

- **Les exemples.** `grep -rli 'krom\|peterlemon' examples` donne :
  - audio/{apu_switch, pitch_mod, play_noise, soundboard, speech_synth} ;
  - backgrounds/mode5_hires, color/hicolor_1792 ;
  - hdma/{gradient_colors, hdma_indirect_gradient, hdma_wave} ;
  - mode7/perspective_rotate, windows/window_multi_hdma.
- **Ce que dit `ATTRIBUTION.md`.** Il n'en couvre que 4 (`hdma_wave`, `pitch_mod`, `soundboard`, `apu_switch`). Il reconnaît lui-même : « krom's repository carries no explicit license » (`ATTRIBUTION.md:210`).
- **Les données reprises telles quelles.**
  - `hdma_wave/res/wavetable.bin`, « extracted verbatim ».
  - La table « krom's exact 14-band data » de `window_multi_hdma`.
  - `cello.brr` en trois copies.
- **La lib.** `lib/source/apu.asm:14` dit « Protocol reference: krom's SNES_SPC700.INC, reimplemented ». C'est un protocole réécrit, risque faible, mais ce code entre dans les ROM.
- **Le risque.** Sans licence, tous les droits sont réservés. Un portage ligne à ligne est une œuvre dérivée.
- **La voie la plus simple.** Une permission écrite de krom, ou une licence posée sur son dépôt. Sinon, remplacer les données verbatim.

**13-9 🟡 Accès aux sources GPL de wla-dx : seulement un pointeur vers une branche de fork.** Effort S.

- `TOOLCHAIN-SOURCES.md` (copie de `compiler/PINS.md`) renvoie à `github.com/k0b3n4irb/wla-dx:opensnes/ram-labels-ignore-base`, au SHA `7382e4c8`.
- Les archives « Source code » d'une release GitHub n'incluent pas les sous-modules, et le zip ne contient aucune offre écrite.
- La conformité au §3 de la GPL-2.0 dépend donc du maintien de ce fork et de cette branche. Les branches cproc et qbe s'appellent `feat/b2-far-qualifier` : un nom de travail, qui peut être supprimé.
- La voie sûre : joindre `wla-dx-<sha>.tar.gz` comme asset de chaque release, une ligne dans `release.yml`.

**13-10 🟡 `ATTRIBUTION.md` et `THIRD_PARTY.md` sont incohérents et périmés.** Effort S.

- **Licence de gfx4snes.** `ATTRIBUTION.md:121-123` dit zlib, `docs/THIRD_PARTY.md:17` dit MIT. `tools/lib/gfx4snes/LICENSE` est MIT, mais ses sources portent la notice zlib.
- **Auteur de smconv.** `THIRD_PARTY.md:56-62` attribue smconv à Alekmaul ; `tools/lib/smconv/README.md` dit Mukunda Johnson.
- **Outils retirés.** `ATTRIBUTION.md:91,99,107,116` cite `img2snes`, `font2snes`, `tmx2snes` et `gfx4snes` comme outils utilisateurs.
- **Dépendances qui ne sont plus liées.**
  - cmdparser n'est lié par **aucun** outil livré (`grep -rl cmdparser tools/*/Makefile` ne donne rien), mais `LICENSE-cmdparser` est livré. C'est inoffensif.
  - stb_image n'est compilé que par `tools/fuzz` (→ 07-5).
- **Contributeurs non crédités :** KungFuFurby (portage WLA de SNESMOD, `snesmod.asm`), undisbeliever (moteur de carte) et luna (MPL-2.0).
- **Ce qui manque à `THIRD_PARTY.md` :** lodepng, cute_tiled, stb et modlib.

**13-11 🟡 La mention de marque n'existe que sur la page d'accueil.** Effort S.

- `README.md:7` dit « SDK for Super Nintendo development ». Le nom du projet contient « SNES ».
- La mention « not affiliated with … Nintendo » (`docs/landing/index.html:563-566`) n'est ni dans `README.md`, ni dans `LICENSE`, ni dans le zip.
- La même page affiche « v0.35.0 · MIT » (`:555`), une version périmée (→ expert docs).

**13-12 🟡 `CONTRIBUTING.md` ne dit pas sous quelle licence entre une contribution.** Effort S.

- Il n'y a ni DCO ni phrase « inbound = outbound » (`grep -niE 'licen|DCO|sign-off' CONTRIBUTING.md` ne donne que la ligne 192, qui concerne le code externe).
- `LICENSE` dit « OpenSNES Contributors ». En cas de futur changement de licence (13-6, option 2), l'accord des contributeurs ne serait pas établi.

**13-13 🟡 Un corpus issu de manuels Nintendo confidentiels sert de source.** Effort —, vigilance.

- La documentation cite le « Nintendo dev manual Book I/II » par section. C'est une citation, pas une reproduction : aucun extrait verbatim trouvé.
- Il faut s'y tenir. Une table copiée d'un manuel fuité serait un autre risque.

→ expert 06 : des notes publiées décrivaient un projet tiers sous licence (06-9, corrigé le 2026-10-17 : retirées, historique réécrit). Cela relève de l'organisation et je ne le recompte pas.

## Risques

1. **Le zip d'exemples redistribue de la propriété Nintendo (graphismes, son, probablement une musique) sous bannière MIT, ROM construites comprises.** Une demande de retrait viserait le dépôt entier, pas un exemple. Une 1.0 annoncée amplifie la visibilité.
2. **Un studio qui livre un jeu sans les notices MIT est techniquement en défaut.** Personne ne le lui a dit (13-6). Ce risque retombe sur l'image du SDK.
3. **Une demande de conformité LGPL ou GPL sur le zip** : glibc statique (13-4), source wla-dx par simple pointeur (13-9). C'est peu probable mais facile à éviter.
4. **« Tetris » dans un nom de ROM distribuée** (13-3). Le titulaire est réputé actif.
5. **Le code krom sans licence.** krom est un contributeur bienveillant de la scène, mais sans licence, le dépôt n'a aucun droit écrit (13-8).

**Ordre pour lever ce qui bloque juridiquement une 1.0 :**

1. Trancher par écrit la contradiction entre la note du 10-05 et `ROADMAP.md:53`.
2. Retirer ou renommer `tetris`, `likemario`, `slope_collision`, `mapandobjects` et `map_scroll`, ou refaire leurs assets, puis retirer `tetris_theme.it`, `overworld.it` et `whatislove.it`.
3. Écrire la section « Third-party assets » d'`ATTRIBUTION.md` pour ce qui reste de PVSnesLib, et corriger `ROADMAP.md:53`.
4. Livrer les notices QBE et cproc, régler la glibc et joindre la source de wla-dx.
5. Rétablir les notices zlib (13-7) et écrire la page « Licensing your game » (13-6).
6. Obtenir la permission de krom.

## Améliorations recommandées

| # | Action | Constat | Effort | Premier pas concret |
|---|---|---|---|---|
| 1 | Décider et écrire si les assets Nintendo/PVSnesLib bloquent la 1.0 ; corriger `ROADMAP.md:53` (« listed in ATTRIBUTION.md » est faux) | 13-1 | S | Une ligne du propriétaire, puis l'édition de ROADMAP |
| 2 | Remplacer les assets à personnages Nintendo (mapandobjects, map_scroll, likemario, slope_collision), puis attribuer le reste fichier par fichier | 13-1 | M-L | Commiter la liste md5 (56 exacts, 19 exemples, script de comparaison au clone PVSnesLib) comme section « Third-party assets » |
| 3 | Retirer ou remplacer `tetris_theme.it`, `overworld.it`, `whatislove.it` ; documenter `effectssfx.it` et `pollen8.it` | 13-2 | S-M | `git rm testing/fixtures/retired_examples/snesmod_music_large/music/whatislove.it` ; un module original pour tetris |
| 4 | Renommer `tetris` et les `ROM_NAME` Mario/1942 | 13-3 | S | `examples/games/tetris` → `blocks`, `ROM_NAME := OPENSNES BLOCKS` |
| 5 | Binaires Linux : musl statique, ou glibc dynamique, ou notice LGPL + objets | 13-4 | S-M | Job release Linux dans `alpine` avec `clang` et `musl-dev` ; contrôle `strings bin/* \| grep -c glibc` = 0 |
| 6 | Livrer `LICENSE-qbe` et `LICENSE-cproc` (et lodepng par courtoisie) | 13-5 | S | `cp compiler/qbe/LICENSE …/LICENSE-qbe ; cp compiler/cproc/LICENSE …/LICENSE-cproc` près de `Makefile:676` |
| 7 | Page `docs/LICENSING.md` (« what lands in your ROM, which notices to carry »), avec un bloc de crédits prêt à coller ; envisager zlib ou 0BSD pour le code OpenSNES de `lib/`, `templates/` et `starter/` | 13-6, 13-12 | S / M | Lister les sections liées d'un starter (`.sym`) et leur licence |
| 8 | `LICENSE-pvsneslib` (notice zlib Alekmaul et mention de KungFuFurby, Mukunda Johnson et undisbeliever), cité dans chaque en-tête dérivé ; en-tête dans `tools/lib/smconv/src/*.c` | 13-7, 13-10 | S | `grep -rl -i 'pvsneslib\|alekmaul' lib templates tools/lib` (28 fichiers dans lib/templates) |
| 9 | Demander à krom une licence ou une permission écrite ; en attendant, attribuer les 13 exemples | 13-8 | S | Une entrée `ATTRIBUTION.md` par exemple ; message à krom |
| 10 | Joindre l'archive source de wla-dx (et des forks) à chaque release ; renommer les branches de pin en `opensnes/main` | 13-9 | S | `git -C compiler/wla-dx archive --prefix=wla-dx/ HEAD \| gzip` dans `release.yml` |
| 11 | Remettre `ATTRIBUTION.md` et `THIRD_PARTY.md` d'accord ; ajouter la mention de marque à `README.md` et au zip | 13-10, 13-11 | S | Ancre `check_doc_drift.py` : toute licence citée dans THIRD_PARTY = celle d'ATTRIBUTION |

## Verdict

Le cœur du SDK est sain pour un jeu commercial fermé. Tout ce qui entre dans une ROM est permissif (MIT, zlib), wla-dx n'est qu'un outil séparé, l'en-tête se déclare « Unlicensed » et le firmware DSP-1 n'est pas livré. En revanche, la distribution n'est pas prête pour une 1.0 : des personnages et sons Nintendo, un module « TETRIS NOW! » d'un tiers et des noms de marque partent sous bannière MIT sans attribution, et la ROADMAP affirme à tort qu'ils sont listés. Le zip omet aussi les notices de QBE, de cproc et de la glibc liée statiquement, et ne dit pas au studio quelle notice porter. Tout se lève en S ou M, à condition de trancher d'abord le sort des assets PVSnesLib/Nintendo, qui conditionne l'ordre du reste.

## Suivi de l'audit du 03/10

| Constat (2026-10-04_reste_a_traiter.md) | État au 17/10 | Preuve d'aujourd'hui |
|---|---|---|
| **F PF1** 🔴 Assets Nintendo sans attribution (40 fichiers, 23 exemples au 03/10) | **ouvert** | `grep -ciE 'nintendo\|mario\|goomba\|koopa' ATTRIBUTION.md` → 0 ; `docs/ASSET_PROVENANCE.md` absent ; 56 fichiers md5-identiques dans 19 exemples ce jour. Seul changement : `ROADMAP.md:53` le met en Must-have, mais affirme à tort une liste dans `ATTRIBUTION.md`. Décision du 10-05 (« pas une porte de la 1.0 ») contredite par la ROADMAP actuelle → 13-1 |
| **H « risque GPL »** (licence wla-dx absente du zip) | **partiel** | Corrigé : `LICENSE-wla-dx` et `TOOLCHAIN-SOURCES.md` livrés (`Makefile:673-677`). Reste : pas d'archive source ni d'offre écrite, et la disponibilité dépend d'une branche de fork (13-9). Nouveaux écarts du même ordre : glibc statique (13-4), notices QBE/cproc (13-5) |
| **H PF17** Blob SNESMOD (côté provenance seulement ; le fond relève de la gouvernance) | **ouvert** | `lib/source/sm_spc.asm` : « Pre-compiled SNESMOD SPC700 driver, Original by mukunda », 5 522 octets, sans texte de licence ni commit amont. Il entre dans chaque ROM qui lie `snesmod` → 13-6 |
