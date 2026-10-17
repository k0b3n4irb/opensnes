Expert : sécurité / pentest (07) · date : 2026-10-17 · HEAD audité : `develop` @ `73a3fdfe` (worktree `/home/kobenairb/workspace/opensnes-audit`)

# Audit 1.0 — 07 Sécurité : outils hôtes, chaîne d'approvisionnement, secrets

## Périmètre couvert

- Revue de code des parseurs et des écritures de fichiers des outils livrés : `tools/common/cli.c` (TOML de réglages, chemins), `tools/opensnes/src/main.c` (CLI : `system()` / `popen()` / `execl`), `tools/lib/tmx2snes/src/level.c`, `tools/lib/gfx4snes/src/loadbmp.c` + `images.c`, `opensnes-rom`, `opensnes-save`, `tools/third_party/README.md`.
- Fuzz : les 7 harnais de `tools/fuzz/` construits (`make -C tools/fuzz all`), `replay` vert (13 entrées de régression), puis 240 s par cible en parallèle sur des corpus copiés dans le scratchpad (`-rss_limit_mb=2048 -timeout=10`).
- Entrées hostiles à la main contre `bin/opensnes-*` du worktree : PNG « bombe » (48 Ko → 20000×20000), PNG 2³¹×2³¹, ROM vides / aléatoires, entiers négatifs / énormes sur 13 options, `.tmj` au nom de calque hostile (avec un `opensnes-level` recompilé ASan+UBSan dans le scratchpad), dossier de projet au nom hostile pour `opensnes build`.
- Chaîne d'approvisionnement : `scripts/install-luna.sh` + `testing/luna.sha256`, les 9 workflows (`uses:`, `permissions:`, caches, téléchargements), recette `make release`, `compiler/PINS.md` (`make verify-toolchain`), `.gitmodules`.
- Secrets : `.gitignore`, `git log --all -- .env`, `git log --all -p -S ghp_` / `-S github_pat_`, recherche de clés AWS / privées / Slack dans l'arbre ; `.env` non lu. `SECURITY.md`.

## Points forts

- **Les parseurs fuzzés tiennent.** 240 s par cible, 0 plantage, 0 fuite de sanitizer : lodepng 6 787 653 exécutions, wav 84 917 912, json 2 456 598, stbimage 1 707 426, tiled 1 128 047, rearrange 14 901, itloader 9 177 (`stat::number_of_executed_units` des journaux). `make -C tools/fuzz replay` : « fuzz-replay: OK » sur 13 entrées committées (`crashes/`).
- **Le décodeur PNG refuse les dimensions absurdes proprement.** Un PNG déclarant 2147483647×2147483647 : `opensnes-image`, `-palette`, `-text` sortent en code 1 avec « integer overflow due to too many pixels ». Le patch local de lodepng (réservation plafonnée à 1032× la taille compressée, `tools/third_party/README.md` « Local patches ») est documenté avec son entrée de régression.
- **Les entiers d'options sont bornés.** 11 des 13 essais refusés avec un message nommant la borne (`--scale -50`, `--align -8`, `--colors 99999999999`, `--passes 2000000000`, `--metasprite -16 -16` / `0 0`, `--offset -5`…) ; `--align 2147483647` finit en « out of memory » code 3, pas en plantage. `opensnes-save get` vérifie `at + count ≤ len` en `size_t` (`opensnes-save/src/main.c:208`).
- **`opensnes-rom` résiste aux ROM malformées** : fichier vide, 100 octets, 40 000 octets aléatoires, `/dev/zero` → code 1 et message (« too short to hold a cartridge header », « no cartridge header at $7FC0, $FFC0 or $40FFC0 »).
- **Le fichier de réglages ne peut pas rediriger une sortie.** `cli_load_settings_path` refuse les clés `out` et `save` (`tools/common/cli.c:412-413`) et une table autre que la sous-commande ; une valeur trop longue tombe sur `toml_value` qui renvoie 0 (syntaxe), pas de débordement.
- **luna est vérifié par SHA-256 épinglé**, hors du canal de téléchargement : `install-luna.sh` refuse toute archive absente de `testing/luna.sha256` (4 sommes pour v1.38.0) et vérifie avec `sha256sum -c` avant `unzip` ; le zip embarque `luna.sha256` (`Makefile:655`).
- **Workflows : 41 `uses:` sur 41 épinglés par SHA de 40 caractères** (`grep -h 'uses:' .github/workflows/*.yml | grep -vcE '@[0-9a-f]{40}'` → 0) ; le Doxygen téléchargé est vérifié par SHA-256 (`lint.yml:201`, `deploy_docs.yml:26`) ; aucun `pull_request_target` ni `workflow_run`.
- **Toolchain épinglé** : `make verify-toolchain` → « OK: toolchain pins match (3 submodule(s) verified) » ; sous-modules en HTTPS sur les forks du mainteneur.
- **Pas de secret dans l'historique.** `.env` ignoré (`.gitignore:2`), `git log --all -- .env` vide ; `-S ghp_` / `-S github_pat_` ne trouvent que deux commits de prose (`80847cae`, `080a3e88`) sans aucune valeur de jeton (`grep -oE 'ghp_[A-Za-z0-9]{10,}'` : 0) ; aucune clé AWS / privée / Slack dans l'arbre.
- **Le CLI lance l'émulateur sans shell** : `execl(emu, emu, rom, NULL)` (`tools/opensnes/src/main.c:503`), et `--tag` / le nom de projet sont filtrés à `[A-Za-z0-9._-]` (`:589`, `:358`).

