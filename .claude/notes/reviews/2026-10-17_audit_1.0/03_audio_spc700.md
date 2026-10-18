Expert 03 — audio SNES (SPC700, S-DSP, SNESMOD, moteur audio v2) · 2026-10-17 · HEAD audité `73a3fdfe` (worktree `/home/kobenairb/workspace/opensnes-audit`, luna v1.38.0)

# Audit 1.0 — audio

## Périmètre couvert

- **Moteur v2** : `lib/include/snes/audio.h` (658 l.), `lib/source/audio.c` (761 l.), `audio_vset.asm`, `audio_stream.asm`, le pilote résident `audio_driver.spc700.asm` (705 l., 959 octets assemblés), `apu.asm` / `apu.h`.
- **SNESMOD** : `snesmod.h`, `snesmod.asm` (la partie 65816), `sm_spc.asm` (le pilote SPC700), la copie du pilote dans l'outil (`tools/lib/smconv/src/spc_program.h`), `.claude/notes/chantiers/audio_v2.md`, `audio_beyond_snesmod.md`, `partners/snesmod-upstream/`.
- **Outils** : `opensnes-music` (IT → soundbank, `spc`, `inspect`) et `opensnes-sample` (WAV → BRR), l'encodeur BRR commun `tools/lib/smconv/src/brr.c`, `it2spc.c`.
- **Tests** : `testing/audio_regress.py`, `phase_sweep.py`, les manifestes audio, `fixtures/libtests` (lignes audio).
- **Mesures dans le worktree** (toutes rejouables) :
  - `python3 testing/audio_regress.py` donne 11/11 MATCH ; `python3 testing/phase_sweep.py` donne 48/48 ; `test_libtest.py` donne 274/274.
  - `luna test` passe sur `audio_v2`, `lib_audio_stream`, `libtest_snesmod`, `audio_echo_wet*`, `audio_snesmod_sfx_play`, `audio_sfx_from_wav` et `footprint_music_aram` : 8/8.
  - Trois ROM de sonde construites dans mon scratchpad contre le worktree : KON enchaînés, position SNESMOD, et la disposition de l'ARAM (`luna spc-dump` comparé à `opensnes-music spc`).
- Les affirmations matérielles ont été arbitrées au corpus, avec `exclude_sources=["opensnes-docs","opensnes-notes-tech"]`. Les identifiants des passages sont cités à chaque fois.

## Points forts

- **Le pilote v2 est à nous, lisible et construit depuis sa source.** Le fichier `.asm` est commenté instruction par instruction et assemblé par `wla-spc700` dans `lib/Makefile:156-171`, sous un budget de 2 Ko. Un protocole à numéro de séquence (1..3, `audio.c:181`) rend l'accusé de réception sans ambiguïté. `$FE` reste réservé à la permutation à chaud (`apuReset`). Toutes les attentes côté CPU ont une borne (`ACK_SPIN_MAX`).
- **Les courses CPU↔SPC700 connues sont fermées, et chacune a un test.**
  - L'épilogue de chargement repose sur une poignée de main en trois temps où chaque valeur reste affichée jusqu'à la réponse (`audio_driver.spc700.asm:616-652`).
  - L'échantillon de 513 octets et celui de 512 sont testés (`r_audio_load513`, `r_audio_load512`).
  - 400 chargements tournent sous un callback NMI lourd, à des phases de trame différentes (`libtests/main.c:660-670`, `r_audio_phase = 0`).
