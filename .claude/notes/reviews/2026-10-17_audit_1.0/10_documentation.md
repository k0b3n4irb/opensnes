Expert documentation technique · 2026-10-17 · HEAD audité `develop` @ `73a3fdfe` (worktree `/home/kobenairb/workspace/opensnes-audit`)

# 10 — Documentation et parcours du nouveau venu

## Périmètre couvert

- Lu : `README.md`, `docs/GETTING_STARTED.md`, `docs/LEARNING_PATH.md`, `docs/README.md`, `docs/API_INDEX.md`, `docs/FAQ.md`, `docs/TROUBLESHOOTING.md`, `docs/MIGRATING_FROM_PVSNESLIB.md`, `docs/UPGRADING.md`, `docs/STABILITY.md`, `KNOWN_LIMITATIONS.md`, `ROADMAP.md`, et 31 tutoriels (9 812 lignes) par extraits ciblés.
- Parcours A rejoué dans `scratchpad/gs` contre le worktree. Projet « manual setup » compilé et capturé avec luna (« Hello SNES! » affiché). `opensnes init --template game`, puis `make` et `make test` : 2 PASS. `opensnes doctor` : tout OK.
- Lancé : `make lint-docs` (0 dérive), `make docs-strict` (0 avertissement), `check_doc_render.py` (OK), puis un second Doxygen avec `WARN_NO_PARAMDOC = YES` (0 avertissement). Deux scans du site généré : pages orphelines et liens cassés.
- Scripts ad hoc : 515 appels SDK des blocs ```c de `docs/` comparés à l'arité des en-têtes ; 15 prototypes cités comparés mot à mot ; couverture des 337 fonctions publiques dans `docs/`, `examples/` et `API_INDEX`.
- Exactitude : 24 affirmations vérifiées contre le code, la CLI de luna v1.38.0 ou le corpus Cartouche (sources `opensnes-*` exclues). Constats E du 03/10 repris un par un.

## Points forts

- **Le sentinel tient ses ancres.** `make lint-docs` répond « OK: no doc drift detected » : version canonique 0.50.0, 85 exemples, 22 fonctions `check_*` dans `devtools/check_doc_drift.py`. La carte en-tête → tutoriel est désormais vérifiée (`check_header_map`, l. 1354), et `CLAUDE.md` est entré dans le contrôle des comptes (l. 214-216).
- **Doxygen est propre, et pas seulement en surface.** `make docs-strict` sort avec 0 et ne produit aucun avertissement, avec `WARN_IF_UNDOCUMENTED = YES` (`docs/Doxyfile:278`). Relancé avec `WARN_NO_PARAMDOC = YES`, il ne signale toujours rien. `check_doc_render.py` répond « OK ». Sur les 889 pages HTML, aucune n'est orpheline.
- **Couverture API quasi totale.** 334 des 337 fonctions publiques sont citées dans `docs/`. Seules manquent `gsuFrameBytes`, `gsuPresentWait` et `hdmaWaveSetSpeed`.
- **Les extraits de code compilent dans la tête du lecteur.** Sur 515 appels SDK des blocs ```c de `docs/`, aucune arité n'est fausse : les 8 alertes du script sont des `...`, une virgule dans une chaîne ou l'ancien `dmaTransfer` cité exprès par `UPGRADING.md`. Les 15 prototypes recopiés dans les tutoriels sont identiques aux en-têtes.
- **Le parcours A fonctionne de bout en bout depuis un arbre construit.**
  - Le `main.c` de GETTING_STARTED:156-168 compile avec les modules indiqués (« 27532 bytes free »), et luna affiche « Hello SNES! » à la trame 60.
  - `opensnes init my-game --template game`, `make` puis `make test` donnent « 2 passed, 0 failed ».
  - `opensnes run` trouve `testing/bin/luna-gui` (`tools/opensnes/src/main.c:434`).
  - print_string affiche bien « TEXT MODULE TEST » en blanc sur bleu foncé, comme GETTING_STARTED:97 l'annonce (capture luna à `-n 3000000`).
