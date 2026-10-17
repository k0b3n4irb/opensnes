# Audit 1.0 — synthèse

2026-10-17 · `develop` @ `73a3fdfe` (v0.50.0 + `[Unreleased]`) · luna v1.38.0 · 13 rapports d'experts (`01` à `13`) et 3 vérifications adversariales (`verif_groupe_1..3.md`), tous dans ce dossier.

Cette synthèse ne remplace pas les rapports. Chaque ligne renvoie à un identifiant (`NN-k`) où l'on trouve la preuve complète et la commande qui la rejoue. Les sévérités sont celles qui restent **après** la vérification adversariale, pas celles des experts.

## 1. Verdict global

**OpenSNES n'est pas prêt pour une 1.0 aujourd'hui.** Ce n'est pas d'abord un problème de qualité du code. Trois portes du propriétaire ne sont ni faites ni datées : session console, jeu vitrine, provenance des assets. Et le contrat que la 1.0 gèlerait (`docs/STABILITY.md`, `compiler/ABI.md`, les en-têtes) est écrit plus large que ce qui est prêt à être gelé.

**Ce qui est réellement solide, et mesuré :**

- **Le runtime matériel.** Pire NMI mesuré à 8 568 mclk, soit 71 % du plafond. Aucune écriture PPU hors blank sur 85 ROM au repos ni sur 119 parcours pilotés. 108 images identiques au pixel sous RAM aléatoire. Les 12 affirmations matérielles échantillonnées sont confirmées par un arbitre (01).
- **Le compilateur.** Les quatre 🔴 du 03/10 sont corrigés et épinglés. 13 500 programmes et 12 000 expressions aléatoires passent, et une fenêtre de chasse jamais parcourue passe aussi. Les refus sont nets et épinglés (16/16) (04).
- **Le filet de non-régression.** `make tests` tourne en 4 min 31 s, de façon déterministe. Les portes échouent sur des défauts injectés, la couverture d'API est de 344/344, et les 85 exemples ont tous un manifeste (09, 11).
- **Le build.** Le passage en incrémental donne les mêmes octets qu'un build propre, les ROM sont reproductibles, aucun interpréteur ne tourne dans un build utilisateur, et les 165 cas golden des outils passent (12).
- **Le premier contact par `opensnes init`.** Un projet jouable et testé en moins de 5 minutes, avec des budgets visibles à chaque build (05).
- **La chaîne d'approvisionnement.** Les 41 `uses:` sont épinglés par SHA, luna est vérifié par SHA-256, aucun secret n'est dans l'historique, et les parseurs fuzzés font 0 plantage sur plus de 100 M d'exécutions (07).
- **Les chiffres publiés.** Ils se reproduisent à l'unité : −37,5 % en statique, −62,3 % mesuré (08).
- **La documentation.** Doxygen strict à 0 avertissement, 334 fonctions publiques sur 337 documentées, et 515 appels d'exemples à la bonne arité (10).

**Ce qui ne l'est pas :**

- **Cinq défauts silencieux de code sont ouverts**, dont deux 🔴 :
  - un VLA compile en corruption de pile (04-1) ;
  - la lib prend le canal DMA 1 deux fois, ce qui laisse l'écran noir en Super FX avec letterbox et texte (01-1) ;
  - une copie de struct depuis un `const` est faite en banque $00 (04-2), et le contrôle qui la rattrape s'efface au second `make` (04-3) ;
  - un `.spc` est mal agencé, et un débordement d'ARAM passe sur un simple avertissement (03-1, 03-2).
- **Le tableau de bord du critère 7 est faux** (06-2). Avec 04-1 consigné aujourd'hui, la fenêtre de deux semaines ne peut pas fermer avant le **2026-10-31**.
- **Rien n'a tourné sur une console** (09-1, 02-4). Tous les oracles interrogent luna seul. Et luna ne modélise pas les révisions de CPU, alors que deux errata touchent justement le HDMA vers INIDISP du SDK (01-2).
- **La distribution n'est pas conforme.** 56 fichiers PVSnesLib, dont des personnages Nintendo, sont publiés sans attribution, et `ROADMAP.md:53` affirme le contraire (11-1/13-1). S'y ajoutent des musiques tierces, des marques, et des notices QBE, cproc et glibc absentes du zip (13-2 à 13-5).
- **La promesse de stabilité gèlerait des choses non prêtes** : des modules « Experimental » (02-3), des variables internes (05-7), un `ABI.md` faux sur cinq points (04-4), un piège reconnu (`oamInit`, 05-5), un blob SNESMOD sans source (03-3), et des outils absents de la promesse (12-5).

**Ordre de grandeur, à titre d'estimation.** Les défauts de code et de doc tiennent en environ trois semaines de travail du mainteneur, presque tout en effort S. Ce qui fixe la date, ce sont les décisions et les actes du propriétaire (assets, vitrine, session console) et la fenêtre du critère 7. Aucune date de 1.0 n'est défendable tant que ces trois portes n'ont pas de date.

## 2. Constats bloquants confirmés

**Bilan de la vérification.** 57 constats 🔴/🟠 ont été soumis à la vérification. **Aucun n'est réfuté** et aucun n'est non rejouable.

| Groupe | Constats | Confirmés | Partiels | Réfutés | Taux de réfutation |
|---|---|---|---|---|---|
| 1 — matériel, puces, audio, bench | 13 | 10 | 3 (01-3, 08-1, 08-3) | 0 | 0 % |
| 2 — compilateur, sécurité, tests, build | 17 | 15 | 2 (09-3, 12-1) | 0 | 0 % |
| 3 — SDK, organisation, docs, exemples, licences | 27 | 20 | 7 (05-6, 05-7, 06-1, 06-8, 10-1, 11-5, 13-4) | 0 | 0 % |
| **Total** | **57** | **45** | **12** | **0** | **0 %** |

Le bilan écrit en bas de `verif_groupe_3.md` dit « 19 confirmés / 8 partiels ». Recompter les lignes de son propre tableau donne 20 et 7. C'est le décompte des lignes qui est retenu ici. La ligne 09-5 vient d'une passe antérieure et n'entre pas dans le compte : partielle, elle est ramenée à 🟡.

Un taux de réfutation nul ne veut pas dire que tout est exact. Les vérificateurs ont **déclassé** six constats et **corrigé la portée** de six autres. Les déclassements sont listés après le tableau, avec leur raison.

Le tableau est trié par sévérité, puis par effort (S < S-M < M < L). Les doublons signalés par les vérificateurs sont fusionnés sur une seule ligne. « Non bloquant » veut dire que le constat est réel, mais qu'il ne conditionne pas le tag.