## Points faibles

**07-1 🟠 — `opensnes-level` : débordement d'un tampon global et écriture hors du dossier de sortie, pilotés par le nom d'un calque du `.tmj`.** (effort S)
`layer_file()` (`tools/lib/tmx2snes/src/level.c:110-119`) fait `strcpy(filemapname, filebase)` puis `sprintf(lastpostslash + 1, "%s%s", layer->name.ptr, ext)` dans `static char filemapname[1100]` (`:69`), sans borne ni filtrage du nom.
- Débordement mesuré : calque nommé de 1000 `B`, `--out` de 200 caractères, binaire recompilé ASan → `AddressSanitizer: global-buffer-overflow … WRITE of size 1005` dans `layer_file`. Le binaire livré (statique, sans sanitizer) écrit au-delà du tableau sans planter et sort en code 1. cute_tiled plafonne une chaîne à 1024 octets, donc un chemin de sortie d'environ 80 caractères suffit.
- Traversée mesurée : calque nommé `../escape/PWN`, `opensnes-level convert --tileset tileset.map --out out trav.tmj` → code 0, « 1 layer of 64x64 tiles (../escape/PWN.m16) », fichier de 8 198 octets écrit dans `escape/`, hors de `--out`. Avec `../../..` ou un chemin absolu, l'écriture va n'importe où dans les droits de l'utilisateur.
- Pourquoi le fuzz ne l'a pas vu : le harnais `tiled` (`tools/fuzz/fuzz_tiled.c`) ne fait que charger avec cute_tiled ; le code qui consomme les champs du fichier (`level.c`) n'est pas fuzzé.
Conséquence : une carte Tiled venue d'un tiers (pack d'assets, contributeur, issue) corrompt la mémoire de l'outil ou écrase un fichier du projet de l'utilisateur à son insu, en toute discrétion.

**07-2 🟠 — Le décodeur BMP de `opensnes-sprite` / `opensnes-tileset` ne contrôle aucune taille.** (effort S–M) — constat de lecture de code, sans PoC.
`bmp_decode(…, const unsigned char* in, size_t insize)` (`tools/lib/gfx4snes/src/loadbmp.c:288`) n'utilise jamais `insize` :
- `memcpy(&bmphead, in, sizeof bmphead)` (`:302`) puis les en-têtes, la palette (`:327-333`, offset tiré de `biSize`) et les pixels (`:373`, offset tiré de `bfOffBits`) sont lus sans comparer à la taille du fichier → lecture hors du tampon sur un fichier court ou mal formé ;
- `malloc(iwidth*iheight)` (`:345`) en `unsigned int` : multiplication qui déborde sans contrôle, puis boucle d'écriture sur `iheight*iwidth` → écriture hors du bloc alloué ;
- le chemin BI_RLE8 (`:224`, `:272`) écrit `*image[pos++]` : la priorité fait indexer le pointeur de pointeur (`*(image[pos])`), pas le tampon ; `pos`, `line` et `inbuf` ne sont jamais bornés. Ce chemin n'a jamais pu fonctionner et écrit à une adresse dérivée de la pile.
Aucun harnais ne couvre le BMP (`tools/fuzz/Makefile:26`, `FUZZERS` sans bmp), aucune fixture `.bmp` n'existe dans les goldens (`ls tools/lib/gfx4snes/tests/fixtures | grep -i bmp` : vide), alors que les deux outils annoncent « indexed PNG or BMP » dans leur aide (`opensnes-sprite/src/main.c:400`, `opensnes-tileset/src/main.c:274`).
Conséquence : le seul format d'entrée image non fuzzé est aussi celui qui ne contrôle rien ; un `.bmp` mal formé lit ou écrit hors mémoire dans un outil livré.