- **Affirmations vérifiées exactes (échantillon de 24, dont 18 justes)** :
  - `textModeInit` (text.md:55-64), identique à `lib/source/text.c:310-320` ;
  - `M7VOFS $17F` (mode7.md:199), identique à `mode7.c:123` ;
  - `0x0100` = 1:1 et le pourcentage × 2,5 (mode7.md:173 et 205) ;
  - les valeurs `KEY_*` (input.md:26-43), identiques à `input.h:74-87` ;
  - le refus de `hdmaEnable` au-dessus de 7 (`hdma.c:25-28`) ;
  - « aucun exemple ne lie `profile` » (profiling.md:157), vrai ;
  - « save_game ne valide pas » (sram.md:272), vrai ;
  - `luna state … --screenshot` et `luna assets-dump … --out` (debugging.md:59 et 93) : exit 0 ;
  - les 39 en-têtes de README:103 ;
  - le message « OpenSNES SDK build complete! » (`Makefile:88`) ;
  - la mosaïque `8` → blocs 9×9 (mosaic.md:58 ; snesdev-wiki `572ccc0dcfbc302a` : « 0 = 1x1, ..., 15 = 16x16 ») ;
  - 32 sprites et 34 tranches par ligne (OAM.md:140 ; anomie-regs `2304edd2bf6755b9` : « determine the first 32 sprites … load up to 34 8x8 tiles ») ;
  - 8 mclk par octet de DMA (KL:47 ; anomie-timing `a8f6e03510109a8f` : « DMA takes 8 master cycles per byte transferred »).
- **Navigation cohérente.** Les 31 tutoriels sont liés depuis `docs/README.md`. `docs/` est entièrement en anglais (grep FR négatif). Toutes les entrées de KNOWN_LIMITATIONS (2 🔴, 1 🟠, 11 🟡, 10 🟢) ont une mitigation, et 9 🟢 sur 10 nomment un test ou une vérification de build.
- **README d'exemples complets.** Les 85 ont un README et les 77 non audio une capture qui existe sur disque (boucle `find … main.c`). L'ancre 9 garantit la section Modules.

## Points faibles

**10-1 🔴 — La première commande du parcours A échoue : la ROM n'est pas là où la page dit (S).** Preuves :
- GETTING_STARTED:88-91 demande `cd opensnes-examples_<version>/examples/text/print_string`, puis `luna-gui print_string.sfc`.
- Or la recette `release-examples` (`Makefile:695-696`) ne met dans l'archive que les fichiers suivis : `git ls-files examples | grep -c '\.sfc$'` → 0.
- Elle copie ensuite toutes les ROM à plat dans `examples/bin/`, ce que la même page dit trois lignes plus haut (l. 81).
- `devtools/release_smoke.py` n'ouvre pas l'archive d'exemples (grep `opensnes-examples` : 0 occurrence), donc rien ne l'attrape.

Conséquence : le débutant tombe sur « file not found » au premier contrôle. De plus, aucun texte ne lui dit qu'il faut `make OPENSNES=~/opensnes` pour reconstruire un exemple de l'archive, puisque `OPENSNES := $(shell cd ../../.. && pwd)` y pointe vers la racine de l'archive.

**10-2 🟠 — Le guide de migration PVSnesLib, une des conditions obligatoires de la 1.0, enseigne un appel inutile et coûteux, et un statut faux (S).** Preuves :
- MIGRATING:32 dit « `textPrintAt` + `textFlush` | Nothing appears without the flush », et MIGRATING:132 répète « The flush is what uploads the tilemap ».
- L'en-tête dit l'inverse (`text.h:200-213`) : chaque `text*` lève le drapeau, le NMI vide tout seul, et `textFlush` envoie « the whole buffer (2048 bytes, 16,384 master cycles of the VBlank) ». Le programme de GETTING_STARTED n'appelle pas `textFlush` et s'affiche (capture ci-dessus).
- `API_INDEX.md:21` recommande aussi `textFlush` pour « print text », et ne cite pas `textModeInit`.
- MIGRATING:142 dit de `LzssDecodeVram` « the old spelling is deprecated », alors qu'il est retiré (`make/removed_api.txt:45`, `lzss.h:39`).