| id | domaine | constat | sévérité après vérification | effort | preuve |
|---|---|---|---|---|---|
| 01-1 | lib / templates | Le HDMA INIDISP du GSU (canal 1) et les DMA du NMI (`tilemapFlush`, `oamVramQueueUpdate`) partagent le canal 1. Résultat : écran noir sans signal, alors que `superfx.md:266` promet que la combinaison marche. **Portée plus large** : `oamVramQueueUpdate` prend aussi les canaux 2 à 4, que `hdma.h` présente comme sûrs (lecture de code, non mesuré). | 🔴 | S | Variante `superfx_game_skeleton` : 0 pixel non noir sur 57 344 à l'image 300. Le contrôle négatif donne `ec9139c7…`. BBAD1 passe de `$00` à `$18` dans le NMI (PC `$00:8704`) |
| 04-1 | compilateur | Un VLA compile sans diagnostic : le tableau devient un pointeur dans la frame fixe de 4 octets, et la pile est corrompue. `__STDC_NO_VLA__` n'est pas défini. Et `STABILITY.md:50-52` interdit de le refuser après la 1.0. | 🔴 | S | `vla(6)` rend `8586` au lieu de `1500` sur luna. `tsa/clc/adc.w #2` (`emit.c:6058-6075`) |
| 06-2 | gouvernance | Le tableau de bord du critère 7 est faux : il manque `fb429433` (mauvaise compilation livrée dans v0.50.0), et la semaine 2 n'est pas consignée. Il affiche « ≥ 10-22 » ; ses propres règles donnent ≥ 10-29, et ≥ 10-31 avec 04-1. | 🔴 | S | `grep -c fb429433 silent_defects_log.md` → 0 (rejoué aujourd'hui sur l'arbre principal). `v0.50.0:compiler/PINS.md:50` |
| 11-1 / 13-1 | licences / exemples | 56 fichiers sous `examples/` (19 exemples, 906 Ko) sont identiques à PVSnesLib, dont mario, goomba, koopa, `overworld.it` et `mariojump.brr`. Ils sont publiés dans `opensnes-examples_<v>.zip`, ROM comprises, sous MIT, sans attribution. `ROADMAP.md:53` dit « listed in ATTRIBUTION.md », ce qui est faux. Et la note du 10-05 contredit la ROADMAP. | 🔴 | S (liste et ROADMAP) / L (remplacement) | md5 contre `~/workspace/pvsneslib` (2 977 empreintes). `grep -ciE 'nintendo\|mario\|goomba\|koopa' ATTRIBUTION.md` → 0 |
| 06-3 / 11-2 | gouvernance | La porte « jeu vitrine (pas un portage) » n'a aucun candidat publiable. Le RPG n'a plus de code depuis le 08-18 et n'a pas de remote ; l'autre jeu actif est un projet tiers sous licence. Aucune des trois portes du propriétaire n'est datée. | 🔴 | S (décision) / L (livraison) | `git -C ~/workspace/opensnes-rpg log` (`32762de`, 2026-08-18). `ROADMAP.md:52` « In progress » |
| 09-1 / 02-4 | matériel / tests | Aucune ROM n'a tourné sur une console, et chaque oracle interroge luna seul. La porte ne retient que les rangées 1 à 7 ; les puces (16-18, 24-26) n'y sont pas. luna valide aussi MS0 à 21 MHz, que le matériel interdit, et le compteur `ms0_at_21mhz` est absent de la v1.38.0. | 🔴 | L (propriétaire) | `HARDWARE_VERIFICATION.md:170` « No session recorded yet ». `ROADMAP.md:17-18,51`. fullsnes `1adef8e33ff3c4e9` |
| 01-2 | lib / matériel | Les HDMA vers INIDISP (`gsuSetupHdmaBlanking` en mode 0 vers BBAD `$00`, `hdmaBrightnessGradient`) ont la forme que deux errata déconseillent. Sur S-CPU-A, une DMA échoue en silence ; sur 1-CHIP, on obtient un fondu au bord de chaque bande. luna ne peut pas le voir. | 🟠 | S | snesdev-wiki `97b76d7d4e2e34a6`, undisbeliever `8a82d25de6e99ae5`, `172b5b77ea6fae37`. DMA VRAM dans les bandes sur 160 images sur 160 |
| 01-3 | protocole matériel | Le protocole ne relève pas la révision des puces, le preflight ne compare pas d'image, et aucune rangée ne démarre port 2 vide. **Corrigé par la vérification** : le 1-CHIP est déjà demandé (`HARDWARE_VERIFICATION.md:19`), et `$4210` ne distingue pas S-CPU-A (il rend 2 pour A, B et 1-CHIP). Il faut relever la sérigraphie de la carte mère. | 🟠 | S | snesdev-wiki `274c2cef6c5b0e1c`. `hardware_preflight.py:23` |
| 02-1 | doc / link | Le tutoriel SA-1 enseigne un `sa1_boot.asm` en `SUPERFREE`. Le build est vert, mais le SA-1 ne démarre pas dès que la banque $00 déborde, ce qui est le comportement normal d'un gros jeu. Aucun garde de link ne vérifie la banque. | 🟠 | S | `SA1Start` à `01:8000` ; fbhash `4c699f3f…` contre `7aee1820…` |
| 02-2 | API puces | `sa1.h` expose en macros C des registres « SA-1 side » (MCNT, MA, MB, MR, OF, CFR, HCR…). Depuis le S-CPU ils ne font rien, et ils seraient gelés. | 🟠 | S | On lit `00 00 00 00` au lieu de `$00012340`. fullsnes `396f57a8c8d7223d` |
| 02-3 | contrat | `STABILITY.md` gèle sans exception SA-1 et Super FX, que le README déclare « Experimental », ainsi que la configuration GSU par variables globales (`gsu_cfgr`, `gsu_scmr_live`…). | 🟠 | S (décision) / M (refonte) | `README.md:93-94` contre `STABILITY.md:15-52` |
| 03-1 | outil audio | `opensnes-music spc` écrit le module à `$19AA`, alors que le pilote le lit à `$1A00`. Le `.spc` de prévisualisation est probablement muet (non joué : luna ne joue pas un `.spc`). | 🟠 | S | ARAM `$1A00` = `ffff…`. `it2spc.c:21` contre `:37` |
| 03-2 | outil audio | `opensnes-music bank` accepte un module plus grand que l'ARAM : un avertissement, puis l'écriture, puis le code 0, contrairement à son `--help`. Le refus n'agit qu'avec `-v`, et la limite part de `$18CA`. | 🟠 | S | `main.c:160-168`, `it2spc.c:610,642` |
| 03-3 / 06-11 | audio / contrat | La 1.0 gèlerait `snesmod*` et le format de soundbank au-dessus d'un pilote sans source (5 522 octets en `.byte`, patchés à la main). L'outil en embarque un autre binaire (5 546 octets). La décision demandée au 03/10 (H PF17) n'est pas prise. | 🟠 | S (décision) / L (désassemblage) | 4 854 octets différents à offset égal. `reste_a_traiter.md:97` |
| 04-2 | compilateur | Affecter une struct, une union ou un élément de tableau de structs depuis un `const` nommé lit en banque $00 (`lda.w`) : `funccopy` n'émet aucun drapeau d'accès. | 🟠 | S | `lda.w kl.kc` ; `r_a` lu `008db101…` |
| 04-3 | build | Le `.sfc` est écrit avant les contrôles post-lien, sans `.DELETE_ON_ERROR`. Le second `make` dit « Built », rc 0, sur une ROM fausse. | 🟠 | S | Un premier `make` donne Error 1, le second rc 0 (copie de `starter/`) |
| 04-4 | contrat | `ABI.md`, que la 1.0 gèle, affirme cinq choses fausses : extension de `u8`/`s8`, retour de struct par pointeur caché, `long long`, floats logiciels. | 🟠 | S | `lda.w g16 / pha`. Les trois autres constructions sont refusées. `make lint-docs` est vert |
| 05-1 | doc / build | Le chemin « Manual setup » de GETTING_STARTED ne construit pas : le commentaire laisse des espaces dans `OPENSNES`, `USE_LIB` manque, et `USE_LIB` est absent du tampon de configuration. | 🟠 | S | Rejoué mot pour mot ; `common.mk:620` |
| 05-2 | CLI | Le Makefile généré par `init` écrit le chemin absolu du SDK avec `:=`. Le projet casse quand le SDK bouge ou quand un coéquipier clone. | 🟠 | S | `main.c:378` ; « No rule to make target …/common.mk » |
| 05-3 | build | Un module oublié donne l'erreur brute de wlalink, sans nommer le module à ajouter. Le commentaire de `snes.h:130` est faux depuis `-d`. | 🟠 | S | « Reference to an unknown label "collideRect" » |
| 05-5 | API | `oamInit` prend un numéro de page, alors que tous les autres paramètres VRAM prennent une adresse. Une adresse passée par erreur est masquée sans diagnostic. Après la 1.0, le corriger serait un changement de sens. | 🟠 | S | `sprite.h:215-240` ; `starter/main.c:45` |
| 05-7 | contrat | La promesse sur les `extern` est ambiguë. **Portée corrigée** : `gsu_cfgr` est un exemple voulu, `objWorkspace` est public. Les `vram_queue_*`, `text_config` et `dsp1_o*` demandent une liste blanche. | 🟠 | S | `STABILITY.md:21-24` ; 35 `extern` |
| 06-9 | gouvernance / IP | Des notes publiées décrivaient un projet tiers sous licence (corrigé le 2026-10-17 : retirées, historique réécrit). `origin/main` publie 300 fichiers de `.claude/`. | 🟠 | S | — |
| 07-1 | outils / sécurité | `opensnes-level` : un nom de calque déborde un tampon global et écrit hors de `--out`. | 🟠 | S | Calque `../escape/PWN` : rc 0, fichier écrit hors du dossier. ASan : global-buffer-overflow |
| 07-3 | CLI / sécurité | Le CLI passe des chemins à `system()` et `popen()`. Un dossier `$(…)` exécute une commande. | 🟠 | S | Le fichier `PWNED` est créé |
| 08-1 | bench / communication | Le banc « jeu entier » (twinbench) montre 4 paires sur 22 plus lentes : dynamicmetasprite +23,7 %, mapandobjects +15,0 %, slopemario +11,5 %, mapscroll +3,3 %. Aucune page ne le publie. **Portée corrigée** : le « 61 % » est bien circonscrit au C. | 🟠 | S (publier) / M-L (combler) | twinbench rejoué au cycle près. `grep twinbench docs ROADMAP.md README.md` → rien |
| 08-2 | tests | `nmi_budget.py` mesure `oamVramQueueUpdate` à 312 mclk, alors que `_vqu_16x16` atteint 7 904. Aucune ligne ne mesure le VBlank cumulé (fusion avec 01-4). | 🟠 | S | `luna profile dynamic_sprite --from-frame 60 --until-frame 252 --top 0` |
| 09-2 | CI | Les jambes Windows et macOS ne lancent que la vivacité : ni `--compare`, ni manifestes, ni comparaison des sha256 des ROM avec Linux (doublon de 12-18). | 🟠 | S | `opensnes_build.yml:168-187` |
| 10-1 | doc | GETTING_STARTED A4 envoie vers `examples/text/print_string/print_string.sfc`, qui n'est pas dans l'archive (les ROM sont dans `examples/bin/`). `release_smoke.py` n'ouvre pas cette archive. | 🟠 (🔴 déclassé : la ROM existe, et la correction tient en une ligne de doc) | S | Recette `release-examples` rejouée |
| 10-2 | doc | Le guide de migration PVSnesLib (un must-have de la porte) prescrit `textFlush` (2 048 octets, 16 384 mclk de VBlank) alors que le NMI vide seul le tampon, et il dit « deprecated » de `LzssDecodeVram`, qui est retiré. L'ancre 16 exempte cette page. | 🟠 | S | MIGRATING:32,132,142 contre `text.h:198-214` |
| 10-3 / 13-6 | licences / doc | Aucune page ne dit à un studio ce que sa ROM contient (lib et crt0 MIT, code Alekmaul, moteur d'undisbeliever, SNESMOD sans texte de licence) ni quelle notice porter. | 🟠 | S | grep licen/commercial/notice dans FAQ, GETTING_STARTED et README : rien |
| 10-4 | contrat | La promesse de dépréciation repose sur la passe clang, que `common.mk:602` saute en silence et que `doctor` déclare optionnelle. | 🟠 | S | Fonction `OPENSNES_DEPRECATED` : rc 0, aucun avertissement |
| 11-4 | exemples | Le README de breakout, le « jeu de synthèse », enseigne `REG_INIDISP`, `pad_keys[0]` et `blockmap` à `$0800`, qui n'existent plus dans le code. `bg2map1-3` (6 Ko) ne sont jamais lus. | 🟠 | S | README:44, 91-93, 215 contre `main.c` |
| 12-2 | build | Le premier build sort en « Error 1 » sans message quand le dernier réglage trié n'écrit pas de `_data.as`. Le second réussit. | 🟠 | S | `res/zpal.toml` : rc 2, puis rc 0 |
| 12-4 | build / doctor | Un SDK dans un chemin contenant un espace casse tout build, alors que `doctor` répond OK. | 🟠 | S | « Makefile:23: …/sp: No such file » |
| 12-5 | contrat | `STABILITY.md` ne gèle ni les lignes de commande `opensnes-*`, ni les clés TOML, ni le schéma `--json`, ni les codes de sortie. | 🟠 | S | grep toml/settings/json/exit → aucune ligne |
| 12-6 | starter | Le modèle de CI du starter télécharge `releases/latest` sans épingle ni somme de contrôle, avec des actions désignées par étiquette mobile, et sans `make test`. | 🟠 | S | `starter/.github/workflows/build.yml` |
| 13-3 | licences | Des marques servent de noms d'exemples et de ROM : `tetris` / OPENSNES TETRIS, LIKEMARIO, SLOPMARIO, SHMUP 1942. | 🟠 | S | grep `ROM_NAME` |
| 13-4 | licences | La glibc est liée en statique dans les 19 binaires Linux, sans notice ni texte LGPL. **Portée corrigée** : la reliaison est possible par les sources publiées ; il ne manque que la notice et le texte. | 🟠 | S | `file bin/*` → 19/19 « statically linked » |
| 13-5 | licences | Les notices MIT de QBE et ISC de cproc ne sont pas dans le zip (doublon de 12-12). | 🟠 | S | `Makefile:667-677` |
| 12-3 | build | `wla-65816`, `wlalink` et les `opensnes-*` sont hors de `COMPILER_ID` : une mise à jour de ces outils n'atteint pas un projet existant, et son build reste vert. | 🟠 | S-M | Un octet ajouté : 0 étape relancée (contre 9 pour `qbe`) |
| 07-2 | outils / sécurité | Le décodeur BMP ne compare jamais une taille à `insize`, calcule `w*h` en `unsigned` sans contrôle, et le chemin RLE8 écrit `*image[pos++]`. Ni fuzz ni fixture. | 🟠 | S-M | Lecture de code, sans PoC, sans ambiguïté (`loadbmp.c:224,272,288,345`) |
| 13-2 | licences | Des modules musicaux tiers sont livrés sans licence : `tetris_theme.it` (@CheapAlert, 2005) dans le zip et la ROM, `overworld.it` (PVSnesLib, thème Mario probable). `whatislove.it` est dans le dépôt seulement. | 🟠 | S-M | `strings tetris_theme.it` |
| 05-6 | API | Les incohérences de nommage seraient gelées : `WaitForVBlank`, `dmaFillVRAM`/`dmaCopyVram`, `objCollid*`, `consoleMesenBreakpoint`, `t_*`. **Portée corrigée** : `oamGetY` en u8 suit le matériel, et les deux polarités de retour sont défendables. | 🟠 | M | grep des prototypes |
| 09-3 | tests | Les références capturent le comportement, pas la spécification, et rien ne refuse une re-capture de masse : le bug `oamSet` y−1 a vécu six mois sous 33 images. **Portée corrigée** : `vram_dma_blank.py` est dans `make tests`. | 🟠 | M | `git show --stat caa73986` |
| 09-4 | tests | Les oracles WRAM et audio mesurent la phase, pas le défaut : `wram.json` est re-capturé dans 69 commits sur 304, avec 0 trouvaille sur 80. Un délai inaudible fait échouer le hash, alors que `luna diff --audio` donne MATCH à 0,15 %. | 🟠 | M | `git log --since=2026-10-05` ; sonde `echo` |
| 11-6 | exemples / règle | Le « RPG template » exige `python3 gen_assets.py` pour éditer sa carte ; 9 scripts `.py` sont lancés à la main dans `examples/`. | 🟠 | M | `rpg/README.md:91` |
| 06-1 | gouvernance | Le lint d'identité refuse tout auteur externe, sur les PR et dans le hook de `CONTRIBUTING.md`. Facteur bus : 1. | 🟠 (🔴 déclassé : c'est une décision écrite du propriétaire ; à trancher, pas bloquant pour publier) | S (décision) / M | `lint_commits.py` → exit 1 « not the maintainer's identity » |
| 06-8 | release | Il n'existe aucune voie de correctif 1.0.x (`release.yml` n'accepte que `main`, aucune politique `maint/*`). | 🟠 non bloquant (`STABILITY.md` ne promet pas de publications de correctifs) | M | `git rev-list --count v0.50.0..develop` → 59 |