- **Le relâchement KOFF a été mesuré avant d'être corrigé** (dans SNESMOD et dans le pilote v2, qui garde 384 cycles entre les deux écritures, `:198`).
- **KON enchaînés : aucune perte mesurée.** L'arbitre prévient que deux écritures rapprochées de KON ne gardent « usually … only the second value » (fullsnes `5c23c8e8dbaf1931`). J'ai lancé huit `audioPlaySampleOn` d'affilée, d'abord avec le chemin complet (VOL, P, ENV, KON) puis avec KON seul, et deux KON coup sur coup. Résultat : ENVX actif sur 0xFF, 0xFF puis 0x03, soit toutes les voix demandées (sonde `kon.toml`, PASS). L'aller-retour du protocole dépasse la fenêtre de scrutation de 64 cycles.
- **Plusieurs défauts de documentation et de code de l'audit du 03/10 sont corrigés** : enveloppe (D1), centre du panoramique, bornage des volumes, `AUDIO_PITCH_C*`, volume SNESMOD 0-255, 17e effet, `apuWaitBoot` borné, `apuUpload(…, 0)`, `loopPoint` (détail en §7).
- **Ce qu'un compositeur obtient en effets sonores est solide** : 8 voix, panoramique et pitch sur 14 bits, ADSR et GAIN, écho avec un FIR à 8 coefficients et EFB, et un anneau d'écho dimensionné au délai demandé. Le chargement d'échantillons peut se faire par pas pendant le jeu (`audioLoadSampleBegin/Step`), et la commande `VSET` règle volume et pitch d'une voix en une seule poignée de main.
- **L'encodeur BRR est celui de smconv, et il est bon.** Il essaie les 4 filtres et les 13 décalages pour chaque bloc, teste le débordement gaussien et ré-encode le bloc de boucle en filtre 0 si la perte est trop forte. La boucle est alignée par déroulage ou rééchantillonnage, et l'outil publie `_tuning` pour compenser.
- **Le suivi amont est tenu.** Cinq issues sont ouvertes chez `mukunda-/snesmod` (#6 à #10), avec un relevé hebdomadaire daté (dernier : 2026-10-17).

## Points faibles

**03-1 🟠 — `opensnes-music spc` écrit un `.spc` dont le module n'est pas à l'adresse où le pilote le lit.**
- Dans le jeu, `luna spc-dump examples/audio/snesmod_music/music.sfc` montre l'en-tête du module à `$1A00` (`806e0324…`). Les deux copies du pilote le lisent là : octets `e5 00 1a`, c'est-à-dire `mov a,!$1A00`, à l'offset `0x2c4` de `sm_spc.asm` et `0x2ef` de `spc_program.h`.
- Le `.spc` produit par `bin/opensnes-music spc examples/audio/snesmod_music/music/pollen8.it` place ce même en-tête à **`$19AA`**, et `$1A00` y vaut `ff ff …`.
- Cause : `it2spc.c:21` fixe `module_base = 0x18ca`, plus bas que la fin du pilote de l'outil (`$0400` + 5546 octets = `$19AA`). Le remplissage `io_zero_fill(&f, module_base - driver_base - spc_program_size)` (`:936`) reçoit donc une taille négative (−224) et ne remplit rien. La constante juste, `MODULE_BASE = 0x1A00`, existe à `:37` mais rien ne l'utilise.
- Conséquence probable : le `.spc` de prévisualisation que l'outil 1.0.0 annonce ne joue pas le morceau. La disposition est mesurée ; la lecture dans un lecteur SPC ne l'est pas.
- Effort S.

**03-2 🟠 — Un module trop grand pour l'ARAM ne produit qu'un avertissement.**
- `tools/opensnes-music/src/main.c:165-168` fait un `cli_warn(…"more than the %d a module may take")`, puis `spc_bank_export` écrit la banque, avec le code de sortie 0.
- Le `--help` promet le contraire : « 1 the input was refused (the message names the limit) … A refused input writes nothing ».
- Le refus hérité de smconv (`it2spc.c:642-646`, `exit(1)`) est enfermé dans `if (g_verbose)` (`:610`) : il ne joue jamais sans `-v`.
- La limite elle-même est calculée faux, sur `$18CA` (`SPC_RAM_FOR_MUSIC`, `main.c:58`), et `envsize` recompte `spc_instrument_export_size` déjà compté dans `instrsize` (`it2spc.c:599-603`). Le chiffre est pessimiste, mais il est faux.
- Conséquence : une soundbank qui déborde l'ARAM est liée dans le jeu, et le pilote lit ou écrit hors de sa mémoire à l'exécution. Le format de la soundbank fait partie de ce que `STABILITY.md` gèle.
- Effort S : refuser, et calculer la limite depuis `$1A00`.

**03-3 🟠 — Le pilote SNESMOD n'a toujours pas de source, et il en existe deux binaires différents.**
- `lib/source/sm_spc.asm` est fait de 173 lignes `.byte` (5522 octets), patchées à la main en place (KOF, `:21-28`). L'outil embarque une autre construction : `tools/lib/smconv/src/spc_program.h`, 5546 octets, dont 4854 diffèrent à offset égal. Rien ne dit d'où elle vient ni si le correctif KOF y figure.
- La 1.0 gèlerait les fonctions `snesmod*` et le format de la soundbank au-dessus d'un code que personne ici ne peut reconstruire, tracer ou corriger autrement qu'octet par octet.
- C'est H PF17, toujours ouvert. La décision écrite demandée (« désassembler ou geler tel quel ») n'est pas prise. Le seul plan de sortie, `audio_beyond_snesmod.md`, est au statut PLANNING depuis 2026-07-27 et ses chiffres sont périmés (627 octets, ESA `$C000`).

**03-4 🟡 — `snesmodGetPosition` rend la position dans la liste d'ordres, pas la ligne.**
- J'ai échantillonné la valeur toutes les 8 trames sur `pollen8` (sonde `pos.sfc`, `--peek r_pos:40`). Elle donne `00`×8, `01`×16, `02`×17, `03`… : elle avance toutes les ~128 trames, au rythme d'un pattern et non d'une ligne.
- `snesmod.h:164-167` dit encore « row number ». C'est l'addendum B du 03/10, toujours ouvert.
- Effort S.

**03-5 🟡 — Des alias hérités seraient gelés avec leur piège.**
- `audio.h:654` : `#define audioSetPan(id, pan)` ne s'expanse en **rien**, donc un appel est ignoré sans un mot.
- `audio.h:651` : `audioStopSample(id)` prend une **voix**, pas un échantillon.
- `STABILITY.md` gèle « le nom et le sens » de chaque macro. Il faut les déprécier (avertissement clang) avant la 1.0, pour pouvoir les retirer en 2.0.
- Effort S.

**03-6 🟡 — `snesmodPlayEffect` (`snesmod.h:224`) serait gelé avec une valeur de retour « Unspecified ».**
- Son en-tête le dit lui-même : « open item of the API audit ».
- Le paramètre `pitch` est un `u16` dont seuls 4 bits comptent, soit 16 vitesses, et le volume n'a que 16 pas. C'est une limite du pilote, mais le type ment.
- Effort S : documenter le retour comme « toujours 0 », ou le rendre `void` avant le gel.

**03-7 🟡 — L'écho est plafonné à 7 (112 ms) alors que le matériel va à 15 (240 ms, 30 Ko).**
- Côté matériel : fullsnes `2c60d98928dd8624` (« 1..15=Size in 2K-byte steps (max=30K) … max=240ms ») et snesdev-wiki `85c59963d05bb619` (« DDDD * 2048 bytes »).
- La justification de `audio.h:131` (« EDL 8-15 would run the echo ring into the IPL region ») est périmée depuis que l'anneau finit à `$FF00` et prend exactement délai × 2 Ko (`audio.c:66`, `:698`) : avec EDL 15, l'anneau irait de `$8700` à `$FEFF`.
- C'est une limite pour un compositeur qui veut une réverbération longue. Le pilote attend déjà 240 ms au premier appel, donc rien d'autre ne l'empêche.
- Effort S.

**03-8 🟡 — Ce qu'un compositeur ne peut pas faire.**
- `audio.h` n'expose ni le bruit (NON), ni la modulation de pitch (PMON). Les exemples `play_noise` et `pitch_mod` passent par un programme SPC700 écrit à la main.
- L'allocation automatique (round-robin, `audio.c:614-616`) peut voler une voix que le jeu réserve (`audioPlaySampleOn(7, …)`), et rien ne permet de l'exclure.
- La musique n'existe qu'avec SNESMOD, et le module `snesmod` ne se lie pas avec `audio` (« One engine per ROM », `audio.h:15`). Un jeu ne peut donc pas avoir à la fois sa musique et des échantillons chargés en cours de jeu (voix, bruitages longs). Les effets SNESMOD sont limités à 16 emplacements et 16 vitesses.
- Le streaming d'échantillons de SNESMOD (que PVSnesLib expose) a été retiré (`snesmod.h:68`). `ROADMAP.md:62` le donne « Not started », en option facultative pour la 1.0.
- Ce n'est pas bloquant si la 1.0 le dit. C'est l'écart principal face à N-SPC et SNESMOD amont (snesdev-wiki `82ee0346422c7829` : « Audio streaming feature »).

**03-9 🟡 — `opensnes-sample` n'aide pas sur la fréquence ni sur le gain.**
- Il ne rééchantillonne pas, et n'avertit qu'au-dessus de 32 kHz (`main.c`, `if (w.rate > DSP_RATE)`). Un WAV à 22 050 Hz joue ×1,45 plus haut sans un mot ; seul le macro `_rate` du `.h` le laisse deviner.
- En cas de débordement, l'encodeur baisse l'amplitude par pas de 1 % (`brr.c`, boucle `amp -= 0.01`) sans le signaler.
- Il n'accepte que le PCM 8 ou 16 bits (pas de 24 bits ni de flottant, qui sont refusés avec un message clair).
- Effort S pour les deux avertissements ; M pour une option `--rate`.

**03-10 🟡 — `audioLoadSample` ne valide ni `size % 9` ni `loopPoint < size` (`audio.c:416-437`).**
- L'en-tête dit « must be multiple of 9 ». Une boucle qui pointe après la fin joue la mémoire qui suit.
- Effort S : refuser avec `AUDIO_ERR_INVALID_ID`.

**03-11 🟡 — Un second `audioInit()` ne rend pas la main tout de suite.**
- Une fois le pilote lancé, l'IPL ne répond plus : `apuWaitBoot` attend environ 7 trames puis rend `AUDIO_ERR_TIMEOUT`. Il n'y a pas de ré-initialisation par `apuReset`, alors que le pilote la sait faire.
- C'est documenté (« Call once »), mais un changement de scène qui réinitialise l'audio tombe dans ce piège.

**03-12 🟡 — L'oracle audio détecte un changement, pas une erreur.**
- `audio_regress.py` hache le WAV de 11 ROM sur 300 trames. Un son faux mais stable passe, et le moindre décalage de phase fait échouer. La règle de `testing.md` (citer `luna diff --audio` à chaque re-capture) est la bonne contrepartie, mais `--update` réécrit sans rien exiger (D PF9).
- La couverture SNESMOD des effets IT se limite à 4 modules. Aucun test n'assert qu'un effet SNESMOD sonne, au-delà d'un hachage dont le pic vaut 2644 (`snesmod_sfx`).
- Les manifestes à registres DSP (`audio_v2.toml`) sont, eux, de vrais oracles de spécification.

**03-13 🟡 — Chiffres de doc périmés.**
- `audio.h:54` annonce un pilote « ~500 bytes » ; il en fait 959.
- `audio_beyond_snesmod.md` donne 627 octets, ESA `$C000`, samples `$0B00→$C000`. Le code d'aujourd'hui finit l'anneau à `$FF00` et le dimensionne au délai.

→ expert 11 : la question F n°4 (ordre KON / registres dans `snesmod_sfx`) est tranchée en §7.

## Risques

- **Geler SNESMOD tel quel** fait d'un binaire de 5,5 Ko, sans auteur actif ici ni amont réactif (aucun commit depuis 2025-01-31, 0 réponse aux 5 issues), une dépendance contractuelle. Tout bug du séquenceur après la 1.0 se corrigera octet par octet, comme le KOF.
- **Deux pilotes SNESMOD divergents** : l'un sert aux jeux, l'autre à la prévisualisation. Ce que le compositeur entend dans son `.spc` n'est pas ce que le jeu joue (et, d'après 03-1, il n'entend probablement rien).
- **Un module qui déborde l'ARAM** se lie sans erreur (03-2). Le symptôme arrive à l'exécution (musique corrompue ou SPC700 planté), loin de sa cause.
- **Le moteur v2 et SNESMOD s'excluent.** Le premier jeu qui veut de la musique et des échantillons chargés en jeu devra contourner le SDK. C'est le cas que la règle `exchanges.md` appelle « un échec de la preuve ».
- **Bus factor** : le pilote v2 est compréhensible, mais aucun document d'architecture public (hors `.claude/notes`) n'explique son protocole à un contributeur.

## Améliorations recommandées

| # | Action | Constat | Sév. | Effort |
|---|---|---|---|---|
| 1 | `spc_bank_make_spc` : placer le module à `MODULE_BASE` (`0x1A00`), et faire échouer le remplissage s'il devient négatif ; ajouter un golden qui vérifie l'offset `$1A00` du `.spc` | 03-1 | 🟠 | S |
| 2 | `opensnes-music bank` : refuser (exit 1, rien d'écrit) un module plus grand que l'ARAM réelle (`$10000 − $1A00 −` en-tête) ; sortir le refus de `g_verbose` ; corriger le double compte `envsize` | 03-2 | 🟠 | S |
| 3 | Décision écrite du propriétaire sur le pilote SNESMOD pour la 1.0 (gel déclaré « binaire, sans source », ou désassemblage annoté) ; unifier les deux blobs (l'outil lit celui de `lib/`) | 03-3 | 🟠 | S (décision) / L (désassemblage) |
| 4 | `snesmodGetPosition` : « order position », pas « row » | 03-4 | 🟡 | S |
| 5 | Déprécier `audioSetPan`, `audioStopSample` (et `audioStop`) avant la 1.0 | 03-5 | 🟡 | S |
| 6 | Spécifier le retour de `snesmodPlayEffect` avant le gel | 03-6 | 🟡 | S |
| 7 | Accepter un délai d'écho de 1 à 15 ; corriger le commentaire `audio.h:131` | 03-7 | 🟡 | S |
| 8 | `opensnes-sample` : avertir sous 32 kHz et en cas de baisse d'amplitude ; `--rate` plus tard | 03-9 | 🟡 | S/M |
| 9 | `audioLoadSample` : refuser `size % 9 != 0` et `loopPoint >= size` | 03-10 | 🟡 | S |
| 10 | `audioInit` : passer par `apuReset()` quand le pilote tourne déjà | 03-11 | 🟡 | S |
| 11 | `audio_regress.py --update` : exiger un fichier de raison (la sortie `luna diff --audio`) | 03-12 | 🟡 | S |
| 12 | Après la 1.0 : NON/PMON dans `audio.h`, masque de voix réservées pour l'allocation automatique, puis le chantier séquenceur (`audio_beyond_snesmod.md`) qui lève l'exclusion `audio`/`snesmod` | 03-8 | 🟡 | M / L |

## Verdict

Le moteur v2 est prêt pour la 1.0 : pilote lisible construit depuis sa source, protocole borné, courses de chargement fermées et testées (513 octets, 400 chargements sous NMI), et une API d'effets sonores honnête une fois les petites dettes réglées (validation de `size`, alias à déprécier, plafond d'écho). La musique ne l'est pas autant : elle repose sur un binaire SNESMOD sans source, présent en deux versions divergentes, et l'outil `opensnes-music` 1.0.0 produit un `.spc` de prévisualisation mal agencé et laisse passer, avec un simple avertissement, une soundbank qui déborde l'ARAM. Je recommande de corriger 03-1 et 03-2 (deux fois S) et d'obtenir la décision écrite sur le blob (03-3) avant de geler, et de dire franchement dans la doc 1.0 que musique et échantillons chargés en jeu ne vont pas encore ensemble.

## Suivi de l'audit du 03/10

Les lignes audio de la lettre B de `2026-10-04_reste_a_traiter.md` et de son addendum, plus les constats audio des autres lettres qui touchent mon périmètre :

| Constat | État | Preuve d'aujourd'hui |
|---|---|---|
| B l.17 `audioInit` rappelée, `apuWaitBoot` sans borne | **corrigé** (le blocage) / reste 03-11 | `apu.asm:24-66` : 65 536 essais, puis rend 1 ; un second `audioInit` rend `AUDIO_ERR_TIMEOUT` après ~7 trames |
| B l.18 `apuUpload(…, 0)` envoyait 65 536 octets | **corrigé** | `apu.asm` : `lda 6,s / beq @nothing` |
| B l.19 `audioSetVoiceVolume` non borné | **corrigé** | `audio.c:298-299` ; `audio_v2.toml` (V7 200 → 127) PASS |
| B l.20 `AUDIO_PAN_CENTER` décentré | **corrigé** | `audio.c:562-582` ; `audio_v2.toml` V6 50/50 PASS |
| B l.28 course de `audioLoadSample` (513 octets) | **corrigé** | épilogue tenu (`audio_driver.spc700.asm:616-652`) ; `r_audio_load513 = 0`, `r_audio_phase = 0` (`test_libtest.py` 274/274) |
| B l.29 17e effet SNESMOD | **corrigé** | `snesmod.h:189` : rend 0xFF, rien n'est chargé |
| B D1 `AUDIO_RELEASE_*` présenté comme release | **corrigé** | `audio.h:123-131` : « sustain RATE … The S-DSP has no programmable release » |
| B D12 volume SNESMOD 0-127 | **corrigé** | `snesmod.h` : 0-255, mesuré sur luna |
| B D13 `AUDIO_PITCH_C3` « Middle C » | **corrigé** | `audio.h:103-108` |
| Addendum `loopPoint = 0` « no loop » | **corrigé** | `audio.h:225-231` (c'est le drapeau du dernier bloc qui décide) |
| Addendum `snesmodGetPosition` « row number » | **ouvert** | mesuré : position d'ordre (03-4) |
| F n°4 ordre KON / VOL / P / ENV dans `snesmod_sfx` | **tranché : sans effet audible** | `luna state …/sfx.sfc --until-frame 120 --input "40:0x0100,44:0,50:0x0100,54:0,80:0x0080,84:0" --dsp-trace sfx.csv` : KON à 1397054, VOLL +15, PH +42, GAIN +65 cycles SPC. anomie-sdsp `f16a9436d9b55cb6` : « there are 5 'empty' samples before envelope updates and BRR decoding actually begin » (160 cycles) ; les registres sont donc en place avant le premier échantillon produit. Pas d'issue amont à ouvrir |
| H PF17 blob SNESMOD sans source | **ouvert** (aggravé : un second blob différent dans l'outil) | 03-3 |
| D T4 courses vérifiées à une seule phase | **corrigé** | `phase_sweep.py` : 48/48 |
| D PF9 re-capture audio sans justification | **partiel** | la règle est écrite (`testing.md`) ; `audio_regress.py --update` n'exige toujours rien (03-12) |