L'ancre 16 exempte justement ce fichier, si bien que le sentinel ne voit rien.

**10-3 🟠 — Rien ne dit au studio ce que sa ROM doit aux licences (S).**
- Aucune page de `docs/`, ni le FAQ, ni le README, ne dit si une ROM qui embarque le runtime, la bibliothèque (MIT) et le pilote SNESMOD (MIT, `ATTRIBUTION.md:45-50`) doit porter une notice.
- Recherche faite : `grep -i 'commercial\|sell\|your rom'` ne trouve que le texte brut de la licence.
- C'est la première question d'un studio commercial, le public visé par `.claude/rules/two_audiences.md`.

**10-4 🟠 — La promesse de dépréciation de STABILITY repose sur un outil que doctor déclare optionnel (S).**
- `docs/STABILITY.md:59-61` : « the clang pre-pass of every build prints a warning naming its replacement ».
- `common.mk:602` saute ce passage en silence sans clang, et `opensnes doctor` dit « not found — no C lint pre-pass (optional) » (`main.c:716`).
- GETTING_STARTED:24-25 présente `cc`/`gcc` comme équivalents à clang.
- La solution de repli en Python de la 0.48 (UPGRADING:5-6) a disparu avec la règle « zéro Python ».

Aucun nom n'est déprécié aujourd'hui (`grep OPENSNES_DEPRECATED` : 0 hors `#define`), donc personne n'est cassé maintenant. Mais le premier cycle 1.x n'avertira pas un utilisateur gcc.

**10-5 🟡 — README:156 annonce « 88 examples » ; il y en a 85 (S).**
- `find examples -name main.c | wc -l` → 85, et README:25 et 105 disent 85.
- La ligne (`dc915451`, 2026-10-14) passe entre les mailles : aucun motif de `COUNT_PATTERNS` (`check_doc_drift.py:190-206`) n'accepte « N examples — ».
- C'est une affirmation ancrée que le sentinel croit couvrir.

**10-6 🟡 — Deux commandes luna des tutoriels sont fausses (S).**
- `debugging.md:117` : `luna frames … --out /tmp/frames` → exit 2, « a similar argument exists: '--out-dir' ».
- `profiling.md:71-72` : `--input "150:START 160:RIGHT 200:RIGHT|A"` → exit 2, « bad hex mask `START …` ». luna n'accepte que `frame:hex`, comme le montre GETTING_STARTED:218.
- Aucune ancre ne rejoue les commandes luna des tutoriels.

**10-7 🟡 — Neuf liens cassés sur le site publié (S).** Le scan des `href` du HTML généré trouve :
- `../../lib/include/snes/sa1.h` (sa1.md:418) et `../../lib/include/snes/superfx.h` (superfx.md:291) ;
- `../../effects/fading/` (backgrounds/mode1/README.md:251) ;
- `../window/` et `../gradient_colors/` (color/transparency/README.md:128-130), qui sont des dossiers inexistants ;
- `../parallax_scroll/` (transitions/fading/README.md:266) ;
- `../../../games/likemario/` (scrolling/continuous_scroll/README.md:162, 171) ;
- et deux liens `.claude/` sur `contributing.html`.

Les liens relatifs des README d'exemples ne sont vérifiés par rien.

**10-8 🟡 — Vu sur GitHub, une partie de la doc est illisible ou sans image (S/M).**
- `docs/README.md` se présente comme « the map of `docs/` for people reading the repository on GitHub ». Or `LEARNING_PATH.md` y est fait de 92 directives `@subpage`/`@ref` brutes, sans un lien cliquable, et README:55 y envoie le lecteur.
- Les 5 images ajoutées aux tutoriels (`![mode7 example](perspective.png)`, mode7.md:3, etc.) ne s'affichent que dans Doxygen (`IMAGE_PATH`) : `docs/tutorials/*.png` n'existe pas.