**Déclassés par la vérification (réels, mais sortis du blocage) :**

| id | sévérité expert → retenue | raison |
|---|---|---|
| 08-3 | 🟠 → 🟡 | `PERF.md:71-75` écrit sa convention (« étiquette seule »), et STABILITY classe les chiffres hors contrat. La table est périmée et trompeuse, sans plus. Le facteur ≈ 3 (4 222 contre 1 351 mclk) reste à corriger. |
| 12-1 (et 05-11, 10-13) | 🟠 → 🟡 | L'écart « outils 1.0.0 / SDK 0.50.0 » disparaît au tag 1.0. Le défaut durable est la valeur en dur, que le sentinel ne voit pas. |
| 05-4 | 🟠 → 🟡 | Livrer `stdint.h` et les autres en-têtes est un ajout, possible après le gel, et les largeurs sont déjà fixées par l'ABI (int 2, long 4). |
| 11-5 | 🟠 → 🟡 | Le skill `port-example` (gfx4snes) est un outil de session contributeur, invisible pour l'utilisateur. |
| 09-5 | 🟠 → 🟡 | La règle de processus existe ; il manque son application par l'outil. |
| 10-1, 06-1 | 🔴 → 🟠 | Voir le tableau. |

**🟠 des rapports non soumis à vérification** (ils n'étaient pas dans le résumé machine ; à revérifier avant d'agir) : 04-8 (fork de 185 patchs, 10 commits QBE en cinq jours avant le gel, `e01207f` en conflit certain avec `79ff312`), 05-15 (numéros de ligne de cproc décalés, refus QBE sans fichier ni ligne), 06-4 (pilotage éclaté entre quatre documents), 06-5 (le rappel `SessionStart` de `settings.json` dit « volatile crashes QBE »), 06-6 (environ 28 k jetons de règles par session), 06-7 (`STRUCTURAL_DEFECTS.md` A5 : quatre comptes qui se contredisent), 06-10 (rythme et taille des releases), 11-3 (deux jeux sur huit bouclent, aucun ne sauvegarde).

