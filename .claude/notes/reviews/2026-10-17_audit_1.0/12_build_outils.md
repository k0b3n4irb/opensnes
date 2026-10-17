Expert 12 : build et outils multi-plateformes · audit du 2026-10-17 · HEAD audité `73a3fdfe` (develop, worktree `/home/kobenairb/workspace/opensnes-audit`)

> **Incident pendant l'audit, à lire avant le reste.** Un `cd` vers un dossier de travail inexistant a échoué, et la commande suivante est partie dans le dossier par défaut de la session, **l'arbre principal** `/home/kobenairb/workspace/opensnes`. Elle y a lancé `make OPENSNES=<worktree> clean`, puis `make GSU_BANK=99`, puis de nouveau `make clean`, puis `make GSU_BANK=x`, entre 11:55 et 11:58 environ. Mon `OPENSNES=` n'agissait que sur `common.mk` : à la racine, c'est le `Makefile` de l'arbre principal qui a tourné.
> État constaté juste après, en lecture seule :
> - `bin/` est reconstruit et complet (19 binaires, 11:58).
> - `lib/build` est reconstruit : ses 233 objets sont identiques octet pour octet à ceux du worktree.
> - En revanche, **les 85 ROM des exemples ont disparu**, et **10 fichiers suivis par git sont supprimés** : `examples/audio/{apu_switch/cello,apu_switch/drums,pitch_mod/player,play_noise/player,speech_synth/player}.spc700.bin`, `examples/audio/sfx_from_wav/res/{blip,coin}.h`, `examples/maps/map_scroll/res/tiledMario.{b16,o16,t16}`.
> - Pendant environ trois minutes, `bin/` a été incomplet pour le jeu qui se construit sur cet arbre.
>
> Je n'ai rien restauré : toute écriture dans l'arbre principal m'est interdite. Pour remettre l'arbre en état, il faut soit `git -C /home/kobenairb/workspace/opensnes checkout -- <les 10 fichiers>`, soit `make examples` dans cet arbre, qui les régénère avec les mêmes octets (ici, le worktree reconstruit ne montre aucune différence avec les fichiers suivis). La décision revient au propriétaire. Il faut aussi prévenir the game.
>
> **Mise à jour, 12:22 (reprise de l'audit).** Lu en lecture seule : l'arbre principal est revenu à un état propre, sans que j'y aie touché. `git status --untracked-files=no` ne liste aucun fichier, les 85 `.sfc` sont présents et `bin/` compte 19 binaires. Les 10 fichiers suivis existent de nouveau, datés de 12:20 (`cello.spc700.bin`, `tiledMario.b16`…). Il reste seulement à prévenir the game que `bin/` a été incomplet vers 11:55-11:58.

## Périmètre couvert

- **Fichiers lus.** `make/common.mk` (776 lignes) et `Makefile` en entier. Les fichiers `Makefile` de `examples/`, `lib/`, `compiler/` et `tools/`, ainsi que `tools/tool.mk`. `tools/opensnes/src/main.c` (doctor). Les workflows `opensnes_build.yml` et `release.yml`. `docs/STABILITY.md`, `docs/tools/{build,CONVENTIONS,opensnes-music}.md`, `docs/GETTING_STARTED.md`. Les constats C du 03/10 et leurs suivis.
- **Zip de release.** Construit avec `make release RELEASE_DIR=<scratchpad>`, rc 0, `opensnes_v0.50.0_linux_arm64.zip` de 30,6 Mo, puis inspecté. Je l'ai extrait (y compris dans un chemin contenant un espace) et j'y ai construit le projet de départ (`starter`) et un projet issu d'`opensnes init`. J'y ai aussi lancé `opensnes release` et `opensnes doctor`.
- **Mesures dans des copies du scratchpad.** Builds propres de `compiler/`, `tools/` et `lib/`, chronométrés. Reproductibilité (mêmes octets d'un dossier à l'autre, et entre le SDK du zip et celui de l'arbre). Incrémentalité (changement de réglage, de binaire, d'asset, d'en-tête). Une vingtaine de valeurs de réglages inhabituelles. Les en-têtes et sommes de contrôle des 85 ROM (`opensnes-rom inspect`).
- **Outils.** `--help`, `--version` et `--json` des 12 outils ; codes de sortie sur un fichier absent ; erreurs des fichiers TOML de réglages ; `make test-tools` (165 cas golden, 2,5 s).
- **Hors périmètre.** Le fuzz, le BMP, l'intégrité des zips et le cache de la CI de release (→ expert 07), ainsi que le C accepté par le compilateur (→ expert 04).

## Points forts

- **Les réglages refusent proprement les combinaisons connues.** On ne peut avoir qu'un coprocesseur ; HiROM est refusé avec Super FX, SA-1 ou DSP-1 (`common.mk:186-198`). Sont aussi refusés : `GSU_BANK` sans Super FX ou à 0 (`:202-212`), un `ROM_NAME` non ASCII, de plus de 21 caractères ou contenant `/&\` (`:215-224`), `SRAM_SIZE` hors de 1..7 (`:226`), `ROM_BANKS` hors de 8..126/64/32 selon la carte mémoire (`:231-244`), `GSU_RAM_KB` hors de {32, 64, 128} (`:246`), `RAM_CODE_SIZE` et `ROM_REGION`. Un module de `LIB_MODULES` mal orthographié est refusé avec la liste des modules disponibles (`:345-354`), et `superfx` sans `USE_SUPERFX` avec sa raison. Mesuré : `ROM_BANKS=8.5`, `RAM_CODE_SIZE=abc`, `BANK0_FAIL_THRESHOLD=abc` et `LIB_MODULES=superfx` donnent tous rc 2 avec un message clair.
- **Changer un réglage reconstruit exactement ce qu'il faut.** Avec la même source, un build LoROM suivi de `make USE_HIROM=1` donne le md5 `d4c2b5d0…` comme un build HiROM propre. `USE_HIROM=1 ROM_BANKS=16 USE_FASTROM=1` en incrémental et en propre donnent tous deux `ff042019…`. Les défauts S3 et S4 du 03/10 sont donc fermés. Un compilateur changé reconstruit tout, assets compris : un octet ajouté à `bin/qbe` relance `[opensnes-sprite] … [CC] … [LD]`, par `COMPILER_ID` (`common.mk:446`).
- **Les ROM d'un projet sont reproductibles.** Le projet de départ donne `d5f16fce…` deux fois de suite, dans deux dossiers, avec le SDK du zip comme avec celui de l'arbre. Une copie de `games/tetris` (`86eda1bf…`) et une de `chips/superfx_hello` (`f2099ae8…`), construites ailleurs, sont identiques aux ROM de l'arbre. Deux builds propres de la lib dans deux dossiers donnent 233 objets sur 233 identiques.
- **Rien d'interprété dans le build d'un utilisateur.** `common.mk` ne contient aucun appel à `python`, `perl` ou `node` (grep vide hors commentaires). Le zip ne contient aucun `.py`. Le projet de départ se construit depuis le zip en 0,34 s, et `opensnes release` y enchaîne build, tests luna, en-tête et CRC (« Released: release/game.sfc (tests: passed) »).
- **Les outils suivent les mêmes conventions et sont testés.** Même aide, mêmes options `--json`, `-q`, `--version`, et codes de sortie documentés dans `--help`. `make test-tools` : 165/165 en 2,5 s, dont 59 cas de refus. Les erreurs de réglages sont précises : « line 9: unknown key 'sise' for sheet — see `opensnes-sprite sheet --help` », « --size 24: the OBJ sizes are 8, 16, 32 and 64 ».
- **Le build complet est rapide.** Sur aarch64 8 cœurs, en propre : compilateur 4,5 s (qbe, cproc, wla-dx avec cmake), outils 4,0 s, lib 0,6 s (233 objets), 85 exemples environ 5,5 s. Moins de 20 s au total, hors clonage des sous-modules.
- **CI sur quatre plateformes, avec exécution des ROM.** Les jambes macOS et Windows exécutent le corpus sur luna (`opensnes_build.yml:166-184`), et chaque OS reconstruit son zip puis le consomme (`release-smoke`). Les 85 ROM ont une somme de contrôle correcte (`opensnes-rom inspect` : 85 ok, 0 fausse ; SA-1 compris, S6 fermé).

## Points faibles

**12-1 🟠 Tous les outils annoncent la version 1.0.0 alors que le SDK est en 0.50.0** (effort S, à corriger avant la 1.0)
- Preuve : `VERSION := 1.0.0` en dur dans les 12 `tools/opensnes*/Makefile:3-4`. `bin/opensnes --version` affiche « opensnes 1.0.0 », et `opensnes doctor --json` porte `"version": "1.0.0"`. C'est un reste de la période où `develop` s'appelait 1.0.0 (`CHANGELOG.md:672-679`) : `snes.h` a été renommé, pas les outils. L'ancre 1 de `check_doc_drift.py` ne lit que `snes.h`.
- Conséquence : un rapport de bogue cite une version fausse, et le JSON consommé par les outils ment. Après la 1.0, la version restera figée tant qu'elle ne sera pas dérivée de `OPENSNES_VERSION_STRING`. Le projet classe lui-même une macro de version mensongère comme ship-blocker (`doc_consistency.md`, ancre 1).

**12-2 🟠 Le premier build échoue sur un « Error 1 » sans message quand le dernier fichier de réglages trié n'écrit pas de `_data.as`** (effort S, à corriger avant la 1.0)
- Preuve : dans la recette de `assets_gen.asm` (`common.mk:394-397`), la boucle se termine par `[ -f "$stem"_data.as ] && echo …`. Si le dernier stem n'a pas de `_data.as`, la recette sort avec le code 1 sans rien dire.
- Reproduit deux fois sur une copie du projet de départ. Avec `res/zpal.toml` (`tool = "opensnes-palette"`, `[plan] sprite = ["player.pal"]`, forme reprise de `games/rpg`) : rc 2, « make: *** [common.mk:395: assets_gen.asm] Error 1 », aucune autre ligne. Même chose avec `res/soundbank.toml` (`opensnes-music`).
- Le **second** `make` réussit, parce que le fichier a été écrit (`> $@`) avant l'échec.
- Conséquence : une fonction documentée (plan de palettes) casse le premier build selon l'ordre alphabétique des noms de fichiers. `rpg` y échappe seulement parce que `uibox.png` trie après `palettes`.

**12-3 🟠 L'assembleur, l'éditeur de liens et les outils d'assets ne font pas partie de la clé de reconstruction** (effort S–M, à corriger avant la 1.0)
- Preuve : `COMPILER_ID` (`common.mk:446`, `lib/Makefile:48`) ne somme que `cc65816`, `cproc-qbe` et `qbe`.
- Mesuré dans le SDK extrait du zip : un octet ajouté à `bin/opensnes-sprite`, `bin/wla-65816` et `bin/wlalink`, puis `touch bin/*`, puis `make`. Rien n'est reconverti, réassemblé ni relié (aucune ligne `[`). Le même octet ajouté à `bin/qbe` reconstruit tout.
- Conséquence : une mise à jour du SDK qui corrige un convertisseur (précédents : `-a` corrigé le 2026-10-05, 191 pixels faux sur 256 ; refus S7 et S8) ou wla-dx (le patch `.BASE`) n'atteint pas un projet existant. Le build reste vert, et l'utilisateur doit savoir qu'il faut un `make clean`. Les objets asm de la lib (`lib/Makefile:130`, objets C seulement) ont le même angle mort pour wla-dx.

**12-4 🟠 Un SDK installé dans un chemin contenant un espace casse tout build, alors que `doctor` répond OK** (effort S, à corriger avant la 1.0)
- Preuve : zip extrait dans `…/sp ace/`. `make` dans `starter/` répond « Makefile:23: …/scratchpad/sp: No such file or directory ». `opensnes init mygame` réussit, puis `opensnes build` échoue de la même façon. `opensnes doctor` affiche « OK: SDK — …/sp ace/opensnes … OK: project ».
- `init` refuse bien un *nom de projet* avec un espace (golden « init 'my game' (refused) »), mais rien ne contrôle le chemin du SDK ni celui du projet, et `GETTING_STARTED.md` n'en dit rien.
- Conséquence : sous Windows, `C:\Users\Prénom Nom\…` est courant. Le premier `make` d'un utilisateur échoue avec un message illisible.

**12-5 🟠 La promesse de stabilité ne couvre pas les outils** (effort S, à corriger avant la 1.0)
- Preuve : `docs/STABILITY.md` gèle l'API C, les réglages de `common.mk`, l'ABI et les *formats produits*. Il ne dit rien des lignes de commande `opensnes-*`, des clés des fichiers `*.toml` de réglages, du schéma `--json` ni des codes de sortie (grep `toml|settings|--json|cli` : vide).
- Or ces fichiers de réglages sont écrits à la main par l'utilisateur, à côté de chaque asset (`CONVENTIONS.md`), et le build les lit.
- Conséquence : à la 1.0, renommer une clé de réglages casserait tous les projets sans violer la promesse écrite.

**12-6 🟠 Le modèle de CI livré avec le projet de départ télécharge la dernière release, sans épingle ni somme de contrôle** (effort S, à corriger avant la 1.0)
- Preuve : `starter/.github/workflows/build.yml` interroge `releases/latest` puis lance `curl … -o sdk.zip` sans vérification. Il utilise `actions/checkout@v4` et `upload-artifact@v4` par étiquette mobile, alors que le dépôt épingle les SHA. Il ne lance pas `make test`. Ce fichier part dans le zip et dans chaque `opensnes init --template game` (golden « init my_game-2 --template game == starter/ »).
- Conséquence : un jeu construit en CI sur la 1.0 passera seul à la 2.0, version qui a le droit de casser, et sans que ses tests luna aient tourné.

**12-7 🟡 Des valeurs de réglages hors domaine passent en silence ou échouent dans l'assembleur** (effort S)
- `USE_HIROM=yes`, `USE_SRAM=true`, `USE_FASTROM=2`, `LD_DISCARD=2` : rc 0, le réglage est ignoré, alors que seule la valeur `1` est lue (`$(filter 1,…)`).
- `USE_SA1=1 SA1_BWRAM_SIZE=12` : « DIRECTIVE_ERROR: SRAMSIZE expects 0-7, 18 is out of range! » (`$0$(SA1_BWRAM_SIZE)` vaut `$012`, `common.mk:166`).
- `GSU_BANK=99` et `GSU_BANK=x` : « DIRECTIVE_ERROR: BANK number must be in [0, 7] » sur `gsu_loader.o`. La borne haute de `build.md:66` (« 1 to `ROM_BANKS` − 1 ») n'est pas vérifiée par make.

**12-8 🟡 `ROM_NAME` : les guillemets donnent une erreur shell ou un message faux** (effort S)
- `ROM_NAME=MARIO'S GAME` donne « /bin/sh: unexpected EOF while looking for matching `'' » suivi de « has a character outside printable ASCII » : faux, l'apostrophe est imprimable (`common.mk:215`, `printf '%s' '$(ROM_NAME)'`).
- `ROM_NAME=A"B` donne une erreur shell sur `[HDR]` (`:640`).
- `ROM_NAME=COST $5` produit un titre « COST » sans avertissement (expansion de make), et un `ROM_NAME` vide est accepté.
- La recommandation PF9 du 03/10 demandait l'échappement : seuls `/`, `&` et `\` sont traités.

**12-9 🟡 Un réglage d'asset écrit en TOML valide mais compact est ignoré en silence** (effort S)
- La découverte des réglages (`common.mk:375`, `grep -l '^tool = "opensnes-'`) exige un espace de chaque côté du `=`, en début de ligne. Avec `tool="opensnes-sprite"` (TOML valide), l'asset n'est pas converti.
- L'erreur arrive plus loin : « 'res/player.inc' file not found » puis « lint failed for main.c — fix the warning or use SKIP_LINT=1 to bypass ». C'est un conseil faux : il n'y a aucun avertissement à corriger, et `SKIP_LINT` ne ferait que déplacer l'erreur.
- Ce même message de lint suit **toute** erreur de compilation (`:602`).

**12-10 🟡 Les codes de sortie et le JSON d'erreur ne sont pas homogènes** (effort S)
- L'aide de chaque outil dit « 3 the file system ». Sur un fichier absent, pourtant : `opensnes-tileset`, `-sprite`, `-level`, `-image` et `-music` sortent en 1 ; `-text`, `-sample`, `-rom` et `-save` en 3.
- `opensnes-music bank --json` n'écrit rien sur stdout en cas d'erreur, et `opensnes-palette plan` sort en 2.
- Le JSON d'erreur porte `"error": 1` sans message.

**12-11 🟡 Un message destiné à l'utilisateur renvoie vers une règle interne, et un seuil trop grand déborde en silence** (effort S)
- `tools/opensnes-rom/src/checks.c:78` cite « (.claude/rules/bank0_budget.md) », un fichier qui n'est pas dans le zip et que la règle à deux audiences réserve aux contributeurs.
- `--bank0-fail 99999999999` est lu comme 1215752191, sans erreur.

**12-12 🟡 Les binaires du zip ne sont ni allégés ni reproductibles** (effort S)
- Les 19 binaires sont « with debug_info, not stripped » (`file`), soit environ 66 Mo décompressés pour un zip de 30,6 Mo.
- `TOOL_BUILD_DATE` (`tools/tool.mk:33`, `date +%Y%m%d`) et les chemins absolus des informations de débogage rendent les binaires différents d'un dossier à l'autre. Les 19 de l'arbre principal diffèrent de ceux du worktree, alors que les 233 objets de lib sont identiques.
- Les licences MIT de QBE et ISC de cproc ne sont pas jointes en texte, contrairement à celles de wla-dx et cmdparser (`Makefile:676`). → licences : expert 07 ou gouvernance.

**12-13 🟡 `CC ?= clang` ne sert à rien dans `tools/tool.mk:31`** (effort S)
- make définit déjà `CC=cc`, donc le `?=` ne s'applique jamais : les outils sont compilés par gcc sous Linux.
- C'est la source de l'un des trois avertissements de `audit_build.log` (`cute_tiled.h:1713: warning: unknown option after '#pragma GCC diagnostic' kind [-Wpragmas]`, ligne 302). Les deux autres viennent aussi de l'hôte : dépréciation cmake de wla-dx (ligne 20) et `-rdynamic` inutilisé (ligne 130).

**12-14 🟡 Aucun test ne couvre le build incrémental** (effort M)
- `examples/Makefile:31-33` nettoie chaque exemple avant de le construire. Un `make` sur un arbre déjà construit relance donc 511 étapes `[CC]`, `[AS]` et `[LD]` (mesuré, 5,4 s), et `make release` dépend de `all`.
- Il n'existe aucune cible « construire, modifier une entrée, reconstruire, comparer au build propre ».
- Les défauts de cette classe (S3 et S4 le 03/10, 12-3 aujourd'hui) ne sont trouvés que par les audits.

**12-15 🟡 `make clean` supprime 36 fichiers suivis par git** (effort S)
- `git ls-files examples` liste notamment des `.spc700.bin`, des `.brr`, des `.b16/.o16/.t16` et `sfx_from_wav/res/*.h`. Ce sont des sorties de conversion versionnées, que `clean` efface (`common.mk:760-773`).
- Le build les réécrit avec les mêmes octets, mais on a deux sources de vérité, et `git status` est sale entre un `clean` et un build (constaté sur l'arbre principal, voir l'incident).

**12-16 🟡 `make test-project` ne marche pas sous macOS** (effort S)
- Le commentaire de `Makefile:345` dit « The sed is done in Python so the target behaves the same on macOS ». La recette utilise pourtant `sed -i 's/…/'` (`:354`, `:360`), la forme GNU, qui échoue avec le sed BSD de macOS.
- Le dossier est en dur dans `/tmp`.

**12-17 🟡 Dépendances incomplètes pour les programmes SPC700 et les `.include`** (effort S)
- `%.spc700.bin` (`common.mk:524`) ne dépend ni de `memmap_spc700.inc` ni de `.opensnes_config`.
- Un `.include` écrit par l'utilisateur dans un `.sfx`, un `.spc700.asm` ou un `.asm` n'est pas suivi : seuls les `.incbin` le sont.
- C'est le reste du PF3 du 03/10, sans cas dans le corpus.

**12-18 🟡 Rien ne vérifie que les ROM sont identiques d'un OS à l'autre** (effort S)
- Les jambes macOS et Windows ne font que la passe de vivacité (`--coverage`, `opensnes_build.yml:173,184`), ni `--compare` ni comparaison de hachés avec Linux.
- Le préprocesseur reste celui de l'hôte, non épinglé (`compiler/cc65816/cc65816.c:374-389`, `-undef -nostdinc` ; le build l'annonce : « note: host preprocessor (unpinned) »).

**12-19 🟡 Pas de documentation des réglages hors ligne, et les bornes manquent dans `build.md`** (effort S)
- Le zip ne contient ni `docs/` ni `build.md`, et l'en-tête de `common.mk` ne nomme que 9 réglages sur 44.
- `build.md:48,67` ne donne ni la plage de `ROM_BANKS` (8..126/64/32) ni les valeurs admises de `GSU_RAM_KB`, que `common.mk` impose pourtant.

## Risques

- **macOS.** Aucun `MACOSX_DEPLOYMENT_TARGET` ni `-mmacosx-version-min` (grep vide dans `.github`, `Makefile`, `compiler/Makefile`, `tools/tool.mk`) : les binaires héritent de la version du runner `macos-latest`. Rien n'est signé ni notarisé, et la documentation ne parle ni de quarantaine ni de `xattr` (grep vide). Un zip décompressé par Archive Utility peut être bloqué par Gatekeeper. Non mesuré : pas de Mac. Pas de build darwin x86_64.
- **Windows.**
  - `install-luna.sh:128` appelle `unzip`, absent de la ligne `pacman -S make mingw-w64-ucrt-x86_64-clang` de `GETTING_STARTED.md:46` (non vérifié sur un MSYS2 vierge).
  - La vérification des DLL (`ldd … || true`, `opensnes_build.yml:317`) ne bloque rien.
  - Un SDK sous `C:\Users\Prénom Nom` relève de 12-4.
- **Release locale depuis des binaires instrumentés.** `make test-sanitizers` laisse des binaires ASan dynamiques dans `bin/`, et `release` copie `bin/*` sans contrôle (`Makefile:638`). La CI part d'un arbre neuf ; une release faite à la main, non. Le cache de toolchain de `release.yml` relève de l'expert 07.
- **Un `make clean` à la racine supprime `bin/` immédiatement** (`Makefile:91-96`, `rm -rf bin/`), alors qu'un jeu consomme l'arbre en direct. L'incident de cet audit montre qu'un `cd` raté suffit.
- **Les entrées des assets composés ne sont pas suivies.** La règle `.done` (`common.mk:385`) dépend du `.toml` et de `$(basename)`. Pour `soundbank.toml` ou un plan de palettes, les fichiers listés dans `inputs` ou `sprite` ne sont pas des prérequis (ces cas ne sont pas reliés à la ROM aujourd'hui, `opensnes-music.md:56`). Le jour où ils le seront, ils deviendront périmés en silence.

## Améliorations recommandées

| # | Action | Sévérité traitée | Effort | Premier pas concret |
|---|---|---|---|---|
| 1 | Dériver `VERSION` des outils de `snes.h`, et étendre l'ancre 1 du sentinel à `--version` | 12-1 🟠 | S | Dans `tools/tool.mk` : `VERSION ?= $(shell sed -n 's/.*OPENSNES_VERSION_STRING "\(.*\)"/\1/p' ../../lib/include/snes.h)`, puis retirer les 12 lignes en dur |
| 2 | Faire réussir la boucle de `assets_gen.asm` quel que soit le dernier stem, et ajouter un golden « plan de palettes trié en dernier » | 12-2 🟠 | S | `if [ -f … ]; then echo …; fi` à la place du `&&` (`common.mk:397`), et `set -e` sur la recette |
| 3 | Mettre `wla-65816`, `wlalink`, `wla-superfx`, `wla-spc700` et chaque `opensnes-*` dans la somme de `.opensnes_config` ; les objets asm de la lib dans `COMPILER_STAMP` | 12-3 🟠 | S | Élargir le `$(wildcard …)` de `COMPILER_ID` (`common.mk:446`, `lib/Makefile:48`) ; contrôle : un octet ajouté à `bin/wlalink` doit relancer `[LD]` |
| 4 | Faire refuser par `doctor` (et par `common.mk`) un chemin de SDK ou de projet contenant un espace ; le documenter | 12-4 🟠 | S | `ifneq ($(word 2,$(OPENSNES)),)` + `$(error …)` en tête de `common.mk` ; ligne « fail » dans `doctor` |
| 5 | Ajouter à `STABILITY.md` une section outils : sous-commandes, options longues, clés des réglages, champs `--json`, codes de sortie | 12-5 🟠 | S | Lister ce qui est gelé à partir de `CONVENTIONS.md` |
| 6 | Épingler la version du SDK et sa somme dans le modèle de CI du projet de départ, épingler les actions par SHA, ajouter `make test` | 12-6 🟠 | S | `SDK_VERSION: v1.0.0` en `env`, téléchargement par tag et `sha256sum -c` |
| 7 | Valider les `USE_*` (0/1), `SA1_BWRAM_SIZE` (1..7) et `GSU_BANK` (≤ `ROM_BANKS` − 1) dans make | 12-7 🟡 | S | Une macro `_bool` qui fait `$(error)` hors de {0, 1} |
| 8 | Contrôler `ROM_NAME` sans le shell : refuser `'`, `"` et `$` par `findstring`, refuser le titre vide | 12-8 🟡 | S | Compléter `common.mk:218` |
| 9 | Découvrir les réglages avec `^tool *= *"opensnes-`, et n'afficher « lint failed » que si clang a signalé un avertissement | 12-9 🟡 | S | Corriger le `grep -l` de `common.mk:375` |
| 10 | Un fichier absent donne 3 partout ; `--json` émet toujours un objet complet, avec un message | 12-10 🟡 | S | Mettre le cas dans `tools/common/cli.c` ; un golden par outil |
| 11 | Retirer la référence `.claude/` de `checks.c:78` ; lire les seuils avec `strtol` et une borne | 12-11 🟡 | S | Une ligne |
| 12 | `strip` (ou `-s`) des binaires dans `release` ; `SOURCE_DATE_EPOCH` pour `TOOL_BUILD_DATE` ; `-ffile-prefix-map` ; joindre `LICENSE-qbe` et `LICENSE-cproc` | 12-12 🟡 | S | `strip bin/*` dans la recette `release`, avant le `zip` |
| 13 | `CC := clang` (ou `ifeq ($(origin CC),default)`) dans `tool.mk` | 12-13 🟡 | S | Une ligne |
| 14 | `make test-incremental` : un projet témoin, vingt mutations (réglage, binaire, asset, en-tête, `.include`), comparaison md5 avec un build propre | 12-14, 12-17 🟡 | M | Script shell dans `devtools/`, appelé par `make tests` |
| 15 | Ne plus suivre les sorties de conversion que la règle `.done` régénère | 12-15 🟡 | S | `git rm --cached` des 36 fichiers, plus `.gitignore` |
| 16 | `sed -i.bak … && rm` (portable) dans `test-project` ; corriger le commentaire | 12-16 🟡 | S | Deux lignes |
| 17 | Comparer les hachés des ROM produites sur macOS et Windows à ceux de Linux | 12-18 🟡 | S | Étape `sha256sum examples/**/*.sfc` publiée en artefact par jambe, puis job `compare` |

## Verdict

Le système de build d'OpenSNES a fermé presque tous les constats du 03/10 : les réglages se refusent proprement, une bascule incrémentale donne les mêmes octets qu'un build propre, aucun interpréteur ne tourne dans un build utilisateur, et les ROM sont reproductibles d'un dossier à l'autre. Il reste six dettes 🟠, toutes d'effort S : des versions d'outils fausses (1.0.0), un premier build qui échoue sans message selon l'ordre alphabétique des réglages, une mise à jour d'assembleur ou de convertisseur qui n'atteint pas les projets existants, un chemin avec espace accepté par `doctor`, des outils absents de la promesse de stabilité, et un modèle de CI qui suit la dernière release sans épingle. Ce périmètre est à une semaine d'un gel 1.0, à condition d'ajouter le test incrémental qui manque depuis deux audits.

## Suivi de l'audit du 03/10

| Constat (lettre C de `2026-10-04_reste_a_traiter.md`) | État au 2026-10-17 | Preuve d'aujourd'hui |
|---|---|---|
| S7, plus de 1024 tuiles BG | **corrigé** | `make test-tools` : « PASS convert toomany_bg.png (1056 distinct tiles) (refused) » |
| S8, plus de 256 tuiles Mode 7 | **corrigé** | « PASS convert toomany_m7.png --mode 7 (tile 256 in a byte) (refused) » |
| S12, index ≥ 2^bpp sans carte | **corrigé** | « PASS sheet index5_2bpp.png --bpp 2 (two palette banks in one tile) (refused) » et « mixed_map.png … (refused) » |
| S13, en-tête SA-1 SRAM | **corrigé** (reste 12-7) | `common.mk:159-166` : `$34`/`$35` selon `USE_SRAM`, `SA1_BWRAM_SIZE` ; `build.md:63-64` documente l'exception. `SA1_BWRAM_SIZE=12` donne encore une erreur de wla |
| S14, `SPRITE_SIZE` / `SOUNDBANK_BANK` en incrémental | **corrigé** | La règle de `gfx4snes` n'existe plus ; `.done` dépend de `.opensnes_config` (`common.mk:385`), et `$(SOUNDBANK_OUT).asm` aussi (`:477`), alors que `SOUNDBANK_BANK` figure dans `_CONFIG_TEXT` (`:631`) |
| S15, tmx2snes | **corrigé** | « PASS refused: rotated tile / tile id past 1024 / second tileset » |
| PF3 (reste), compilateur et `.include` en prérequis | **partiel** | Le compilateur est dans la clé (`COMPILER_ID`, mesuré : `qbe` modifié reconstruit tout). wla-dx et les outils d'assets n'y sont pas (12-3, mesuré). Les `.include` des `.sfx`/`.spc700.asm` ne sont pas suivis (12-17) |
| PF8, avertissement `SLOT 0` | **corrigé** | `templates/assets.inc:109` `SLOT "ROM"` ; `grep -c "There is a SLOT number 0" audit_build.log` donne 0 |
| PF9, `GSU_BANK=0` et `ROM_NAME` | **corrigé en grande partie** | `GSU_BANK=0` refusé (`common.mk:208`) ; `ROM_NAME` non ASCII, de plus de 21 caractères ou contenant `/&\` refusé (`:215-224`). Restent `'`, `"` et `$` (12-8), et la borne haute de `GSU_BANK` (12-7) |
| PF10, `doctor` et commentaires | **corrigé** | `doctor` vérifie clang, `wla-spc700`, `wla-superfx` et `sa1_patch` (`tools/opensnes/src/main.c:645,715`) ; `hdr_hirom.asm:62` dit « $00=ROM, $02=ROM+RAM+battery » ; gfx4snes et wav2brr sont retirés |
| rec 5, test d'en-tête | **corrigé** | `opensnes-rom inspect` sur les 85 ROM : 85 sommes correctes ; `ROM_BANKS_MAX` (`common.mk:241`) ; `luna_runner.py:173-194` (`header_problem`, `checksum_computed`) |
| rec 8, lignes partenaires | **corrigé côté luna**, ouvert côté snes-rag | `checksum_computed` livré, ligne retirée de `OPEN_luna.md` ; la ligne SA-1 BW-RAM reste ouverte dans `OPEN_snes-rag.md:11` (attente du partenaire) |