**10-9 🟡 — La référence API publiée, c'est-à-dire le contrat que la 1.0 gèle, porte encore l'histoire interne (S/M).**
- `lib/include/snes/*.h` contient 68 lignes d'historique daté (« since 2026-10-12 », « Removed on 2026-10-05 »), 9 « chantier » (`text.h:201` « As of chantier T.4 », `types.h` ×3…) et 5 renvois `.claude/`.
- Parmi ces renvois, `dsp1.h:47` contient « @see .claude/notes/tech/dsp1_reference.md — the full command reference » : un lien mort pour l'utilisateur, puisque `.claude/` n'est pas dans le zip.
- L'ancre 18 nettoie `docs/` mais ne lit pas les en-têtes, qui deviennent pourtant 8 pages HTML du site.

**10-10 🟡 — `console.h`, l'en-tête de démarrage, n'a pas de tutoriel (M).** `API_INDEX.md:166` l'avoue : « 🟡 partial — no page owns init, brightness, fades, region ». À cela s'ajoutent :
- `build.md:48` ne dit pas la limite de `ROM_BANKS` (8 à 126 en LoROM, 64 en HiROM/SA-1/GSU, 32 en DSP-1), que seul le message d'erreur de `common.mk:242` donne ;
- FAQ:66-72 répond à « How large can my ROM be? » sans donner de chiffre.

**10-11 🟡 — Petites erreurs dans les extraits et les pages de dépannage (S).**
- GETTING_STARTED:201 et 216 : `rom = "../my-snes-game.sfc"`, alors que la même page fixe `TARGET := my_game.sfc` (l. 149). Le lecteur du « manual setup » obtient un test qui ne trouve pas sa ROM.
- sram.md:265 enseigne `save.magic != "SAVE"`, qui compare des pointeurs en C (le champ est `u8 magic[4]`, l. 137).
- TROUBLESHOOTING:118-122, « A big buffer behaves strangely … wrong-banked », contredit l. 110-115 : « The build checks it after every link and stops when the band overflows ».

**10-12 🟡 — Les deux 🔴 de KNOWN_LIMITATIONS ne citent pas les filets que le SDK fournit (S).** KL:29-56 ne mentionne ni `vramQueue`/`vramQueueCost` (`vramqueue.h`), ni l'assertion luna `[asserts.dma] unsafe_writes = 0`, que GETTING_STARTED:251-252 montre pourtant (grep : 0 occurrence dans KL).

**10-13 🟡 — Les outils se disent 1.0.0 dans un SDK 0.50.0 (S).** `bin/opensnes --version` → « opensnes 1.0.0 », de même pour les 10 `opensnes-*` (`tools/*/Makefile: VERSION := 1.0.0`). La version est aussi écrite dans les fichiers générés (`tools/common/incfile.h:10`). Un rapport de bogue citera « 1.0.0 », et `doctor` n'affiche pas la version du SDK. → expert outils pour le fond.

Hors périmètre, une ligne chacun :
- Le zip v0.50.0 publié ne contient pas `luna.sha256`, si bien que `install-luna.sh` refuse et que l'étape A2 échoue pour qui télécharge la release actuelle. C'est corrigé dans `[Unreleased]` (CHANGELOG:190-195) mais pas publié. → expert release/CI.
- 128 des 337 fonctions publiques ne sont appelées par aucun exemple (ex. `gsuFrameBytes`, `hdmaWaveSetSpeed`, ni documentées ni exemplifiées). → expert exemples.
- SA-1 SIWP (KL:310) : le test cité, `libtests_sa1_sram`, lit la BW-RAM et non l'I-RAM que SIWP protège. → expert puces.

## Risques

