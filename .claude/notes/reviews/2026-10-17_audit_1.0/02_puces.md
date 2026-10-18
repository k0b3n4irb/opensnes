Expert 02, puces d'extension (SA-1, Super FX, DSP-1). Audit du 2026-10-17. HEAD audité : `develop` @ `73a3fdfe` (worktree `/home/kobenairb/workspace/opensnes-audit`), luna `1.38.0`.

# 02 — Puces d'extension : état réel avant 1.0

## Périmètre couvert

- Lu : `lib/include/snes/{sa1,superfx,dsp1,sram}.h`, `lib/source/{superfx.asm,sram.asm,dsp1.asm}` (parties puces), `templates/{sa1_boot.asm,hdr_superfx.asm}`, blocs SA-1 et Super FX de `templates/crt0.asm` (l. 136-170, 605-625, 820-895), `make/common.mk` (l. 114-263, 642-712), `docs/tutorials/{sa1,superfx,dsp1}.md`, `docs/HARDWARE_VERIFICATION.md`, `docs/STABILITY.md`, `ROADMAP.md`, `README.md` (§ maturité), `KNOWN_LIMITATIONS.md`, `examples/chips/README.md`, l'audit G du 03/10, `2026-10-04_reste_a_traiter.md`, la revue `2026-09-24_superfx_game_gaps.md`, `chantiers/superfx_runtime.md`, `OPEN_luna.md`, `OPEN_snes-rag.md`, `silent_defects_log.md`, la note luna du 2026-10-12.
- Lancé dans le worktree : `luna_runner.py --compare --only chips` (8/8) et `--only dsp1` (2/2) ; `--compare --only chips --power-on random=3` (8/8) ; `--coverage --region pal` (83 OK / 2 INPUT-DEP / 0 DEAD, dont les 9 ROM puces) ; `luna test` sur les 20 manifestes puces (20/20) ; `test_libtest_dsp1.py` (16/16, firmware présent) ; `sa1_save` / `superfx_save` sous `--power-on zero|ones|random=1|random=7`.
- Variantes construites dans le scratchpad : `sa1_hello` avec le `sa1_boot.asm` du tutoriel + 31 000 octets ; une ROM SA-1 qui utilise le multiplicateur SA-1 depuis le C ; `libtests_dsp1` avec `USE_SRAM=1` ; `superfx_3d` et `superfx_game_skeleton` hors de l'arbre.
- Corpus (avec `exclude_sources=["opensnes-docs","opensnes-notes-tech"]`) : 4 `snes_search` (sd2snes et Super FX ; CRV du SA-1 ; Star Fox, `$0108` et WRAM ; côté d'accès de l'arithmétique SA-1).
- Pas couvert : le DMA SA-1 et la conversion de caractères (le SDK n'en a aucun code), le comportement interne de `wla-superfx` (→ expert compilateur), la CI distante.

## Points forts

1. **Les tests des trois puces passent, y compris dans les configurations dures.** 20/20 manifestes puces (`coproc_*`, chaînes de sauvegarde `d_`…`k_`, `libtest_gsu_cached{,_pal}`, `superfx_game_skeleton`), 10/10 comparaisons visuelles, 8/8 sur RAM aléatoire (graine 3), les 9 ROM puces vivantes en PAL. Le fichier `testing/baselines/never_executed.txt` ne contient aucune fonction `gsu*`, `sa1*` ou `dsp1*` : toute l'API publique des trois puces est exécutée au moins une fois.
2. **Le runtime Super FX a la forme d'un vrai jeu et suit l'arbitre.** Les vecteurs sont en WRAM à `$0100-$010F` (`crt0.asm:146-163`). Les deux entrées viennent de fullsnes `9ef0ca93628968b7` (« [FFEAh]=0108h NMI Vector … ») et confirment l'usage de Star Fox (fiches `17f8aa1845820a16` et `4544ecaf0f5bdd55`, `SF/ASM/NMI.ASM:21-72`). luna mesure 452 lectures de vecteur en `$010C` pendant les jobs de Star Fox (`595d313b97f6f7d1`). Il y a le cache, la présentation en double tampon (`gsuPresent`), `gsuCall` avec ses points d'entrée générés (`<name>.sfx.h`), `GSU_BANK` et la sauvegarde. Les manifestes assertent `bus_violations = 0`, chacun avec son contrôle négatif écrit (`libtest_gsu_cached.toml`, en-tête).
3. **Les six défauts silencieux de l'audit du 03/10 sont corrigés et inscrits au journal.** On les retrouve dans `silent_defects_log.md` :
   - l. 56, MS0 à 21 MHz ;
   - l. 59, `gsuSetupHdmaBlanking(0, n)` ;
   - l. 73, `.sa1_boot` en `SUPERFREE` ;
   - l. 77, `gsuPresentInit` au-delà de 64 Ko ;
   - l. 60, borne de `ROM_BANKS` ;
   - l. 63, somme de contrôle SA-1.

   Le code le confirme : `superfx.asm:373-390` découpe ou omet les bandes, `templates/sa1_boot.asm:15` place le programme en `BANK 0 … SEMIFREE`, `common.mk:240` pose `ROM_BANKS_MAX` (32 avec DSP-1).
4. **Les sauvegardes des deux puces sont livrées, exemplifiées et testées.** Elles passent par la BW-RAM pour le SA-1 et par la RAM Game Pak pour le Super FX.
   - Les exemples `chips/sa1_save` et `chips/superfx_save` existent, ainsi que les chaînes `h_`→`i_` et `j_`→`k_` à travers un `.srm`.
   - L'en-tête suit le choix de l'utilisateur : `$34`/`$35` pour le SA-1, `$13`/`$15` pour le Super FX (`common.mk:177-178`), et la taille vient de `SA1_BWRAM_SIZE`, documenté dans `docs/tools/build.md:63-64`.
   - Démarrées sur `zero`, `ones`, `random=1` et `random=7`, les deux ROM lisent `had_save = 0` et `now = 1`. Le mot magique résiste donc à une RAM de cartouche non initialisée, que luna 1.38 randomise désormais (`luna state --help` : « Fills … the cartridge RAM no battery keeps »).
5. **DSP-1 : l'API est typée, et ses faits sont mesurés sur le vrai firmware.** luna émule le µPD77C25 au niveau bas, sur le dump `dsp1b.rom`. Ce que dit `test_libtest_dsp1.py` (Distance « one low » `r_dist = 12`, Range `>>15` `r_range_out = 732`) vient donc du code de Sony, pas d'une réimplémentation. Le bug DSP1/1A de Distance est expliqué dans `dsp1.h:258-267`. Un DSP-1 avec `USE_SRAM=1` (carte SHVC-1B5B, fenêtre `$20-$3F`) trouve sa puce en `$30:8000` : construit dans le scratchpad, il rend `r_done = BEEF`, `r_dist = 12`, `r_range_out = 732`.
6. **Les chemins de build hors de l'arbre fonctionnent.** `superfx_3d` copié dans le scratchpad et construit avec `OPENSNES=<sdk>` produit une ROM identique octet pour octet (`cmp` : IDENTICAL). `bin/` livre `wla-superfx` et `sa1_patch` (`Makefile:638`).
7. **Le protocole matériel couvre les six chemins des puces.** Les rangées 16-18 et 24-26 de `HARDWARE_VERIFICATION.md` couvrent le cube DSP-1, le starfield SA-1, le Super FX 3D, la présentation, et les deux sauvegardes. Le fait « le FXPak Pro fait tourner SA-1 et Super FX » est sourcé (blog sd2snes, `c94f64959972104e` ; v1.11.x « long-standing issues with SA-1 and SuperFX performance », `e4121c6943fd9f1b`).

## Points faibles

**02-1 🟠 Le tutoriel SA-1 enseigne exactement le défaut corrigé le 03/10 : un `sa1_boot.asm` en `SUPERFREE` qui ne démarre pas, sans aucune erreur.**
- Preuve : `docs/tutorials/sa1.md:149` (« `.SECTION ".sa1_boot" SUPERFREE` »), alors que le gabarit et les exemples sont passés en `BANK 0 … SEMIFREE` (`templates/sa1_boot.asm:15`).
- Reproduction : le bloc du tutoriel copié tel quel dans une copie de `sa1_hello`, avec `.dsb 31000` pour simuler un vrai programme. Le build est **vert** (« OK: bank $00 ROM (code): 27190 bytes free »), le `.sym` place `01:8000 SA1Start`, et luna lit `sa1_status = $00` contre `$A5`, `$00:3000 = 00`, PC du SA-1 à 0, fbhash `4c699f3fb20c4f85` contre `7aee1820d6cdfed5`.
- Arbitre : la CRV est sur 16 bits (manuel Book II 4.1.4, `fd09e9ca4f6df406`, « Size: 16 bits » ; sfc-dev-wiki `e5a179772c621ff7`, « in bank $00 »).
- Aucun garde de link ne vérifie la banque de `SA1Start` : la correction tient à une directive que l'utilisateur doit recopier. Or, depuis le 10-10, déborder de la banque $00 est le comportement *normal* d'un gros jeu (`bank0_budget.md`). Un jeu SA-1 qui grossit perd donc son coprocesseur en silence dès que le linker déplace la section.
- Le même extrait n'active pas non plus CBWE, que le gabarit écrit.
- Effort S : corriger le tutoriel, et faire refuser par `opensnes-rom check` un `SA1Start` hors banque $00.

**02-2 🟠 `sa1.h` expose comme macros C des registres que seul le SA-1 peut lire ou écrire. Ils ne font rien depuis le C, et `STABILITY.md` les gèle.**
- Preuve : `sa1.h:94-109` (« Hardware arithmetic » : `REG_SA1_MCNT`, `MAL…MBH`, `REG_SA1_MR`, `REG_SA1_OF`) et `:76-83` (`CFR`, `HCR`, `VCR`, `DCNT`, `SCNT`, `CIE`, `CIC`, `CIWP`). Aucun commentaire ne dit « SA-1 side only » ; la seule indication est la ligne d'en-tête de bloc l. 58 pour `SCNT`/`CIE`/`CIC`.
- Arbitre : fullsnes `89dcb30a6cfdbd19` (« 2306h **SA-1** MR - Arithmetic Result ») et `4a4b420d55965574` (colonne « Side ») ; sneslab `0fb5456c714fa637` (MR, OF, CFR, HCR, VCR : « Access SA-1 »).
- Mesuré : une ROM SA-1 dont le `main()` écrit `MCNT=0`, `MA=$1234`, `MB=$0010`, puis lit `$2306`, obtient `00 00 00 00` au lieu de `$00012340`, sans avertissement.
- Le C ne tourne que côté S-CPU (`sa1.h:10-13`) : la moitié du bloc de macros est un piège, et `docs/STABILITY.md:17-20` promet de le garder jusqu'à 2.0.
- Effort S : déplacer ces macros dans un `.inc` pour l'asm SA-1, ou les annoter « SA-1 side: unusable from C » avant le gel.

**02-3 🟠 La promesse 1.0 gèle deux modules que le README déclare encore « Experimental », et l'API Super FX gelée inclut une configuration par variables globales.**
- Preuve : `README.md:93-94` (« **SA-1** | Experimental », « **SuperFX** | Experimental » ; le DSP-1 est absent du tableau) contre `docs/STABILITY.md:17-24` (« Every public function, type, macro and constant … Every documented variable a header declares `extern` … `gsu_cfgr` »), sans aucune exception de module.
- `superfx.h` déclare 27 fonctions, inlines et variables `extern` (`grep -c '^extern\|^void\|^u8\|^u16\|^inline'`). La configuration passe par `gsu_cfgr`, `gsu_scmr`, `gsu_scbr`, `gsu_dma_src_hi`, `gsu_prog_bank`, `gsu_prog_addr` et `gsu_scmr_live`, que l'utilisateur doit garder cohérent à la main (`superfx.h:152-160`). Geler cette forme contredit le principe « type-safe boundaries » de `PHILOSOPHY.md`.
- Conséquence : soit la 1.0 ment (« Experimental » mais gelé), soit le SDK s'interdit de remplacer les globales par une structure de configuration avant 2.0.
- Effort S pour décider : exempter explicitement `sa1.h`/`superfx.h` dans `STABILITY.md` (statut « provisional »), ou retirer « Experimental » et assumer le gel. Effort M pour refondre la configuration avant le gel.

**02-4 🟠 Aucune ligne de code des trois puces n'a jamais tourné sur une console, et la porte 1.0 ne l'exige pas.**
- Preuve : `HARDWARE_VERIFICATION.md`, « Results log » : « No session recorded yet ». `ROADMAP.md:17-18` ne fait porter la porte que sur les rangées 1 à 7 ; les puces sont en 16-18 et 24-26.
- Tout ce que ce rapport crédite est donc « vrai sur luna ». Or luna valide au moins un cas que le matériel interdit : MS0 + CLS. Le compteur `gsu.ms0_at_21mhz` n'existe que sur le `develop` de luna (`1e0df1f`, 2026-10-12), absent de la v1.38.0 épinglée : `luna state … | jq '.gsu | keys'` ne le liste pas.
- Le FXPak exécute des réimplémentations FPGA (sources : le blog sd2snes, solid ; l'arbitre `sd2snes-changelog` ne sert toujours que `99d8550526ffa29f`, S-DD1). Une rangée verte sur FXPak n'est donc pas encore une preuve contre une vraie cartouche.
- Conséquence : la 1.0 gèlerait des API de puces sans le moindre essai matériel.
- Effort S pour inclure les rangées 16-18 et 24-26 dans la première session, ou pour écrire « chip rows not yet run on hardware » dans `KNOWN_LIMITATIONS.md`.

**02-5 🟡 SA-1 : un jeu n'a aujourd'hui que le démarrage, l'I-RAM et la BW-RAM.**
- Livré : `sa1IsReady()` (la seule fonction de `sa1.h`), le stub de démarrage, la sauvegarde BW-RAM, des macros de registres.
- Absent :
  - pas de C côté SA-1 (`sa1.h:10-13`) : toute logique déportée s'écrit en asm ;
  - aucun vecteur NMI/IRQ SA-1 (CNV, CIV) ni messagerie CCNT/SCNT encapsulée ;
  - ni DMA SA-1 ni conversion de caractères, les fonctions qui servaient aux jeux commerciaux ;
  - pas de Super MMC au-delà de 2 Mo (`ROM_BANKS_MAX` = 64).
- Aucune fixture ne teste une boucle SA-1 au-delà de `sa1_starfield`. C'est honnête dans `README.md` (« C wrapper is minimal »), mais absent de `KNOWN_LIMITATIONS.md`, qui n'a qu'une entrée SA-1 : la polarité SIWP, 🟢.
- Effort S pour une entrée KL « ce que le SDK fait et ne fait pas sur SA-1 » ; L pour une API de messagerie et de DMA.

**02-6 🟡 Super FX : aucune bibliothèque GSU.**
- La revue du 24/09 l'appelait G4. Les 593 lignes de `.sfx` du dépôt sont des exemples (`find . -name '*.sfx' | xargs wc -l`) : pas de remplissage de triangle, de clear, de blit ni de macros des règles de l'ISA.
- Un jeu écrit tout son rendu GSU à partir de zéro, en recopiant `superfx_3d` (fil de fer, Bresenham).
- Effort L. C'est hors porte 1.0, mais il faut le dire dans le tutoriel comme limite.

**02-7 🟡 Le squelette Super FX ne se recopie pas tel quel.**
- Preuve : `examples/chips/superfx_game_skeleton/Makefile:16` (`SOUNDBANK_SRC := ../../audio/snesmod_music/music/pollen8.it`). Copié hors de l'arbre, il échoue : « No rule to make target '../../audio/snesmod_music/music/pollen8.it' ».
- C'est pourtant l'exemple que la page des puces désigne comme point de départ d'un jeu (`examples/chips/README.md:19`).
- → expert exemples pour la règle générale. Effort S : copier le `.it` dans l'exemple.

**02-8 🟡 Les deux faits mesurés du DSP-1 ne seront jamais vus par une console, ni par la CI.**
- Distance et Range ne sont exercés que par `testing/fixtures/libtests_dsp1`. `dsp1_cube` appelle `Attitude`, `Objective`, `Project` et `SetCamera` ; `dsp1_ground` appelle `Raster` et `Triangle`. Le kit matériel ne contient pas la fixture, et `dsp1_ground` n'a pas de rangée.
- En CI (pas de `dsp1b.rom`), les 13 fonctions de `testing/baselines/executed_only_with_firmware.txt` ne s'exécutent pas. Le module DSP-1 n'est donc validé que sur la machine du propriétaire. L'exemption est voulue et documentée, mais c'est la seule partie du SDK dans ce cas.
- La règle de choix entre `dsp1.bin` et `dsp1b.bin` du FXPak reste inconnue (`OPEN_snes-rag.md:14`). Un écart de Distance en session console ne pourrait être attribué que si le firmware chargé est noté, et la rangée 16 ne le demande pas.
- Effort S : une rangée « fixture DSP-1 » qui affiche ses valeurs, et le firmware noté dans la grille.

**02-9 🟡 Incohérences et références mortes dans la doc des puces.**
- `docs/tutorials/sa1.md:79` dit « ROM access … (separate bus, no conflicts) ». La même page, l. 18-22 et 47-49, mesure que les deux CPU « share the ROM bus » (5,04 MHz, 20,8 % de cycles perdus en conflits).
- `sa1.md:398-403` : le tableau « Example ROMs » omet `sa1_save`.
- `dsp1.h:47` : `@see .claude/notes/tech/dsp1_reference.md`. Le fichier ne fait pas partie du zip (`Makefile:634-668` ne copie pas `.claude`), et un en-tête public pointe donc vers un fichier absent chez l'utilisateur.
- `crt0.asm:623` : « writable per Mesen2/snes9x », justification d'émulateur périmée sous une polarité arbitrée.
- Effort S.

**02-10 🟡 `KNOWN_LIMITATIONS.md` ne porte plus que deux entrées pour les puces (l. 294, 317).**
- Il manque :
  - l'IRQ H/V *perdue* pendant `gsuLaunch` (dite seulement dans `superfx.h:197-200`) ;
  - la RAM de sauvegarde Super FX partagée avec les framebuffers (`sram.h:23-30`) : un `sramClear()` efface l'écran ;
  - la règle « `SA1Start` en banque $00 » pour qui écrit son propre `sa1_boot.asm` ;
  - le firmware DSP-1 non distribué et la dépendance à sa révision (1/1A contre 1B) ;
  - l'absence de toute session matérielle (02-4).
- Effort S.

## Risques

- **Le bump de luna au-delà de 1.38.0 doit apporter deux choses qui ne sont pas encore là.** D'une part le compteur `gsu.ms0_at_21mhz` : il faudra l'asserter à 0 dans les manifestes Super FX, comme le prévoit `OPEN_luna.md:16`, sinon un programme qui écrit lui-même CFGR passera vert. D'autre part le décodage du DSP-1 par carte (`$20`, `$30` ou `$60`). Aujourd'hui `ROM_BANKS_MAX = 32` rend la configuration à `$60` inatteignable. Un relèvement de cette borne sans mapping conditionnel dans `dsp1.asm:33-79` (`$30C000`/`$308000` en dur) casserait une ROM DSP-1 de 2 Mo avec RAM.
- **Les réimplémentations FPGA du FXPak.** Un KO sur une rangée de puce pourra venir de la cartouche (le correctif « SA-1 and SuperFX RAM write cycles » de la v1.10.3 l'illustre). Le protocole le dit (`HARDWARE_VERIFICATION.md:20`), mais une seconde référence (cartouche donneuse) n'est pas disponible et n'est pas planifiée.
- **Le gel de `superfx.h` (02-3) fige aussi `gsu_scmr_live`.** C'est un contrat d'ordre d'écriture, « write it BEFORE giving the GSU the buses » (`superfx.h:152-160`), qu'aucun outil ne vérifie : un utilisateur qui écrit `REG_SCMR` sans le tenir à jour corrompt la présentation, sans diagnostic. luna compte `bus_violations`, pas cette faute-là.
- **Le SA-1 sans test de charge.** `sa1_starfield` est la seule ROM où le SA-1 fait un travail suivi. Aucun manifeste n'asserte le protocole d'échange de l'I-RAM sous contention (les deux CPU qui écrivent la même page). Un premier jeu SA-1 sera le vrai test.

## Améliorations recommandées

| # | Action | Sévérité traitée | Effort | Premier pas concret |
|---|---|---|---|---|
| 1 | Corriger le bloc `sa1_boot.asm` du tutoriel (`BANK 0 SLOT "ROM" SEMIFREE`, CBWE) et faire refuser par `opensnes-rom check` un `SA1Start` hors banque $00 | 🟠 02-1 | S | rejouer la variante `.dsb 31000` : le link doit échouer |
| 2 | Séparer les registres « SA-1 side » de `sa1.h` (un `.inc` asm, ou une annotation explicite) avant le gel | 🟠 02-2 | S | la ROM « multiplicateur depuis le C » de ce rapport sert de contrôle négatif |
| 3 | Trancher le statut de SA-1 et de Super FX pour 1.0 : exemption « provisional » dans `STABILITY.md`, ou retrait de « Experimental » du README ; ajouter le DSP-1 au tableau | 🟠 02-3 | S | une ligne dans `STABILITY.md` § « What is not frozen », décidée par le propriétaire |
| 4 | Mettre les rangées 16-18 et 24-26 dans la première session console, ou écrire leur absence dans `KNOWN_LIMITATIONS.md` | 🟠 02-4 | S | `make hardware-preflight ROWS=16-18,24-26` puis la session |
| 5 | Au bump de luna (≥ la version qui porte `1e0df1f`) : `ms0_at_21mhz = 0` dans les manifestes Super FX ; refus dans `luna_runner --coverage` | risque | S | ajouter `[asserts.gsu] ms0_at_21mhz = 0` aux six manifestes qui assertent déjà `bus_violations` |
| 6 | Entrées KL : SA-1 (ce qui est livré, ce qui ne l'est pas), IRQ perdue sous `gsuLaunch`, sauvegarde Super FX dans les framebuffers, firmware et révision du DSP-1 | 🟡 02-5, 02-10 | S | un commit `docs` ; `make lint-docs` |
| 7 | Rendre `superfx_game_skeleton` autonome (`.it` copié dans l'exemple) | 🟡 02-7 | S | `cp` du module, `SOUNDBANK_SRC := music/…` |
| 8 | Rangée console « fixture DSP-1 » à valeurs affichées, avec le firmware chargé noté dans la grille | 🟡 02-8 | S | ajouter `textPrint` des `r_*` à `libtests_dsp1`, puis l'ajouter au kit |
| 9 | Nettoyer `sa1.md:79` et `:398`, `dsp1.h:47`, `crt0.asm:623` | 🟡 02-9 | S | un commit `docs(lib)` |
| 10 | Bibliothèque `.sfx` minimale (clear, triangle plein, blit 8 bpp), avec une fixture qui asserte les pixels | 🟡 02-6 | L | extraire la ligne de Bresenham de `superfx_3d` en `templates/gsu/line.sfx` |

## Verdict

Les trois puces fonctionnent de bout en bout sur luna, avec des tests sérieux : Super FX avec un runtime de jeu (vecteurs WRAM, cache, présentation, sauvegarde), DSP-1 typé et mesuré sur le vrai firmware, SA-1 réduit au démarrage, à l'I-RAM et à la BW-RAM. Les défauts silencieux du 03/10 sont tous corrigés. Ce qui sépare ce domaine d'une 1.0 n'est pas le code mais le contrat : le tutoriel SA-1 enseigne encore le défaut de banque corrigé dans le gabarit (02-1), et `sa1.h` gèlerait des macros inopérantes depuis le C (02-2). La 1.0 figerait aussi deux modules « Experimental » (02-3), jamais lancés sur une console (02-4). Quatre corrections d'environ un jour chacune et une décision du propriétaire suffisent, avec l'adoption du compteur MS0 au prochain bump de luna.

## Suivi de l'audit du 03/10

| Constat (`2026-10-04_reste_a_traiter.md`) | État au 17/10 | Preuve d'aujourd'hui |
|---|---|---|
| C S13 / G PF9 — SA-1 toujours `$35`/`$05`, `SRAM_SIZE` ignoré | **corrigé** | `common.mk:177` (`$34`, ou `$35` avec `USE_SRAM`), `:178` (`SA1_BWRAM_SIZE`) ; `docs/tools/build.md:63-64` dit que `SRAM_SIZE` ne s'applique pas au SA-1 |
| G PF5 — chemins non exercés (`gsuLaunch`+`gsuPresent`, `GSU_PRESENT_ON_LAG_FRAMES`, H160/H192, PAL, sauvegarde en vol) | **corrigé** | 9 occurrences de `SCMR_H160`/`SCMR_H192`/`GSU_PRESENT_ON_LAG_FRAMES` dans `testing/fixtures/libtests_gsu/main.c` ; `libtest_gsu_cached_pal.toml` PASS ; aucune fonction `gsu*` dans `never_executed.txt` |
| G PF6 — IRQ « waits » au lieu de « dropped » | **corrigé** | `superfx.h:197-200` : « acknowledged and DROPPED — your IRQ handler never sees it » ; reste absent de `KNOWN_LIMITATIONS.md` (02-10) |
| G PF7 — `dsp1.h` « without saying what the bug was » | **corrigé** | `dsp1.h:258-267` décrit le bug DSP1/1A et la commande `$2F` |
| G PF8 — `sram.asm:35-38`, `hdr_superfx.asm:52-56`, `ROADMAP.md:143`, `dsp1.h:12` | **corrigé** | `sram.asm:35-38` décrit le support SA-1 ; `hdr_superfx.asm:52-56` décrit l'en-tête étendu ; `dsp1.h:12` dit `$03`/`$05` ; le tableau des modules a quitté `ROADMAP.md` (76 lignes) |
| G PF10 — protocole matériel sans sauvegardes ni présentation, rangée 16 sans `dsp1b.bin` | **corrigé** (le volet firmware est **partiel**) | Rangées 24, 25 et 26 (`superfx_game_skeleton`, `sa1_save`, `superfx_save`) ; la rangée 16 nomme les dumps mais ne demande pas de noter lequel est chargé (02-8) |
| G rec 11 — lignes partenaires (luna : power-on cartouche, DSP-1 2 Mo, MS0 ; snes-rag : changelog Super FX, `dsp1.bin`/`dsp1b.bin`) | **partiel** | Toutes écrites (`OPEN_luna.md:16-17`, `OPEN_snes-rag.md:13-15`). luna : le power-on de la RAM de cartouche est livré et épinglé (aide de `--power-on` en 1.38.0) ; MS0 et le DSP-1 par carte sont livrés sur `develop` `1e0df1f` mais absents de la v1.38.0 épinglée (champ `ms0_at_21mhz` absent de `.gsu`). snes-rag : la ligne Super FX de `sd2snes-changelog` n'est toujours pas servie (requête rejouée ce jour : seul `99d8550526ffa29f`) |
