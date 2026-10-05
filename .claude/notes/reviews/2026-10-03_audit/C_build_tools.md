# Audit C — système de build et outils d'assets (2026-10-03)

`develop` @ `73fbdddb`, v0.47.0, luna v1.31.0. Lecture seule sur le dépôt ; toutes les expériences dans des copies sous
`/tmp/claude-1000/-home-kobenairb-workspace-opensnes/e33d7b0e-5fea-4b39-854f-2296e1506ce0/scratchpad/` (`ps/` = copie de
`examples/text/print_string`, `gsu/` = copie de `devtools/libtests_gsu`, `sa/` = `examples/chips/sa1_hello`,
`d1/` = `examples/chips/dsp1_cube`, `gfx/`, `wav/` = entrées fabriquées pour les outils, `cks.py` = vérificateur de
somme de contrôle). Le seul `make` lancé dans le dépôt : `make test-tools` (goldens dans des dossiers temporaires).

## Défauts silencieux candidats

Tous construits avec `rc=0`, sans message d'erreur, sauf mention contraire.

| # | Où | Attendu | Obtenu | Preuve | Sév. |
|---|---|---|---|---|---|
| S1 | `make/common.mk:122,127` — `ROM_BANKS` sans borne haute par mapping | refus, ou données placées dans des banques que la carte mappe en ROM | LoROM `ROM_BANKS=127` : `main_string.3` à **`$7E:8000` (WRAM)** ; `128` → `$7F` ; `200`/`256` → `$C7`/`$FF` (miroirs de `$47`/`$7F`). HiROM `ROM_BANKS=80` : label à **`10f:0000`** (débordement 24 bits). SA-1 `ROM_BANKS=65` : `.rodata.*` à **`$40:0000` = BW-RAM**. DSP-1 `ROM_BANKS=64` : assets en `$3F:8000`, au-delà de la carte 1 Mo que le module `dsp1` pilote | `ps/` : `luna diff b8.sfc b127.sfc` → `DIFF` aux frames 60 et 120 ; capture `b127.png` : fond bleu **sans le texte** (`b8.png` : « TEXT MODULE TEST ») ; `h80.png` idem ; `sa/` : `luna diff a.sfc b.sfc` → DIFF, capture noire. Cartes : SA-1 `$40-$4F` = BW-RAM (vitor-sa1-hw-doc, chunk `7109aff120e0c93c`, *solid*, aucun arbitre) ; DSP-1 LoROM 1 Mo DR/SR en `30-3F:8000-FFFF` (fullsnes, arbitre, chunk `57eb36348d575212`) | 🔴 |
| S2 | `bin/cc65816:86` — `cc -E` de l'hôte, sans `-nostdinc`/`-undef` | `#include <stdint.h>` refusé, ou types aux tailles de la cible | `sizeof(int32_t)` = **2**, `sizeof(int64_t)` = **4**, `sizeof(intptr_t)` = 4 : le `<stdint.h>` de glibc est lu avec les tailles de cproc (`int` 2 octets, `long` 4). `__aarch64__` est défini sur cette machine (sur x86_64, `__x86_64__`) : une même source donne deux ROM selon l'hôte | `scratchpad/pp2.c` → `pp2.s` : `.dw 4` (a), `.dw 2` (b), `.dw 4` (c) ; `pp.c` → `host` = 2. Aucune mention de `stdint` dans `docs/`, `KNOWN_LIMITATIONS.md`, `compiler/*.md` (`grep -rni stdint` vide). → aussi aspect compilateur | 🔴 |
| S3 | `common.mk:402` — `%.sfx.bin` ne dépend ni de `.opensnes_config` ni des `.include` | `GSU_BANK` changé → programme GSU réassemblé | `make GSU_BANK=2` sur un arbre construit avec `GSU_BANK=1` : crt0, asm et C reconstruits, **pas `[GSU]`** ; `gsu_job.sfx.bin` garde son md5 `c1aa0f66…` ; la section est déplacée en banque 2 mais le programme lit `ROMB` = 1 | `gsu/` : `cmp -l inc.sfc libtest_gsu.sfc` (incrémental vs propre) → 3 octets, dont `65602: 1 → 2` (l'octet de banque dans le programme) et la somme de contrôle | 🟠 |
| S4 | `common.mk:527,555,549` — `data_init_start.o`, `data_init_end.o`, `$(SOUNDBANK_OUT).o` sans `.opensnes_config` ; `MEMMAP_DEP` qui change de nom pointe vers un fichier plus ancien | `make USE_HIROM=1` après un build LoROM → ROM HiROM | **ROM LoROM de 256 Ko** (en-tête en `$7FC0`, `$FFD5=$20`) alors que le build propre donne 512 Ko, en-tête `$FFC0`, `$21` ; 23 773 octets différents. Pour un projet SNESMOD, le `sed .ORG $8000` HiROM (`:361`) n'est pas réappliqué non plus | `ps/` : `cks.py inc_hi.sfc print_string.sfc` ; sortie make incrémentale : seulement `[HDR] [AS] crt0 [CC] main.c [LD]` | 🟠 |
| S5 | `lib/source/mul32.asm:47`, `div32.asm:59` — `SEMISUPERFREE BANKS 7-1` littéral dans la lib prébuilt | `ROM_BANKS=1..7` → ROM de 1..7 banques, ou refus | fichier toujours **262 144 octets**, `tcc_mul32` en **`$07:8000`**, en-tête `$FFD7` = `$05` (32 Ko) pour `ROM_BANKS=1`, `$06`, `$07` pour 2, 3. Un lecteur qui respecte l'en-tête mirroire la banque 7 sur la banque 0 : chaque `jsl __mul32` saute dans du code de bank 0 (luna, qui mappe le fichier entier, rend identique : `diff b8 b1` MATCH) | `ps/` : boucle `ROM_BANKS` (`size=262144 hdr=2000050001 banks_used=[00 07]`) ; `grep '^07:' print_string.sym` | 🟠 |
| S6 | `tools/sa1-patch/src/sa1_patch.c:75` — OR `$03` sur `$7FD5` sans recalcul | somme de contrôle `$FFDE` = somme des octets | **les 3 ROM SA-1 du dépôt** ont une somme fausse de +3 (`sa1_hello` 0x9A01 vs 0x9A04, `sa1_starfield` 0x01CD vs 0x01D0, `libtest_sa1_sram` 0x7BA5 vs 0x7BA8) ; les 100 autres ROM sont justes | `python3 scratchpad/cks.py $(find examples devtools -name '*.sfc')` : 100 OK, 3 MISMATCH. luna dit `"checksum_valid": true` (il ne vérifie que le complément) → à remonter à luna | 🟠 |
| S7 | gfx4snes, cartes BG (`-m`, mode 1) | > 1024 tuiles uniques → erreur | 2048 tuiles uniques : champ tuile plafonné à 1023, **1024 entrées portent le bit 10** (= bit 0 de palette) : la tuile 1024+k s'affiche comme la tuile k en palette 1 | `gfx/big.png` (512×256, 2048 tuiles distinctes) : `Counter(x>>10)` = `{0:1024, 1:1024}` ; log : « 2048 tiles (ratio 0%) processed », rc 0 | 🟠 |
| S8 | gfx4snes, `-M 7` | > 256 tuiles uniques → erreur | `.pc7` de 32 768 octets (512 tuiles) mais `.mp7` à **256 valeurs distinctes, max 255** : index tronqué à 8 bits | `gfx/m7b.png` (512 tuiles distinctes), rc 0 | 🟠 |
| S9 | `tools/smconv/src/it2spc.c:335-338` | > 8 canaux IT → échec | affiche `error: More than 8 channels. Found channel 12`, **sort en 0** et écrit `ch12.bnk` (motif tronqué à la ligne fautive) ; `common.mk:358` continue | `wav/mkit.py` → `ch12.it` ; `smconv -s -o ch12 … ; echo rc=$?` → `rc=0` | 🟠 |
| S10 | wav2brr (`main.c:284-286`) + règle `%.brr: %.wav` (`common.mk:383`) | WAV > 32 kHz signalé au build | avertissement **seulement avec `-v`**, que la règle automatique ne passe jamais ; le `.brr` n'emporte pas le taux. À `AUDIO_PITCH_DEFAULT 0x1000` (32 kHz, `audio.h:100`) un 44,1 kHz joue à ×0,726 — **plus grave et plus lent** ; `README.md:20-21` et le message disent « sharp / higher-pitched », le contraire | `wav/mono44k.wav` → « wrote mono44k.brr (1800 bytes, 200 blocks) », aucun avertissement | 🟠 |
| S11 | `common.mk:130` — `GSU_RAM_KB` sans validation, `int(log2)` | 48 Ko → arrondi haut ou refus ; 0 / faute de frappe → erreur | `48` → `$FFBD=$05` (**32 Ko** déclarés) ; `0` ou `abc` → traceback Python à l'écran, **build vert**, `.DEFINE GSU_RAM_SIZE_VAL ` vide, `$FFBD=$00` (une sauvegarde Super FX sans RAM déclarée) ; `1024` accepté | `gsu/` : `make GSU_RAM_KB=0` → `rc0=0`, `xxd -s 0x7fb0` | 🟠 |
| S12 | font2snes ; gfx4snes sans `-m` | index de pixel ≥ 2^bpp refusé | masqué : glyphe 0 en index 4 sous 2bpp → **8 lignes à zéro** ; gfx4snes `-u 16` sans carte : tuile en index 16-31 = même bitmap que 0-15 | `gfx/f5.png` → `xxd -l 16 f5.bin` tout à zéro ; `gfx/c32.pic` tuiles 0 et 1 identiques | 🟡 |
| S13 | `common.mk:160` — SA-1 : `SRAMSIZE $05` codé en dur | `SRAM_SIZE` honoré ou refusé sous SA-1 | `USE_SRAM=1 SRAM_SIZE=1` et `=7` → `$FFD8=$05` les deux fois ; `docs/tools/build.md:54` ne mentionne pas l'exception | `sa/` : `xxd -s 0x7fd5 -l 4` → `23350805` ×2 | 🟡 |
| S14 | `common.mk:370` (GFX), `:356` (soundbank) | `SPRITE_SIZE`, `SOUNDBANK_BANK` changés → assets régénérés | règles dépendantes de la seule source ; `SOUNDBANK_BANK` est dans le tampon mais `soundbank.asm` n'en dépend pas : le conseil de `build.md:55` (« move it with `SOUNDBANK_BANK := 2` ») est sans effet en incrémental | lecture des règles (non reproduit pour `SPRITE_SIZE`) | 🟡 |
| S15 | `tools/tmx2snes/tmx2snes.c:511-516, 546` | ids > 1023, 2ᵉ tileset, rotation Tiled → erreur | `(tileattr - 1) & 0x03FF`, seul `map->tilesets` (le premier) est lu, `CUTE_TILED_FLIPPED_DIAGONALLY_FLAG` jamais testé | lecture du code, non reproduit | 🟡 |

## Périmètre couvert

- Lu : `make/common.mk` (714 l.), `lib/Makefile` (dépendances, l. 105-190), `templates/{memmap*.inc, hdr*.asm, assets.inc}`, `bin/cc65816`, `tools/sa1-patch/src/sa1_patch.c`, `tools/wav2brr/{README.md, src/main.c}`, `tools/smconv/{README.md, src/it2spc.c}`, `tools/tmx2snes/tmx2snes.c`, `docs/tools/build.md`, `release.yml:186-222`, `scripts/install-luna.sh:35-50`, `tools/fuzz/crashes/README.md`, l'annexe C du 2026-09-26.
- Construit dans le scratchpad : ~40 builds de `print_string`, `libtests_gsu`, `sa1_hello`, `dsp1_cube` sur `ROM_BANKS` ∈ {1, 2, 3, 12, 64, 65, 80, 127, 128, 200, 256} × LoROM/HiROM/SA-1/DSP-1, `GSU_BANK` ∈ {0, 2, 7, 9, x}, `GSU_RAM_KB` ∈ {0, 48, 1024, abc}, `ROM_NAME` spéciaux, bascule incrémentale LoROM→HiROM et `GSU_BANK` 1→2.
- Vérifié : en-têtes par `xxd`, sommes de contrôle des **103** `.sfc` du dépôt (`cks.py`), rendu par `luna diff` et `luna state --screenshot`.
- Outils : `make test-tools` (8 suites vertes), gfx4snes / font2snes / wav2brr / smconv sur entrées fabriquées (32 couleurs, 2048 et 512 tuiles, WAV stéréo/8/24 bits/44,1 kHz, IT à 12 canaux, fichiers non-IT).
- Corpus : 3 requêtes `snes_search` (SA-1, LoROM, DSP-1) avec `exclude_sources=["opensnes-docs","opensnes-notes-tech"]`. Hors périmètre : codegen (→ compilateur), contenu des tests luna (→ tests/CI).

## Points forts

- **Les 🔴 et la plupart des 🟠 de l'audit du 26/09 sont fermés, et vérifiés ici.** Liste des devtools du zip dérivée de `common.mk` (`Makefile:51`, `RELEASE_DEVTOOLS := … grep 'python3' make/common.mk`) et zip réellement construit et testé sur les 4 OS (`release.yml:208-220`, `make release-smoke`, Windows compris). Tampon de configuration `.opensnes_config` (`common.mk:489-509`) : `make GSU_BANK=2` reconstruit bien `[HDR] [AS] crt0 … [CC] main.c` (S3 excepté). En-têtes locaux suivis (`:473`). Lib : objets C dépendants de `snes/*.h`, asm des `.inc` et de `sm_spc.asm` (`lib/Makefile:118-121`). Lint NMI inconditionnel (`common.mk:666-679`). `install-luna.sh` multi-OS (`:35-50`).
- **Six combinaisons refusées avec la raison** au lieu d'une : coprocesseurs exclusifs, HiROM+SuperFX, HiROM+SA-1, HiROM+DSP-1, `GSU_BANK` sans `USE_SUPERFX` ou avec ≠ 1 fichier, `SRAM_SIZE` hors 1..7, `ROM_BANKS` non positif, `ROM_REGION` inconnu, `RAM_CODE_SIZE` hors 0..16384 (`common.mk:139-199, 215-217`). Module inconnu refusé avec la liste des modules (`:293-302`), `superfx` hors `USE_SUPERFX` aussi.
- **En-tête calculé et arbitré** : `$FFD6` composé (`$05` DSP-1+SRAM, `$15` GSU+SRAM, `:155-159`), `$FFD7` arrondi vers le haut (`:124-126`, `ROM_BANKS=12` → `$09`), `$FFD9` par `ROM_REGION`. 100 des 103 ROM du dépôt ont une somme de contrôle exacte (S6 pour les 3 autres).
- **Doc des boutons en un seul endroit et gardée** : `docs/tools/build.md` liste chaque `?=` ; ancre 12 de `check_doc_drift.py`.
- **Outils d'assets** : 8 suites golden vertes (`make test-tools` : gfx4snes 5/5, tmx2snes 2/2, smconv 11/11, wav2brr 2, palplan 3, aseprite2snes 2, font2snes 3/3, img2snes 2/2) ; smconv refuse désormais un non-IT (`'g.it' is not an Impulse Tracker module (no IMPM signature)`, rc 1) ; wav2brr refuse le 24 bits ; font2snes refuse une taille non multiple de 8 ; `gfx4snes -h` existe. 13 entrées de régression fuzz, dont 3 écrites à la main pour le décodeur IT compressé du 03/10.
- **Build rapide** : `print_string` propre 1,09 s, no-op 0,11 s (`/usr/bin/time`, 6 cœurs).

## Points faibles

1. 🔴 **`ROM_BANKS` n'a pas de borne haute par mapping** (S1). La ROM la plus grande qu'un utilisateur demandera (« je veux 4 Mo ») produit des données dans la WRAM, la BW-RAM ou hors 24 bits, et le build est vert ; `check_bank_reads.py` répond « OK ». Conséquence : du texte et des tables qui disparaissent sans un message, exactement la classe que les ratchets bank $00 ont été écrits pour interdire. Effort S.
2. 🔴 **`<stdint.h>` de l'hôte compile en silence avec de mauvaises tailles** (S2). `int32_t` sur 16 bits est le premier `#include` d'un développeur C. Le préprocesseur non épinglé n'est plus qu'une « note » de `verify_toolchain.py:194-204` ; le danger concret n'est écrit nulle part. Effort S.
3. 🟠 **Trois trous d'incrémentalité qui lient un objet périmé** (S3, S4, S14) : `.sfx.bin` / `.spc700.bin` sans le tampon ni leurs `.include` ; trois objets wrappés sans le tampon ; assets convertis sans leurs drapeaux. Aucun objet n'a `$(CC)`/`$(AS)` en prérequis : après une mise à jour du compilateur, `make` relie l'ancien code (documenté seulement comme « make clean && make », `GETTING_STARTED.md:297`). Effort S.
4. 🟠 **La lib prébuilt fixe la banque 7** (S5) : le bouton `ROM_BANKS` est faux sous 8, et l'en-tête ment sur la taille. Effort S (refuser `< 8`) ou M (lib assemblée avec la plage du projet).
5. 🟠 **Somme de contrôle fausse sur toute ROM SA-1** (S6), depuis que `sa1_patch` existe. Bénin sur console, visible dans tout émulateur qui vérifie la somme, et invisible pour nous parce que luna ne recalcule pas. Effort S.
6. 🟠 **Les convertisseurs perdent des données sans erreur** (S7, S8, S9, S10, S12). Les goldens ne testent que le chemin heureux (gfx4snes : 5 cas, tous mode 1 4bpp ; aucun `-M 5/6/7`, 2bpp, 8bpp, `-k`, `-y`, `-f`, aucun cas négatif hors palplan/aseprite2snes) et le fuzz ne cherche que des plantages : la perte sémantique n'est couverte par rien. Effort S par outil.
7. 🟠 **`GSU_RAM_KB` et les shells Python dans le Makefile** (S11) : une erreur Python dans un `$(shell …)` imprime un traceback et laisse une variable vide qui devient `$00` dans l'en-tête. Même motif potentiel pour `ROMSIZE` (`:126`) si `ROM_BANKS` passait la garde. Effort S.
8. 🟡 **Bruit qui contredit « zéro avertissement »** : 6 `WARNING: There is a SLOT number 0, but there also is a SLOT (with ID 1) with starting address 0` dans le dernier build complet (`scratchpad/build23.log`, `grep -c` = 6) et 3 à chaque build de `libtests_gsu` — le `SLOT 0` de `GSU_SECTION` (`assets.inc`) et des fenêtres RAM code. Effort S.
9. 🟡 **Bornes documentées non appliquées** : `GSU_BANK=0` accepté (`rc=0`) alors que `build.md:65` dit 1..`ROM_BANKS`−1 ; `ROM_NAME` > 21 caractères tronqué, UTF-8 (`CAFÉ` → `C3 89`) écrit dans l'en-tête, `&` et `/` donnent des erreurs cryptiques de `sed` / WLA (`common.mk:514`). Effort S.
10. 🟡 **Ergonomie restante de l'audit précédent** : `doctor` ne vérifie toujours ni `python3`, ni `clang`, ni `wla-superfx`/`wla-spc700`/`sa1_patch`/`wav2brr` (`scripts/opensnes:441`) ; gfx4snes `-z` décrit comme `-b`, `-Y` libellé `--meta-width`, PNG RGB → « png decoder error 82 » ; `hdr_hirom.asm:62` commente `$21=ROM, $23=ROM+SRAM` pour un octet qui vaut `$00`/`$02` ; l'exemple de `tools/wav2brr/README.md:30` utilise `superfree` au lieu d'`ASSET_SECTION`. Effort S.

## Risques

- **La campagne de défauts silencieux ne regarde pas encore `make/` et `tools/`** : le journal (`silent_defects_log.md`) n'a que des défauts lib/runtime ; les S1-S11 ci-dessus sont tous dans son champ (« `lib/`, `templates/`, `compiler/` ou `make/` ») ou à sa frontière (outils). Quinze candidats en une journée dans ce seul périmètre dit que le critère 7 n'est pas proche.
- **luna rend vert ce que la console rendrait faux** : il mappe le fichier entier (S5 passe `MATCH`) et ne recalcule pas la somme (S6 `checksum_valid: true`). Un oracle « header vs fichier » côté luna (ou dans `symmap.py`) manque — à remonter dans `partners/luna/OPEN_luna.md`. Côté snes-rag : les requêtes « SA-1 memory map » et « LoROM $7E-$7F » ne renvoient **aucun arbitre** (vitor *solid*, wikibooks *complement*) — à remonter aussi.
- **Les combinaisons ne sont testées qu'à l'unité** : aucun exemple n'utilise `ROM_BANKS`, `GSU_RAM_KB`, `ROM_REGION=jp`, ni une bascule incrémentale ; la matrice des boutons n'a pas de test (seul `test-link-modules` balaie un axe).
- **Le prochain outil appelé par `$(shell python3 …)`** reproduira S11 tant que le motif « traceback → variable vide → build vert » existe.
- **`smconv` hérite d'avertissements formatés en « error »** (S9) : l'utilisateur apprend à ignorer le rouge.

## Améliorations recommandées

| # | Action | Sévérité traitée | Effort | Premier pas concret |
|---|---|---|---|---|
| 1 | Borner `ROM_BANKS` par mapping : LoROM ≤ 126 (ou passer les assets en banques `$80+` au-delà), HiROM ≤ 64, SA-1 ≤ 64, Super FX ≤ 64, DSP-1 ≤ 32 ; et ≥ 8 tant que la lib fixe « 7-1 » | 🔴 S1, 🟠 S5 | S | Bloc `$(error …)` après `common.mk:199`, une ligne par mapping, raison citant la carte (fullsnes `57eb36348d575212` pour DSP-1) |
| 2 | Préprocesseur : `-nostdinc -undef` + macros cible explicites, et un `stdint.h` SDK (`int32_t` = `long`, etc.) dans `lib/include` ; documenter dans `KNOWN_LIMITATIONS.md` | 🔴 S2 | S | `bin/cc65816:86` : `cc -E -nostdinc -undef -D__OPENSNES__=1 …` ; comparer les `.c.asm` du corpus avant/après |
| 3 | Tampon `.opensnes_config` en prérequis de `%.sfx.bin`, `%.spc700.bin`, `data_init_*.o`, `$(SOUNDBANK_OUT).asm/.o`, des règles GFX ; `$(CC) $(AS) $(LD)` en prérequis des `.o` (lib et projets) ; dépendances `.include` des `.sfx`/`.spc700.asm` par grep comme `INCBIN_DEPS` | 🟠 S3, S4, S14 | S | `common.mk:402` : `%.sfx.bin %.sfx.h: %.sfx .opensnes_config` ; `:527,555,549` : ajouter `.opensnes_config` |
| 4 | `sa1_patch` recalcule somme et complément après l'OR | 🟠 S6 | S | Dans `sa1_patch.c`, après `fputc`, ajouter 3 (le delta) à `$7FDE` et l'inverse à `$7FDC` ; contrôle : `cks.py` sur les 3 ROM SA-1 |
| 5 | Un test d'en-tête dans la CI : somme de contrôle + `$FFD7` ≥ taille du fichier + aucun symbole dans une banque non-ROM du mapping, pour chaque `.sfc` | 🟠 S1, S5, S6 | S | Étendre `symmap.py --check-overlap` (déjà bloquant dans `release.yml:186-191`) ; capable de voir S1 à la place de luna |
| 6 | Convertisseurs : erreur (exit ≠ 0) sur > 1024 tuiles BG, > 256 tuiles mode 7, index ≥ 2^bpp, > 8 canaux IT ; wav2brr : avertissement toujours imprimé au-delà de 32 kHz et sens corrigé (« plus grave »), ou rééchantillonnage | 🟠 S7-S10, 🟡 S12 | S | `tools/smconv/src/it2spc.c:337` : propager l'échec jusqu'au `main` ; un golden négatif par cas (fixtures `big.png`, `m7b.png`, `ch12.it` du scratchpad) |
| 7 | Remplacer les `$(shell python3 -c …)` de calcul par de l'arithmétique shell validée, ou vérifier la sortie non vide ; valider `GSU_RAM_KB` ∈ {puissances de 2, 2..128} | 🟠 S11 | S | `common.mk:130` : `ifeq ($(GSU_RAM_SIZE_VAL),) $(error …)` + arrondi vers le haut comme `ROMSIZE` |
| 8 | Remonter aux partenaires : luna (`checksum_valid` ne recalcule pas ; pas d'avertissement quand `$FFD7` < taille), snes-rag (cartes mémoire SA-1 / LoROM sans arbitre) | Risques | S | Une ligne dans `partners/luna/OPEN_luna.md` et `partners/snes-rag/OPEN_snes-rag.md` avec les ROM et commandes de ce rapport |
| 9 | Inscrire S1-S11 au journal `silent_defects_log.md` au fil des correctifs | Risques | S | Une ligne par défaut, date 2026-10-03, « audit C » |
| 10 | Nettoyages 🟡 : `SLOT 0` → `SLOT 0` sans ambiguïté (ou `.SLOT` nommé) ; `GSU_BANK=0` refusé ; `ROM_NAME` validé (ASCII `$20-$7E`, ≤ 21, échappement `sed`) ; `doctor` complet ; libellés gfx4snes ; commentaire `hdr_hirom.asm:62` ; exemple `ASSET_SECTION` dans le README wav2brr | 🟡 8-10, S13, S15 | S | `common.mk:514` : `sed "s|__ROM_NAME__|…|"` avec échappement de `&` et `|`, et `$(error)` si `ROM_NAME` contient un octet hors ASCII |

## Verdict

Le système de build a fermé en une semaine presque tout ce que l'audit du 26/09 relevait : zip testé sur 4 OS, tampon de configuration, combinaisons refusées, en-tête arbitré, lib et en-têtes locaux suivis. Mais la chasse aux silences montre que la discipline « refuser proprement » s'arrête aux combinaisons déjà rencontrées : `ROM_BANKS` hors de 8..64, `<stdint.h>`, une bascule incrémentale, `GSU_RAM_KB=0` et quatre convertisseurs produisent des ROM ou des assets faux avec un build vert, et deux de ces défauts (S1, S2) touchent un utilisateur dès sa première ROM ambitieuse. Rien de cela n'est profond (chaque correctif est d'effort S), mais tant qu'aucun test d'en-tête et aucun golden négatif n'existent, ce périmètre n'est pas au niveau d'un gel 1.0.

## Suivi (2026-10-04)

- **S7, S8, S12 corrigés** (commit `fix(tools): gfx4snes refuses…` de ce
  jour) : plus de 1024 tuiles BG, plus de 256 tuiles Mode 7, et — pour les
  planches sans carte (sprites, fontes) — une tuile 2bpp/4bpp dont les
  pixels viennent de deux banques de palette sont refusés avec le numéro
  de tuile, d'entrée ou de pixel. Trois fixtures générées (`toomany_bg.png`,
  `toomany_m7.png`, `index5_2bpp.png`) les épinglent dans `run_golden.py`
  (table `REFUSED`). Le refus a attrapé un défaut du corpus :
  `games/mode7_flying` avait 379 tuiles distinctes pour une carte Mode 7
  (123 entrées repliées modulo 256 depuis la création de l'exemple), corrigé
  dans le générateur.
- **Avec carte, la banque est un choix** : l'entrée de carte porte la
  palette (celle du premier pixel de la tuile, `maps.c:222`) et les plans
  gardent les bits bas ; `color/transparency`, `scrolling/mixed_scroll` et
  `hdma/hdma_wave` utilisent plusieurs palettes de 16 couleurs ainsi. Une
  première version du refus, appliquée à tout pixel ≥ 2^bpp, les a arrêtés :
  la vérification reste sur le chemin sans carte.
- **Question ouverte sur `-a` (`--pal-rearrange`), non mesurée** :
  `gfx4snes.c:172` copie l'image dans `tiles_snes` (`tiles_convertsnes`
  rend un tampon `malloc`), puis `:177` `palette_rearrange_snes` réécrit
  les indices de `snesimage.buffer` (`palettes.c:180,348`), et `:181`
  `map_convertsnes` consomme `tiles_snes`, copié *avant*. Si la lecture est
  juste, la réorganisation des indices n'atteint jamais les tuiles écrites
  (seule la palette `.pal` serait réordonnée). `transparency` (`-a -u 16`,
  19 couleurs) a une image plausible, ce qui ne tranche pas (PVSnesLib a le
  même ordre). À vérifier avec une image dont le réarrangement changerait
  visiblement les indices, avant d'en faire un défaut.