- **Les pages exemptées du sentinel dérivent sans bruit.** MIGRATING et UPGRADING sont exemptés de l'ancre 16 : 10-2 en est le résultat direct. Toute future suppression de nom laissera ces deux pages fausses.
- **Rien n'exerce l'archive d'exemples.** `release_smoke.py` ne teste que le zip SDK. Une page qui décrit cette archive (10-1) peut donc être fausse indéfiniment.
- **Le site est publié depuis `main`** (README:17, `deploy_docs.yml`). Une correction faite sur `develop` n'atteint le débutant qu'à la release suivante, d'où l'intérêt d'une 0.50.1 documentaire.
- **Le contrat 1.0 serait gelé avec un bruit historique.** STABILITY gèle « every public function … declared in `lib/include/snes/*.h` ». Si les en-têtes partent en 1.0 avec 68 lignes d'historique (10-9), chaque nettoyage ultérieur sera un diff sur les en-têtes gelés, sans danger mais difficile à relire.
- **La note « (optional) » de doctor sur clang** (10-4) finira par donner un utilisateur 1.x qui découvre une dépréciation à la suppression, en 2.0.

## Améliorations recommandées

| # | action | sévérité traitée | effort | premier pas concret |
|---|---|---|---|---|
| 1 | Corriger A4 et tester l'archive d'exemples | 10-1 🔴 | S | GETTING_STARTED:88-91 → `cd opensnes-examples_<v>/examples/bin && luna-gui print_string.sfc`, plus une ligne « rebuild: `make OPENSNES=~/opensnes` » ; faire extraire `opensnes-examples_*.zip` par `release_smoke.py`, qui vérifie `examples/bin/print_string.sfc` |
| 2 | Retirer `textFlush` du parcours texte | 10-2 🟠 | S | MIGRATING:32 et 132 → « `textPrintAt` (the NMI uploads it) » ; MIGRATING:142 → « removed at 0.49 » ; API_INDEX:21 → `textModeInit`, `textPrintAt` |
| 3 | Ajouter « Licensing your ROM » au FAQ | 10-3 🟠 | S | un paragraphe : MIT du SDK et de SNESMOD, ce qu'il faut créditer (ou non) dans une ROM vendue, avec renvoi à `ATTRIBUTION.md` |
| 4 | Aligner la promesse de dépréciation | 10-4 🟠 | S | doctor : « clang — needed for deprecation warnings » (warn explicite) ; STABILITY:59 : « with clang installed (Path A installs it) » ; ou un `opensnes upgrade --deprecated` lancé par `common.mk` quand clang manque |
| 5 | Étendre le sentinel | 10-5, 10-6, 10-7 | S | motif `\b(\d{2,3})\s+examples?\s+[—-]` dans `COUNT_PATTERNS` ; liens relatifs des `examples/**/README.md` ; `luna <sub> --help` comparé aux options citées dans les blocs ```bash de `docs/tutorials` |
| 6 | Corriger les deux commandes luna | 10-6 | S | debugging.md:117 `--out-dir` ; profiling.md:72 `--input "150:0x1000,151:0,160:0x0100,200:0x0180"` |
| 7 | Purger l'historique et `.claude/` des en-têtes | 10-9 | M | étendre l'ancre 18 à `lib/include/snes/*.h` (blocs Doxygen) puis réécrire les 68 lignes et retirer le `@see` de `dsp1.h:47` |
| 8 | Rendre LEARNING_PATH et les images lisibles sur GitHub | 10-8 | S/M | soit le lien README:55 pointe vers le site, soit le filtre Doxygen accepte des liens Markdown à la place des `@subpage` ; images référencées par `../../examples/<cat>/<ex>/<ex>.png` |
| 9 | Écrire `tutorials/console.md` (init, luminosité, fondus, région, taille ROM) | 10-10 | M | partir de la ligne 166 de l'API_INDEX ; ajouter la limite de `ROM_BANKS` à `build.md:48` et au FAQ |
| 10 | Petites corrections | 10-11, 10-12 | S | GETTING_STARTED:201/216 → `../my_game.sfc` ; sram.md:265 → `memcmp(save.magic, "SAVE", 4)` ; supprimer TS:118-122 ; KL:39 et 53 citent `vramQueue` et `[asserts.dma]` |
| 11 | Aligner la version des outils sur le SDK | 10-13 | S | `VERSION := $(OPENSNES_VERSION)` lu depuis `snes.h` ; `doctor` affiche la version du SDK (→ expert outils) |

## Verdict

La documentation d'OpenSNES est au-dessus de ce qu'on attend d'un SDK 1.0 sur la discipline. Doxygen est strict à zéro avertissement, 334 fonctions publiques sur 337 sont documentées, les 515 appels des exemples de code ont la bonne arité, et un sentinel de 19 ancres a fermé presque tous les constats E du 03/10. Ce qui reste casse pourtant précisément le premier contact : la première commande du parcours A pointe vers une ROM absente de l'archive, le guide de migration (un indispensable de la 1.0) prescrit un `textFlush` faux et coûteux, et rien ne dit à un studio ce que sa ROM doit aux licences. Ce sont des corrections d'un jour au total : après elles et une passe sur l'historique des en-têtes, la documentation ne bloque pas la 1.0.

## Suivi de l'audit du 03/10

| Constat (2026-10-04_reste_a_traiter.md) | État 2026-10-17 | Preuve d'aujourd'hui |
|---|---|---|
| **E rouge 2** — les dépréciations n'atteignent pas les utilisateurs sans clang | **partiel** | Aucun nom n'est déprécié aujourd'hui (0 `OPENSNES_DEPRECATED` hors `#define`). Un nom retiré échoue à la compilation avec l'indication de `opensnes upgrade` même sans clang (`common.mk:608`). Mais le canal de dépréciation promis par `STABILITY.md:59` reste clang seul, sauté en silence (`common.mk:602`), et doctor le dit « optional » (`main.c:716`) → 10-4. La décision sur `hdmaEnable` est écrite dans `STABILITY.md:67-70` (sens changé en 0.49), pas dans ROADMAP. |
| **E jaune** — `SEMISUPERFREE BANKS 7-1` dans framework.md:309 et object.md:533 | **corrigé** | `framework.md:300-302` : « from the last bank down to 1 (`ROM_BANKS - 1` to 1) » ; `grep '7-1' docs/tutorials` → 0. |
| **E PF6** — aucune image dans les tutoriels | **partiel** | 5 tutoriels visuels ont une capture (colormath, hdma, mode7, window, mosaic, ligne 3), mais elles sont cassées sur GitHub (10-8). Les 26 autres n'en ont pas. |
| **E PF7** — « chantier » et `.claude/` dans les pages publiques | **corrigé** dans le périmètre d'origine | `grep -w chantier` dans KL, ROADMAP et `docs/` → 0. Seul `.claude/` restant : `docs/tools/luna.md:7`, généré et exempté. Le même défaut est ouvert dans les en-têtes publiés → 10-9. |
| **E PF8** — Doxyfile sans PHILOSOPHY, BENCHMARK, ROADMAP, docs/README | **partiel** | `docs/Doxyfile:26-27` inclut maintenant PHILOSOPHY et BENCHMARK. ROADMAP et CHANGELOG restent hors site ; docs/README n'en a pas besoin (mainpage la remplace). |
| **E PF9** — 🟢 de KL sans test, ni date | **partiel**, bien avancé | 9 🟢 sur 10 nomment un test ou un contrôle de build ; l'entrée « 79 characters » (KL:368) n'en nomme aucun. Les dates ont été retirées volontairement (ancre 18), ce n'est plus un défaut. data_init (KL:217) cite maintenant `opensnes-rom check`. |
| **E PF10** — HARDWARE_VERIFICATION sans résultat | **ouvert** (propriétaire) | `docs/HARDWARE_VERIFICATION.md:170` : « No session recorded yet. » |
| **E PF11** — scrolling.md:262 « from the next scanline » non arbitré | **corrigé** | `scrolling.md:262` dit maintenant « the references we hold do not pin the exact point inside a scanline where a write starts to count ». |
| **E rec 8** — sentinel : carte en-tête → tutoriel et compte de CLAUDE.md | **corrigé** | `check_doc_drift.py:1354` `check_header_map` ; `:214-216` ajoute CLAUDE.md ; `make lint-docs` est vert. |