**07-3 🟠 — Le CLI `opensnes` interpole des chemins dans des lignes de shell.** (effort S)
`run_make` construit `make --no-print-directory -C "%s"` et appelle `system()` (`tools/opensnes/src/main.c:259-264`) ; `run_tool` fait de même avec le chemin du SDK et le ROM (`:513-525`) ; `doctor` passe le chemin de luna à `popen()` (`:722-724`). Les guillemets doubles n'empêchent ni `$(…)`, ni les backquotes, ni un `"` dans le nom.
Mesuré : un dossier `p$(touch PWNED_BY_DIRNAME)/`, `opensnes init game && cd game && opensnes build` → le fichier `PWNED_BY_DIRNAME` est créé dans `game/`, puis make échoue sur `…/inj/p/game: No such file or directory`.
Portée honnête : construire un projet non fiable exécute déjà son Makefile ; ce n'est donc pas une élévation de privilège. En revanche, un SDK ou un projet extrait dans un chemin contenant `$`, `` ` `` ou `"` casse le build, ou exécute un fragment de commande. Le remède est mécanique (`fork` + `execvp` / `posix_spawn`, `_spawnvp` sous Windows).

**07-4 🟡 — Pas de plafond de pixels avant décodage.** (effort S)
Un PNG de 48 685 octets (20000×20000 en niveaux de gris 1 bit) coûte, mesuré avec `/usr/bin/time` : `opensnes-image inspect` 9,0 s / 1,61 Go, `opensnes-text inspect` 8,3 s / 2,00 Go, `opensnes-palette quantize` 21,5 s / 2,35 Go, et ce dernier écrit un PNG de 20000×20000. Aucun de ces outils n'a besoin de plus que quelques milliers de pixels de côté (une image SNES fait 256×224 ; une planche de sprites ou de tuiles reste sous 4096). Conséquence : déni de service local ou OOM en CI sur un asset hostile ou corrompu.