## 3. Critères go / no-go de la 1.0

Chaque critère se vérifie par une commande ou un fichier. « Aujourd'hui » vaut au 2026-10-17, sur `73a3fdfe`.

| # | Critère | Comment le vérifier | Aujourd'hui |
|---|---|---|---|
| G1 | Le filet de non-régression est vert | `make tests`, `make lint`, `make docs-strict` à 0, et `rom_coverage.py` à 344/344 | **GO** (09, 10) |
| G2 | Une session console est consignée, rangées 1-7, 16-18 et 24-26 comprises, avec le modèle et la sérigraphie de la carte mère | `docs/HARDWARE_VERIFICATION.md`, « Results log » non vide | **NO-GO** : « No session recorded yet » |
| G3 | Critère 7 : 14 jours sans défaut silencieux, et chaque semaine de la fenêtre a sa ligne « Weekly effort » | `silent_defects_log.md` : dernière ligne de défaut ≤ tag − 14 j | **NO-GO** : `fb429433` et 04-1 absents, semaine 2 absente → ≥ 2026-10-31 |
| G4 | Aucun asset tiers non attribué ; aucun personnage Nintendo dans `git ls-files examples` | Script md5 contre PVSnesLib (11-1) = 0 fichier non listé dans `ATTRIBUTION.md` | **NO-GO** : 56 fichiers, 0 attribué |
| G5 | Un jeu vitrine original et publiable existe | Dépôt public, commit de code de moins de 30 jours, ROM jointe à la release | **NO-GO** : RPG sans code depuis le 08-18 et sans remote |
| G6 | La frontière du C accepté est fermée | `cases/negative/vla.c` refusé ; `__STDC_NO_VLA__` défini ; cellule `d_quals` `r = kl` juste ; `make test-compiler` vert | **NO-GO** (04-1, 04-2) |
| G7 | Une ROM qui échoue au contrôle post-lien n'est jamais livrée | `rm game.sfc; make; make` échoue deux fois | **NO-GO** (04-3) |
| G8 | Le contrat gelé dit vrai et dit tout : `ABI.md` conforme au code ; STABILITY liste les refus, le statut des puces, la liste blanche des `extern`, les outils (CLI, TOML, `--json`, codes de sortie) et le sort de SNESMOD | Lecture, plus une ancre sentinel « refus de `cases/negative/` ⊂ `ABI.md` » | **NO-GO** (04-4, 02-3, 05-7, 12-5, 03-3) |
| G9 | Aucun conflit de canal DMA/HDMA interne | Fixture « letterbox + texte + sprites dynamiques » qui asserte une image non noire dans `make tests` ; table des canaux réservés dans `hdma.h` | **NO-GO** (01-1) |
| G10 | Le zip est conforme aux licences | `unzip -l` liste `LICENSE-qbe`, `LICENSE-cproc` et la notice LGPL (ou `strings bin/* \| grep -c glibc` = 0) ; `docs/LICENSING.md` existe | **NO-GO** (13-4, 13-5, 10-3) |
| G11 | Les outils ne lisent pas hors mémoire et n'écrivent pas hors de leur dossier | Golden « calque `../x` refusé » ; `bmp` dans `FUZZERS` ; `grep -n 'system(\|popen(' tools/opensnes/src/main.c` = 0 | **NO-GO** (07-1, 07-2, 07-3) |
| G12 | Les outils audio sont corrects | Golden : en-tête du module à l'offset `$1A00` du `.spc` ; module de plus de 57 Ko → exit 1, rien d'écrit | **NO-GO** (03-1, 03-2) |
| G13 | Les ROM Windows et macOS sont celles de Linux | Job CI qui compare les sha256 des 85 ROM par jambe, ou `--compare` et `test-manifests` sur chaque jambe | **NO-GO** (09-2) |
| G14 | Le premier contact marche tel qu'il est écrit | `release_smoke.py` extrait l'archive d'exemples et trouve la ROM d'A4 ; l'extrait « Manual setup » construit ; un chemin avec espace est refusé par `doctor` | **NO-GO** (10-1, 05-1, 12-4) |
| G15 | La performance publiée dit ce que mesure le dépôt | `BENCHMARK.md` contient le tableau twinbench (22 paires) ; `nmi_budget.py` replie `_vqu_*`/`_endframe_*` et a une ligne cumulée avec plafond | **NO-GO** (08-1, 08-2) |

**1 GO sur 15.** G2, G4 et G5 dépendent du propriétaire. G3 dépend du calendrier : ce critère ne peut être vert qu'après tous les autres correctifs de code.

## 4. Feuille de route

Les lots sont ordonnés par dépendance. « Mainteneur » veut dire une session de ce dépôt ; « propriétaire », une décision ou un acte qu'il est seul à pouvoir faire. Tout changement de compilateur, d'en-tête public ou de `make/common.mk` se fait dans un **worktree séparé** et s'annonce au jeu qui consomme cet arbre (`exchanges.md`).

| Lot | Contenu | Dépend de | Effort | Qui |
|---|---|---|---|---|
| **0 — Décisions** | (a) Les assets PVSnesLib et Nintendo bloquent-ils la 1.0 ? La ROADMAP dit oui, la note du 10-05 dit non (11-1/13-1). (b) Réécrire et dater la porte vitrine (06-3). (c) Dater la session console (09-1). (d) SNESMOD : gelé « binaire sans source », ou hors promesse (03-3). (e) SA-1 et Super FX : « provisional » ou gelés (02-3). (f) Contributions externes, critère 2 (06-1). (g) Les ruptures d'API voulues avant gel : `oamInit`, renommages (05-5, 05-6), fonctions à plus de 5 arguments (05-8). (h) Retirer ce qui décrit le projet tiers (06-9, fait le 2026-10-17). (i) Voie 0.50.1 (06-8). | — | S chacune | Propriétaire, sur recommandation écrite du mainteneur |
| **1 — Défauts silencieux** | Refus du VLA et `__STDC_NO_VLA__` (04-1) ; drapeaux d'accès dans `funccopy` (04-2) ; `.DELETE_ON_ERROR` (04-3) ; DMA du NMI sur le canal 7, avec sa fixture (01-1) ; HDMA INIDISP en mode 1 vers `$21FF` avec `$8F` (01-2) ; tutoriel SA-1 et refus de `SA1Start` hors banque $00 (02-1) ; `opensnes-music` à `$1A00` et refus de l'ARAM (03-1, 03-2). Puis consigner 04-1 et `fb429433` et recalculer la fenêtre (06-2). | — | ≈ 4 j, Classe A pour 04-1 et 04-2 | Mainteneur |
| **2 — Contrat à geler** | `ABI.md` (04-4) ; liste des refus dans STABILITY (04-10) ; liste blanche des `extern` (05-7) ; section outils (12-5) ; registres « SA-1 side » de `sa1.h` (02-2) ; ruptures décidées au lot 0 (05-5, 05-6), avec `removed_api.txt` et `changed_api.txt` ; alias audio à déprécier et retour de `snesmodPlayEffect` (03-5, 03-6) ; historique daté purgé des en-têtes (05-12, 10-9) ; dépréciation sans clang (10-4) | 0 (g, d, e) | M | Mainteneur ; le jeu prévenu avant et après |
| **3 — Outils et build** | 07-1, 07-3 (S), 07-2 (S-M) ; 12-2, 12-3, 12-4, 12-6 ; 05-1, 05-2, 05-3 ; versions dérivées de `snes.h` (12-1) ; ne plus suivre les sorties de conversion (12-15) ; test incrémental (12-14) | — | ≈ 4 j | Mainteneur |
| **4 — Licences et distribution** | `LICENSE-qbe`, `LICENSE-cproc`, notice LGPL ou musl (13-4, 13-5) ; `docs/LICENSING.md` (10-3/13-6) ; `LICENSE-pvsneslib` et en-têtes zlib (13-7) ; archive source wla-dx (13-9) ; renommages tetris, mario et 1942 (13-3) ; retrait ou remplacement des `.it` tiers (13-2) ; liste « Third-party assets » et correction de `ROADMAP.md:53` (S), puis remplacement des assets Nintendo (L) ; permission de krom (13-8) | 0 (a) | S à M, puis L pour les assets | Mainteneur ; propriétaire pour les assets et krom |
| **5 — Mesure et tests** | CI Windows/macOS par sha256 ou `--compare` (09-2) ; repli `_vqu_*` et ligne cumulée (08-2, 01-4) ; twinbench publié et réparé (08-1, 08-8) ; `PERF.md` régénéré par script et ancré (08-3) ; `--reason-file` pour les re-captures audio et de masse (09-4, 09-5, 09-3) ; `vram_dma_blank.py` sous entrée et étendu à CGRAM/OAM (01-12, 09-7) ; asserts de spécification sur les positions (09-3) | 1 (fixture canal) | M | Mainteneur ; luna pour le `--budget` inclusif et `diff --audio` contre un profil |
| **6 — Documentation du premier contact** | 10-1 et un `release_smoke.py` qui ouvre l'archive d'exemples ; 10-2 ; 11-4 ; comptes « 88 » et « 86 » et motifs du sentinel (10-5, 11-10) ; commandes luna des tutoriels (10-6) ; une seule table « 1.0 readiness » liée depuis ROADMAP (06-4) ; rappel `settings.json` (06-5) | 2 (pour les pages qui citent le contrat) | ≈ 2 j | Mainteneur |
| **7 — Matériel** | Protocole corrigé (01-3 : ligne 0 identité, sérigraphie, port 2 vide, firmware DSP-1 noté) puis session FXPak : rangées 1-7, 16-18, 24-26 | 1 (01-2 corrigé avant, sinon un KO sur 18 et 24 reste illisible) ; 5 (preflight à image) | L | Propriétaire |
| **8 — Fenêtre du critère 7** | Gel de la vélocité du backend : correctifs seulement, plus de nouvelle règle de peephole (04-9 rec 9) ; une ligne « Weekly effort » par semaine ; les configurations « interrupteur coupé » dans la chasse nocturne | Fin du lot 1 (tout correctif silencieux relance le compte) | 14 j calendaires minimum | Mainteneur |
| **9 — Release candidate** | `release: v1.0.0-rc` sur `main`, le zip passé au `release_smoke`, l'archive d'exemples conforme à G4 | Tous les lots, plus le go du propriétaire | S | Propriétaire (tag) |
| Après la 1.0 | Rattrapage du moteur d'objets et de `oamMetaDrawDyn` (08-1) ; NON et PMON, et coexistence `audio`/`snesmod` (03-8) ; moteur de carte multi-couche (05-14) ; bibliothèque `.sfx` (02-6) ; absorption de `e01207f` (04-8) | — | M à L | Mainteneur |

## 5. Progrès depuis l'audit du 03/10

Le compte porte sur les lignes des sections « Suivi de l'audit du 03/10 » des 13 rapports. Un constat du 03/10 qui touche deux domaines y est compté une fois par rapport.

| Rapport | Lignes | Corrigés | Partiels | Ouverts | Non vérifiable |
|---|---|---|---|---|---|
| 01 matériel | 5 | 2 | 2 | 1 | — |
| 02 puces | 7 | 6 | 1 | 0 | — |
| 03 audio | 15 | 12 | 1 | 2 | — |
| 04 compilateur | 10 | 3 | 3 | 4 | — |
| 05 SDK (pas de lettre au 03/10) | 1 | 0 | 0 | 1 | — |
| 06 organisation | 15 | 5 | 3 | 6 | 1 |
| 07 sécurité | 1 | 1 | 0 | 0 | — |
| 08 benchmark | 4 | 2 | 2 | 0 | — |
| 09 tests | 16 | 8 | 3 | 5 | — |
| 10 documentation | 9 | 4 | 4 | 1 | — |
| 11 exemples | 12 | 4 | 5 | 3 | — |
| 12 build | 12 | 9 | 3 | 0 | — |
| 13 licences | 3 | 0 | 1 | 2 | — |
| **Total** | **110** | **56 (51 %)** | **28 (25 %)** | **25 (23 %)** | **1** |

**Lecture.**

- **Les fermetures portent surtout sur le code et l'outillage.** Toutes les lignes C du build sont fermées ou partielles (aucune ouverte). Six des sept lignes des puces sont corrigées, et 12 lignes audio sur 15.
- **Les ouverts restants sont surtout des décisions et des actes du propriétaire, ou des dettes structurelles.** Assets (F PF1, compté par 11 et 13, **aggravé**), session console (D PF6 et E PF10), blob SNESMOD (H PF17, compté par 03, 06 et 13, **aggravé** : un second blob différent est apparu dans l'outil), facteur bus (H PF19, **durci** par le lint d'identité), portes non datées (H PF14, **aggravé**). Côté compilateur : argument `u8` non étendu (A n°7), lint hôte en `int` 32 bits (A n°8), `e01207f` non absorbé (A PF7). Côté tests : références dorées (D T1) et coût de l'oracle WRAM (D PF7).
- **Des constats ont récidivé sous une autre forme.** La classe « qualificatif perdu » (A PF3) revient avec 04-2. `ABI.md` (A PF4) compte trois affirmations fausses de plus. Les chantiers fermés laissés dans `chantiers/` (H PF6) reviennent avec 06-12.
- **Doublons entre rapports** : B PF2 (01, 08), D T8 (01, 08, 09), H PF16 (04, 06), H PF17 (03, 06, 13), F PF1 (11, 13), D PF12 (07, 09). Une fois dédoublonnés, les proportions restent les mêmes, à quelques points près.
- **Deux discordances entre experts :**
  - **B PF5** (`hdmaIrisWipe`) : « corrigé (doc) » pour 08, « partiel » pour 01. 01 a raison : la doc dit « FIRST call », alors que `hdma.c:513` attend à chaque appel (01-8).
  - **F n°4** (ordre KON / registres de `snesmod_sfx`) : « tranché, sans effet audible » pour 03, « hypothèse » pour 11. 03 a la meilleure pièce : une trace `--dsp-trace` mesurée, plus anomie-sdsp `f16a9436d9b55cb6` (« 5 empty samples before envelope updates »). 11 n'a interrogé que snesdev-wiki `6768cd86607515b5`. Le point est **tranché** ; 11-18 est à retirer.

## 6. Constats transverses

1. **La promesse de stabilité est écrite plus large que ce qui est prêt à être gelé.** Elle gèle des modules « Experimental » (02-3), des `extern` internes (05-7), un `ABI.md` faux (04-4), des macros inopérantes (02-2), un piège reconnu (05-5), des noms incohérents (05-6), des alias qui ne font rien (03-5) et un pilote sans source (03-3). Elle interdit aussi de refuser plus tard un VLA qu'on accepte aujourd'hui (04-1). Et elle **oublie** ce que l'utilisateur écrit à la main : les fichiers TOML, la CLI, le `--json` (12-5). Cinq experts, cinq angles, un même constat : STABILITY doit d'abord être resserrée (une liste blanche, des exemptions datées), puis tenue.
2. **Tout repose sur un seul émulateur.** Aucune session console (09-1, 02-4). luna ne modélise pas les révisions de CPU, alors que deux errata visent la forme exacte du HDMA INIDISP du SDK (01-2). luna accepte MS0 à 21 MHz (02-4). Aucun second émulateur ne sert de contre-témoin (09-1). Et les ROM Windows et macOS ne sont comparées à rien (09-2, 12-18). Le risque se concentre sur les rangées HDMA + DMA du protocole (5, 6, 18, 24).
3. **Un état sur disque fait mentir le build.** Le second `make` réussit sur une ROM fausse (04-3), le premier build échoue mais le second passe (12-2), `USE_LIB` est absent du tampon de configuration (05-1), wla-dx et les convertisseurs sont hors de `COMPILER_ID` (12-3). C'est la même classe chez quatre experts : le build croit ce que le disque lui montre. Il manque une cible `test-incremental` (12-14), réclamée par deux audits.
4. **Des Makefiles et des harnais écrivent des fichiers suivis par git.** `make clean` efface 36 fichiers suivis (12-15), et la passe `--coverage` réécrit `CORPUS_COVERAGE.md` (09-10, déjà vu par 01). L'incident de cet audit (§ 8) l'a rendu visible. Il a aussi trompé un expert : 13-1 attribue à « un travail commencé hors de HEAD » la suppression des `tiledMario.{b16,o16,t16}`, alors qu'elle venait du `make clean` de l'incident.
5. **Les documents qui pilotent la 1.0 disent faux.** Le critère 7 (06-2), « listed in ATTRIBUTION.md » (`ROADMAP.md:53`, 11-1/13-1), la vitrine « In progress » (06-3/11-2), le suivi resté au 10-04 (06-4), « 88 examples » (06-16, 10-5, 11-10), le « 61 % » publié sans le banc des jumeaux (08-1). La décision de geler se prendrait sur ces documents.
6. **Les angles morts du sentinel ont la forme de ses exemptions.** Le guide de migration est exempté de l'ancre 16 (10-2). Les en-têtes, les `.c`/`.asm` d'exemples et les Makefiles ne sont pas lus par l'ancre 18 (05-12, 10-9, 11-11). L'ancre 4 ne vérifie que les prototypes (04-4). Ne sont lus par aucune ancre : `settings.json` (06-5), `PERF.md` (08-3), les commandes luna des tutoriels (10-6), les liens relatifs des README (10-7), gfx4snes dans les skills (11-5), « N examples — » et « N small ROMs » (10-5, 11-10), la version des outils (12-1).
7. **Le coût VBlank n'est vu qu'à moitié.** La porte NMI mesure des étiquettes nues (08-2), il n'y a ni cumul ni plafond sur les hooks (01-4), `PERF.md` est sous-estimé d'un facteur 3 (08-3), et 32 en-têtes sur 39 ne citent aucun coût (05-13, 08-10). Un défaut là donnerait des tuiles manquantes, sans erreur.
8. **Les oracles de hachage sont devenus une taxe.** WRAM et audio sont re-capturés dans un commit sur quatre et n'ont rien trouvé seuls (09-4, 03-12). Les re-captures ne sont pas justifiées par l'outil (09-5), et les références de masse ne sont pas gardées (09-3). Le différentiel du compilateur et les audits trouvent, les hachages non.
9. **Les dettes héritées se gèlent par défaut.** SNESMOD (03-3, 06-11, 13-6) ; le moteur d'objets et le moteur de carte de PVSnesLib, à la fois plus lents (08-1) et figés dans leur forme (05 risque 1, 05-14) ; les notices zlib retirées des dérivés (13-7) ; le code de krom sans licence (13-8).
10. **Un seul mainteneur, une forte vélocité, juste avant le gel.** 614 commits en 30 jours (06-10), 10 commits QBE en cinq jours, dont des règles de peephole textuelles (04-8). Un fork de 185 patchs, sans remontée amont par décision du propriétaire (04-8, 06-7). Le facteur bus est imposé par l'outil (06-1). Et les règles dépendent d'une seule machine (06, risques).
11. **L'arbre partagé avec le jeu est un point de fragilité.** Les builds concurrents d'experts ont fait échouer `make tests` (09-9). Le marqueur de tests est commun aux worktrees (06-15). Un `make clean` à la racine supprime `bin/` d'un coup (12, risques), et le jeu consomme cet arbre en direct (`exchanges.md`).

## 7. Devoirs envers les partenaires

Une ligne par manque, à reporter le jour même dans `OPEN_<partenaire>.md` (`partners.md`). Chaque ligne cite sa pièce.

**snes-rag** → `.claude/notes/partners/snes-rag/OPEN_snes-rag.md` (puis copie dans son dossier d'échange) :

- `snes_verify` rend `arbiter_covers_topic_only` sur « drapeau H-blank à H = 274 », alors qu'anomie-timing `08a81c8c93552908` l'énonce. La phrase n'est pas dans `evidence` (01, risque 5).
- `snes_verify` rend `unsettled` sur le verrou d'écriture partagé entre BG1 scroll et Mode 7, alors que fullsnes `6b054a12234f8aa4` et snesdev-wiki `78c0e95f65b95a86` l'énoncent (01, risque 5).
- La ligne Super FX de `sd2snes-changelog` n'est toujours pas servie : seul `99d8550526ffa29f` (S-DD1) remonte (02-4 ; G rec 11, rejoué aujourd'hui).
- La règle de choix `dsp1.bin` / `dsp1b.bin` du FXPak reste sans source (02-8). La ligne `OPEN_snes-rag.md:14` existe : ajouter la pièce du jour.
- La ligne SA-1 BW-RAM (`OPEN_snes-rag.md:11`) reste en attente (12, rec 8).
- Fait non couvert : la valeur du 17ᵉ bit d'un port 2 vide n'est donnée par aucune référence (`crt0.asm:1416-1418`, 01 risque 2). À mesurer pendant la session console, puis à transmettre avec sa provenance.
- Fait mesuré, à proposer au corpus : huit KON enchaînés par le protocole du pilote v2 ne perdent aucune voix (ENVX actif sur `0xFF`, `0xFF`, `0x03`, sonde `kon.toml`), malgré l'avertissement de fullsnes `5c23c8e8dbaf1931` (03, points forts).
- Récupération : sur l'ordre KON / VOL / P / ENV, une requête a ramené snesdev-wiki `6768cd86607515b5`, qui ne tranche pas, et pas anomie-sdsp `f16a9436d9b55cb6`, qui tranche (03 contre 11, F n°4).

**luna** → `.claude/notes/partners/luna/OPEN_luna.md`, puis un message direct à la session luna (`exchanges.md`) :

- Le compteur `gsu.ms0_at_21mhz` et le décodage DSP-1 par carte (`$20`/`$30`/`$60`) sont livrés sur `develop` `1e0df1f` mais absents de la v1.38.0 épinglée. Il faut une release (02-4, 02 risques).
- La colonne `hclock` des écritures HDMA vaut 0 dans `--mem-trace`, alors que le HDMA a lieu vers H = 278 (01, risque 5 ; fullsnes `ae801928f9327bcc`).
- Demande : modéliser les révisions S-CPU v1 et v2A et le 1-CHIP, au moins pour les deux errata DMA/HDMA et le fondu INIDISP (01-2, 01-3). Sans cela, aucun test ne peut voir 01-2.
- Demande : un `profile --budget` inclusif par plage d'adresses, qui somme l'appelé et ses étiquettes internes sur une même image (01-4, 08-2).
- Demande : `diff --audio` contre un profil de référence (RMS par fenêtre, en JSON) au lieu d'une ROM, pour remplacer le hachage WAV (09-4, rec 3).
- Remarque : `--power-on` randomise les RAM mais pas les registres PPU (01, risque 3). C'est un risque faible, à signaler sans demande.

**Autres** (hors `partners.md`, pour mémoire) : `e01207f` à absorber au prochain relevé mensuel QBE (04-8) ; permission écrite à demander à krom (13-8) ; la licence actuelle de `mukunda-/snesmod` n'a pas été revérifiée faute de réseau (13).

## 8. Incident pendant l'audit

**Ce qui s'est passé.** Pendant l'audit 12, un `cd` vers un dossier de travail inexistant a échoué. La commande suivante est partie dans le dossier par défaut de la session, qui est l'arbre principal `/home/kobenairb/workspace/opensnes`. Elle y a lancé `make OPENSNES=<worktree> clean`, `make GSU_BANK=99`, de nouveau `make clean`, puis `make GSU_BANK=x`, entre 11:55 et 11:58 environ. Le récit détaillé est en tête de `12_build_outils.md`. `OPENSNES=` n'agissait que sur `common.mk` : à la racine, c'est le `Makefile` de l'arbre principal qui a tourné.

**Les effets.**

- `bin/` a été incomplet pendant environ trois minutes, alors que le jeu qui se construit sur cet arbre le consomme.
- Les 85 ROM d'exemples ont disparu.
- 10 fichiers suivis par git ont été supprimés : cinq `.spc700.bin`, `sfx_from_wav/res/{blip,coin}.h` et `map_scroll/res/tiledMario.{b16,o16,t16}`.

**La restauration.** Les fichiers suivis ont été remis, les exemples reconstruits, et likemario et tetris concordent sur luna. Le jeu a été prévenu. Relu ici, en lecture seule, au moment de cette synthèse : `git status --untracked-files=no` est vide, 85 `.sfc` sont présents et `bin/` compte 19 binaires.

**Ce que l'incident révèle.**

- **Un constat sur le build.** Un `make clean` efface 36 fichiers suivis par git (12-15), et `make tests` réécrit `CORPUS_COVERAGE.md` (09-10). Un dépôt où le build supprime ou réécrit des fichiers versionnés a deux sources de vérité, et un `git status` sale après un simple `clean`. C'est aussi ce qui a induit l'expert 13 en erreur (§ 6, point 4). Les correctifs sont dans le lot 3 : ne plus suivre les sorties de conversion, et n'écrire `CORPUS_COVERAGE.md` que sous `--update`.
- **Un constat d'organisation.** Un `make clean` à la racine supprime `bin/` immédiatement (12, risques). L'arbre que le jeu consomme en direct est aussi celui où atterrit toute commande mal dirigée. Le garde-fou mis en place pendant l'audit (`cd <chemin absolu> || exit 1`, jamais de `make` dans l'arbre principal) devrait devenir une règle des agents d'audit (`.claude/agents/audit-*.md`). Une protection côté `Makefile` vaut aussi d'être étudiée, par exemple un `clean` racine qui demande une variable explicite quand `bin/` est en usage.