- **Question ouverte sur la palette d'une tuile de carte, non mesurée** :
  `maps.c:222` prend la banque de palette du **premier** pixel de la tuile
  (`imgbuf[currenttile * sizetile] >> 4`). L'indice 0 est la couleur
  transparente de toutes les banques : une tuile de banque 2 qui commence
  par un pixel transparent recevrait la palette 0 dans l'entrée de carte, et
  ses pixels (bits bas conservés) s'afficheraient avec les couleurs de la
  palette 0. Le refus des planches sans carte a dû apprendre ce cas (la
  banque est celle du premier pixel *opaque*, sinon un sprite de banque 2 à
  coin transparent était refusé) ; la carte a le même angle mort, hérité de
  PVSnesLib. À mesurer avec une image dont une tuile de banque ≠ 0 commence
  par la couleur 0, avant d'en faire un défaut.
- **S13 corrigé** (même commit que G PF9) : l'en-tête SA-1 suit `USE_SRAM`
  (`$34` / `$35`) et `SA1_BWRAM_SIZE` (`$FFD8`), documenté sur
  `docs/tools/build.md` ; `SRAM_SIZE` ne s'applique pas au SA-1, la page le
  dit.


## Suivi 2026-10-05 (session)

- **Rec 5 (test d'en-tête)** : la passe `luna_runner.py --coverage` échoue
  un exemple dont l'octet de taille `$FFD7` ne couvre pas le fichier ou
  dont le complément de somme ne correspond pas, à partir du bloc `rom`
  que `luna state` renvoie déjà (zéro coût : pas de lecture
  supplémentaire). Contrôles : un dict forgé (128 Ko / 262 144 octets) et
  une copie de `print_string.sfc` avec `$FFD7 = $07` sont refusés ; le
  corpus (89 + les fixtures) passe. La somme elle-même n'est pas
  recalculée : c'est à luna (`checksum_valid` ne compare que
  somme ⊕ complément — mesuré, un octet inversé en `$0100` reste
  « valide »), ligne ajoutée à `OPEN_luna.md` avec la commande. « Aucun
  symbole dans une banque non-ROM » reste à faire dans `symmap.py`.
- **Rec 8 (lignes partenaires)** : luna, `checksum_valid` ci-dessus ;
  snes-rag, la carte mémoire SA-1 était déjà dans `OPEN_snes-rag.md`
  depuis le 10-04.
- **Rec 5, suite (même jour)** : luna a ajouté `rom.checksum_computed` sur
  son `develop` (`39359de`) le jour même de la demande ; `header_problem`
  compare l'en-tête à cette somme dès que le champ existe (absent sur la
  v1.32.0 épinglée : ignoré). Contrôle sur leur binaire : la copie altérée
  de `print_string.sfc` est refusée (`0xAF40` ≠ `0xB01F`), l'originale
  passe, le corpus passe. La somme est donc couverte à la prochaine
  épingle ; reste « aucun symbole dans une banque non-ROM ».