**07-5 🟡 — Le fuzz ne vise pas tout ce qui lit un fichier, et une cible ne vise plus rien de livré.** (effort M)
- Aucune couverture fuzz pour : le BMP (07-2), `level.c` (07-1), `it2spc.c` (1169 lignes, conversion après le chargement de l'IT), `anim.c` (aseprite après le JSON), le TOML de réglages (`cli.c:315-430`), le `.map` lu par `opensnes-level --tileset`, les `.pal` lus par `opensnes-palette plan` / `--palette`.
- `fuzz_stbimage` fuzze un parseur qu'aucun outil livré ne compile (`grep -rn STB_IMAGE_IMPLEMENTATION tools --include=*.c` → seul `tools/fuzz/fuzz_stbimage.c:10`), alors que `tools/third_party/README.md:7` et `ATTRIBUTION.md:107` disent « font2snes ».
- `fuzz_itloader` passe à 38 exécutions/s parce que le chargeur écrit un avertissement par échantillon tronqué (journal de 112 Mo pour 240 s, 1 037 178 lignes « truncated ») : la cible la plus délicate (« trusts every count and offset », `fuzz_itloader.c:3`) est aussi la moins explorée, d'environ 1000× moins que les autres.
- Les corpus de départ sont maigres : 1 graine pour `tiled` et `wav`, 2 pour `itloader`.

**07-6 🟡 — Les workflows héritent des droits par défaut du jeton.** (effort S)
7 workflows sur 9 n'ont aucun bloc `permissions:` (`compiler-hunt`, `fuzz`, `lint`, `luna-bench`, `msys2_cproc_diagnostic`, `opensnes_build`, `pal`) : leur `GITHUB_TOKEN` a les droits du réglage du dépôt, non vérifiable sans réseau. `release.yml:9-10` donne `contents: write` à **tous** ses jobs, y compris `build`, qui restaure un cache et lance `make` sur trois OS, alors que seul `publish` écrit.

**07-7 🟡 — Les zips de release ne portent ni somme publiée ni provenance, et leur toolchain vient d'un cache.** (effort S)
`release.yml` publie cinq zips (`:310-315`) sans fichier `SHA256SUMS` et sans attestation (`grep -n -i 'sha256\|attest' release.yml` → 0). L'utilisateur n'a que le `digest` de l'API GitHub, le canal que `testing/luna.sha256` utilise lui-même pour luna. Par ailleurs, `bin/` et les trois compilateurs sont **restaurés du cache** construit par la CI de `develop` (`release.yml:65-74`) et copiés tels quels (`Makefile:638`, `cp -r bin/*`) : le binaire livré n'est pas reconstruit depuis les sources du tag. Le cache n'est écrivable que par des runs de branches du dépôt, donc le risque d'empoisonnement est faible ; la reproductibilité, elle, n'existe pas.

**07-8 🟡 — `SECURITY.md` décrit le projet de juin.** (effort S)
`SECURITY.md` (dernier commit `25bc1acb`, 2026-06-29) : « The project is in beta », périmètre « `gfx4snes`, `font2snes`, `smconv` » (remplacés par les `opensnes-*`), et « Toolchain submodules … are pinned third-party projects — report issues there upstream », alors que ce sont des forks à patches locaux et que la politique du dépôt est de ne pas contribuer en amont (`partners.md` §6). Pour une 1.0, cette page est le contrat de sécurité : elle doit nommer les outils réels et dire où va un rapport sur le compilateur.

**07-9 🟡 — Les sorties suivent les liens symboliques et écrasent sans prévenir.** (effort S, confort)
`cli_write_file` fait `fopen(path, "wb")` (`tools/common/cli.c:257-268`) : un `--out` qui contient un lien symbolique préexistant (`x.pic -> ~/.bashrc`) est suivi. C'est le comportement usuel d'un outil de build local ; à noter seulement parce que 07-1 permet déjà à un fichier d'entrée de choisir le nom de sortie.

## Risques

- **Jeton classique sur-scopé dans `.env`.** `.claude/notes/conventions/gh_auth_via_dotenv.md` (section « The classic token is over-scoped ») consigne au 2026-10-03 un `GH_GHP_TOKEN` portant `repo`, `delete_repo`, `admin:org`, `workflow`, et un conseil de révocation. Rien dans le dépôt ne dit qu'il a été révoqué ou réduit ; sans lire `.env` ni appeler l'API, je ne peux pas le vérifier. Toute session agent de cet arbre (jusqu'à 13 en parallèle aujourd'hui) a ce fichier à portée. → à confirmer par le propriétaire, en une ligne.
- **Patch local de lodepng à ré-appliquer à chaque mise à jour** (`tools/third_party/README.md`, « Re-apply on any lodepng update ») : une mise à jour faite sans relire cette note rouvre l'allocation de 17 Go ; seule l'entrée `crashes/lodepng/` du replay la rattraperait.
- **Le build exécute le Makefile du projet** : `opensnes build|test|release` sur un projet téléchargé, c'est exécuter son code. C'est normal pour un SDK, mais ce n'est écrit nulle part (ni dans `SECURITY.md`, ni dans `GETTING_STARTED.md`) ; un utilisateur peut croire qu'« ouvrir un exemple » est sans risque.
- **Fenêtre de cinq releases luna** : `install-luna.sh` échoue proprement quand l'asset a disparu (`no_asset_hint`), mais le repli proposé (`git clone … && cargo build`) se fait sans vérification d'intégrité du tag.
- **`compiler-hunt.yml:59`** interpole `${{ github.event.inputs.start }}` dans un `run:`. C'est sans risque tant que seul le mainteneur peut lancer un `workflow_dispatch`, mais le motif est celui des injections de script GitHub Actions ; à passer par `env:` si le dépôt ouvre un jour des droits d'écriture.

## Améliorations recommandées

| # | action | sévérité traitée | effort | premier pas concret |
|---|---|---|---|---|
| 1 | Filtrer le nom de calque (refuser `/`, `\`, `..`, NUL, plafonner à 64) et passer `layer_file` en `snprintf` borné avec contrôle de troncature | 07-1 🟠 | S | `level.c:110-119` : `snprintf(filemapname, sizeof filemapname, …)` + refus si `strpbrk(name, "/\\")` ou `strstr(name, "..")` ; fixture golden « layer name `../x` refusé » |
| 2 | Borner le décodeur BMP (insize ≥ 54, offsets + tailles ≤ insize, `w*h` en `size_t` avec plafond, RLE8 : corriger `(*image)[pos++]` et borner `pos` / `inbuf`) ; ajouter un harnais `fuzz_bmp` et une fixture `.bmp` | 07-2 🟠 | S–M | harnais calqué sur `fuzz_lodepng.c` appelant `bmp_decode(&img, &st, &w, &h, data, size)` ; l'ajouter à `FUZZERS` |
| 3 | Remplacer `system()` / `popen()` du CLI par `fork`+`execvp` (POSIX) et `_spawnvp` (Windows), avec argv construit et sans shell | 07-3 🟠 | S | `run_make` d'abord (`main.c:256-266`) ; `chdir` dans l'enfant à la place de `-C "%s"` ; test : projet dans un dossier `a$(x)"b` |
| 4 | Plafond de pixels commun avant décodage (par ex. 8192×8192), via `lodepng_state.decoder` ou un contrôle de l'IHDR | 07-4 🟡 | S | une fonction `cli_check_dims()` dans `tools/common`, appelée par les 6 outils image |
| 5 | Étendre le fuzz aux consommateurs (level.c, it2spc, anim.c, TOML, `.map`, `.pal`), faire taire le chargeur IT sous fuzz, supprimer ou réaffecter `fuzz_stbimage`, enrichir les graines | 07-5 🟡 | M | `-close_fd_mask=2` dans la recette `run` pour itloader ; `fuzz_level.c` qui écrit dans un tmpdir et vérifie qu'aucun fichier n'apparaît hors de lui |
| 6 | `permissions: contents: read` en tête de chaque workflow ; dans `release.yml`, `contents: write` sur le seul job `publish` | 07-6 🟡 | S | 7 lignes, un commit `ci(...)` |
| 7 | Publier `SHA256SUMS` avec les zips (et, à terme, `actions/attest-build-provenance`) ; reconstruire le toolchain sans cache pour un tag | 07-7 🟡 | S | étape `sha256sum opensnes_*.zip > SHA256SUMS` dans `publish`, ajoutée à `files:` |
| 8 | Réécrire `SECURITY.md` pour la 1.0 : outils réels, forks du toolchain (rapport ici), « construire un projet exécute son Makefile », versions supportées après 1.0 | 07-8 🟡 + risque | S | remplacer la ligne « In scope » et la note « Toolchain submodules » |
| 9 | Confirmer la révocation ou la réduction (`public_repo`) du jeton classique et l'écrire dans la note | risque | S | une ligne datée dans `gh_auth_via_dotenv.md` |

## Verdict

Les parseurs que le projet a choisi de fuzzer sont solides : 0 plantage en 240 s par cible, plus de 100 millions d'exécutions au total, des entiers d'options bornés et une chaîne d'approvisionnement exemplaire pour un projet de cette taille (SHA partout, luna vérifié hors canal, aucun secret dans l'historique). La surface d'attaque réelle est ailleurs, dans les parties qui consomment un fichier sans être fuzzées : `opensnes-level` déborde un tampon global et écrit hors du dossier sur un simple nom de calque (mesuré sous ASan), le décodeur BMP des deux outils graphiques ne vérifie aucune taille, et le CLI passe des chemins à `system()`. Pour une 1.0 qui promet des outils compilés de qualité « produit », 07-1 et 07-2 se corrigent en un jour chacun et devraient l'être avant le tag, 07-3 dans la foulée ; le reste relève de l'hygiène.

## Suivi de l'audit du 03/10

Mon domaine n'avait pas de lettre propre dans `2026-10-03_audit/2026-10-04_reste_a_traiter.md`. Seul un constat de ce fichier touche mon périmètre :

- **D PF12** (🟡, « Le fuzz perd des runs à cause de la concurrence (`fuzz.yml:33-35`) », « non traité » au 04/10) : **corrigé**. `fuzz.yml:38` porte aujourd'hui `cancel-in-progress: ${{ github.ref != 'refs/heads/develop' }}`, avec un commentaire qui cite D_testing.md PF2 ; `timeout-minutes: 90` est dimensionné pour les 7 cibles × 600 s (`fuzz.yml:50`).
- Le point fort du rapport H (« 38 `uses:` épinglés par SHA », `.env` absent de l'historique) **tient toujours** : 41/41 épinglés aujourd'hui, `git log --all -- .env` vide.
