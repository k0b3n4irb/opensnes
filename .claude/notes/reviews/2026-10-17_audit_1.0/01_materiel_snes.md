Expert 01 — matériel SNES (65816, PPU, DMA/HDMA, NMI, timing) · 2026-10-17 · HEAD audité `73a3fdfe` (develop), luna v1.38.0 · mesures dans le worktree `opensnes-audit`

# Audit 1.0 — matériel SNES

## Périmètre couvert

- `templates/crt0.asm` lu en entier sur les chemins matériels : `Start` et `InitHardware` (l. 342-912), `NmiHandler` à `WaitForVBlank` (l. 1007-1542), `IrqHandler`/`DefaultIrqHandler` (l. 2073-2092), `tilemapFlush` (l. 2120-2187), entrée des blobs Super FX. Vecteurs de `templates/hdr.asm`. Règle `nmi_audit.md` appliquée point par point.
- Chemins de la lib qui touchent le matériel en dehors du handler : `hdma.asm`/`hdma.c` (setup, enable, iris, gradient de luminosité), `superfx.asm` (`gsuSetupHdmaBlanking`, `gsu_present_step*`), `console.c` (INIDISP, NMITIMEN), `mode7.c`, `sprite.c`/`sprite_dynamic.asm` (masquage des sprites, file VRAM), lecteurs Super Scope (masquage du bus ouvert).
- Mesures sur luna v1.38.0, toutes rejouables dans le worktree :
  - `python3 testing/nmi_budget.py --report` puis le gate : 12/12 dans le plafond et la dérive.
  - `python3 testing/vram_dma_blank.py` : 85/85 propres sur 200 images.
  - `make hardware-preflight` : 26/26 lignes prêtes (3 graines aléatoires et PAL).
  - Sonde personnelle des écritures CPU et DMA sur `$2104`/`$2118`/`$2119`/`$2122` hors blank (`luna state --mem-trace --trace-writes`) : 85 ROM au repos sur 300 images, puis **119 exécutions pilotées** par les scripts d'entrée des manifestes (`rom_coverage.manifest_runs`).
  - Sonde personnelle des framebuffers : 108 points de capture, `--power-on zero` comparé à `random=1/2/3`.
  - Sonde personnelle du moment où le handler interroge `$4212` pour la première fois.
  - Sonde personnelle des DMA générales concurrentes d'un HDMA sur les lignes affichées.
  - Une ROM variante (Super FX + HUD texte) pour reproduire le constat 01-1, avec son contrôle négatif.
- Arbitrage au corpus Cartouche (`exclude_sources=["opensnes-docs","opensnes-notes-tech"]`) de 12 affirmations de `KNOWN_LIMITATIONS.md`, `docs/hardware/` et `crt0.asm`, plus 5 errata que le SDK ne cite pas encore (tableau en fin de section « Points forts »).
- Hors périmètre, à peine effleuré : SA-1 et DSP-1 (→ expert 02), audio (→ expert audio), la qualité du compilateur.

## Points forts

- **Le handler NMI suit la règle et coûte peu.** Ordre : file dynamique, OAM, tilemap, scroll, Super FX, callback, pads. Les registres sont sauvés en 16 bits et le D est isolé sur `tcc__nmi_registers`. Tout accès système passe en `.w`. Le D est remis à zéro par `rep #$38`. Pire image mesurée aujourd'hui : 8 568 mclk (breakout), soit 71 % du plafond de 12 000 et 17 % de la VBlank NTSC.
- **Aucune écriture PPU hors blank sur le corpus, y compris sous entrée.** Aucune écriture VRAM, OAM ou CGRAM (CPU ou DMA) n'a lieu hors V-blank ou forced blank, ni sur les 85 ROM au repos, ni sur les 119 parcours pilotés par les manifestes. Les seules exceptions sont voulues et légales : CGRAM écrite par HDMA (tetris, gradient_colors, hdma_indirect_gradient) et la DMA CGRAM de `hicolor_1792`, mesurée entre H = 1270 et 1300 mclk, donc en H-blank (H ≥ 274 points = 1096 mclk). Cela couvre ce que `vram_dma_blank.py` ne regarde pas : CGRAM, OAM, écritures CPU et chemins pilotés.
- **Démarrage indépendant de la RAM.** Les 108 points de capture du corpus donnent un framebuffer identique au pixel près entre `--power-on zero` et `random=1`, `random=2`, `random=3` (luna remplit WRAM, VRAM, CGRAM, OAM et ARAM). Le passage en forced blank et la coupure de NMITIMEN/HDMAEN dès le vecteur reset (`crt0.asm:605-609`, arbitré dans le commentaire) font leur travail. C'est un cran plus fort que le preflight, qui ne vérifie que la vivacité.
- **Lecture des pads sûre.** Le handler interroge `$4212` pour la première fois à H ≈ 1152 mclk (point 288) de la ligne 225, sur toutes les ROM mesurées (`basics/*`, `input/controller`). L'auto-lecture démarre entre les points 32,5 et 95,5 (snesdev-wiki `e4cef4cc52ed4ebf`), donc le piège de l'errata (`$4212` testé trop tôt) ne peut pas se produire. Les bits de bus ouvert sont masqués : `$4016`/`$4017` par `and #$01`, OPHCT/OPVCT et STAT78 dans `ReadScope`.
- **Le scroll vertical −1 est appliqué partout où la lib écrit** : sync NMI (4 BG, BG3 brut en mode OPT), `mode7SetScroll`, `mode7Init` (`0x017F`), valeurs de reset. L'arbitre l'énonce mot pour mot (anomie-regs `c5ed07ac9c3b9ada`).
- **HDMA en milieu d'image traité à la racine.** `hdmaSetup` pré-charge A2A et NTRL (`hdma.asm:113-138`), exactement le geste demandé par l'arbitre (« you must basically do the init process manually by setting $43x8-A », anomie-regs `d30459cd9066579f`).
- **Les constats B du 03/10 de mon périmètre sont tous traités, sauf un.** La réentrance en callback est fermée et le contrat est écrit sur `nmiSet()` (`interrupt.h:49-86`). Les hooks `tilemapFlush` et `oamDynamic*` sont dans le gate NMI. Le canal GSU est nommé dans `hdma.h:71`. Le coût de l'iris est écrit. Le reste est en section 7.

**Affirmations matérielles arbitrées** (12 du SDK + 5 errata nouvelles ; ✔ = une phrase de l'arbitre énonce le point et sa valeur) :

| # | Affirmation (fichier) | Arbitre, chunk | Verdict |
|---|---|---|---|
| 1 | VRAM ignorée hors V-blank/forced blank, y compris en H-blank ; VMADD s'incrémente quand même (`KNOWN_LIMITATIONS.md:29-37`) | snesdev-wiki `4e0a1bda9e0e98e5` | ✔ |
| 2 | 37 lignes, 1364 mclk/ligne, 48 988 hors refresh, 8 mclk/octet, ≈ 6 Ko bruts (`KL:43-55`) | snesdev-wiki `62477989f869da50`, `c0c7157e69e13ebf` (6 123 octets) | ✔ |
| 3 | Scanline 0 jamais affichée, VOFS = −1 (`KL:121-131`, `crt0.asm:379-386`) | anomie-regs `c5ed07ac9c3b9ada` | ✔ |
| 4 | BG3 en OPT : VOFS choisit les rangées de la table, la verticale est la suivante (`KL:142-147`) | snesdev-wiki `d594aeedde1b87c2` (« as if 8 was added to VOFS ») | ✔ |
| 5 | CGRAM écrite pendant l'image → mauvaise entrée (`KL:149-153`) | snesdev-wiki `60df1d0a294af255` | ✔ |
| 6 | IRQ H à HTIME + 3,5 points (`KL:155`) | fullsnes `ae801928f9327bcc` | ✔ |
| 7 | Drapeau H-blank à H = 274 (`KL:156`) | anomie-timing `08a81c8c93552908`, fullsnes `ae801928f9327bcc` | ✔ (`snes_verify` a rendu `arbiter_covers_topic_only` ; voir « Risques ») |
| 8 | Verrou d'écriture double partagé entre BG1 scroll et Mode 7, MPY corrompu (`KL:166-176`) | fullsnes `6b054a12234f8aa4`, snesdev-wiki `78c0e95f65b95a86` | ✔ (`snes_verify` : `unsettled`, alors que les phrases l'énoncent) |
| 9 | Pad : 16 bits puis des 1 ; `$4017` bits 2-4 lus à 1 (`crt0.asm:1411-1419`) | anomie-regs `56a1f95cb6d41c36`, `786c3baa4eacf2bd` | ✔ |
| 10 | Démarrage HDMA en milieu d'image : A2A et NTRL à poser à la main (`hdma.asm:118-131`) | anomie-regs `d30459cd9066579f`, `e94b100cf8bb1d70` | ✔ |
| 11 | Auto-joypad lancé tôt dans la VBlank, environ 4 K mclk (`crt0.asm:1354`) | fullsnes `ae801928f9327bcc` (H 32,5-95,5, 4224 clks) | ✔ |
| 12 | Sprite à X = 256 compté comme X = 0 pour range/time (`docs/hardware/OAM.md:137-141`) | anomie-regs `2304edd2bf6755b9`, snesdev-wiki `10ee3c4bff0f187f` | ✔ |
| E1 | S-CPU v1 : plantage si une DMA générale finit juste avant un HDMA | Nintendo manual `aa3f50af62cded1a`, snesdev-wiki `5d101c645805e974` | non cité par le SDK |
| E2 | S-CPU-A : un HDMA récent avec BBAD = $00 (INIDISP) peut faire échouer une DMA ; parade : mode 1 vers `$21FF` | snesdev-wiki `97b76d7d4e2e34a6`, undisbeliever `8a82d25de6e99ae5`, `9cde32ae8100f0c1` | non cité → 01-2 |
| E3 | 1-CHIP : la luminosité INIDISP change progressivement ; forcer le blank avec `$8F`, pas `$80` | snesdev-wiki `172b5b77ea6fae37` | non cité → 01-2 |
| E4 | NMITIMEN.7 armé alors que RDNMI.7 est levé → NMI immédiate | snesdev-wiki `172b5b77ea6fae37` | non cité → 01-7 |
| E5 | Forced blank levé en milieu de ligne → ligne glitchée | anomie-regs `efc3c17af0c4d30c` | non cité → 01-10 |

Aucune des affirmations 🔴/🟠 de `KNOWN_LIMITATIONS.md` échantillonnées n'est fausse.

## Points faibles

**01-1 🔴 — La lib utilise elle-même le canal 1 deux fois. Un jeu Super FX avec letterbox et HUD texte reste sur un écran noir.**
- **Les deux usages.** `gsuSetupHdmaBlanking` programme le **HDMA canal 1** vers INIDISP (`lib/source/superfx.asm:441-462`). Dans le NMI, `tilemapFlush` reprogramme le **canal 1** en DMA générale vers `$2118` (`templates/crt0.asm:2144-2175` : `$4310 = $1801`, `$4312` = tampon texte). `oamVramQueueUpdate` fait de même (`lib/source/sprite_dynamic.asm:518-576`).
- **Ce qui se passe.** À l'image suivante, le canal HDMA 1 relit A1T, qui pointe désormais sur le tampon texte. La table se termine aussitôt et INIDISP reste figé sur la dernière bande (`$80`, forced blank).
- **Reproduit sur luna.** Variante de `chips/superfx_game_skeleton` : `text` ajouté à `LIB_MODULES`, `textInit(0x5000,0,0)` avant `gsuSetupHdmaBlanking`, `textPrintAt(0,0,"HUD")` toutes les 64 images. L'image 300 est **noire** ; elle montre le cube sans le `textPrintAt` (contrôle négatif). Avec `--trace-writes 4311` : BBAD1 passe de `$00` à `$18` (6 écritures), et les écritures HDMA1 vers `$2100` tombent de 1449 à 382 sur 400 images.
- **Personne ne prévient l'utilisateur.** Aucun en-tête ne le dit. `docs/tutorials/superfx.md:266-267` affirme même « combines with your own HDMA channels; just leave channel 1 to it », et `hdma.h:68` attribue le canal 1 au seul flush texte (le moteur de sprites dynamiques s'en sert aussi). La vivacité luna passe (NMI vivante), donc aucun gate ne le voit.
- **Couche** : lib + templates.
- **Correctif (S).** Faire tourner les deux DMA du NMI sur le **canal 7**, libre dès la fin de la DMA OAM (les DMA générales s'exécutent l'une après l'autre dans le handler). Le canal 1 revient alors au seul GSU. Ajouter une fixture « letterbox + texte + sprites dynamiques » qui asserte l'image.

**01-2 🟠 — Les HDMA vers INIDISP suivent la forme que deux errata matériels déconseillent.**
- **Où.** `gsuSetupHdmaBlanking` (mode 0, BBAD = `$00`, valeurs `$80`/`$0F`, `superfx.asm:402-409, 442-445`) et `hdmaBrightnessGradient` (`HDMA_MODE_1REG` vers `HDMA_DEST_INIDISP`, `hdma.c:356`).
- **Errata S-CPU-A.** Un HDMA récent avec BBAD = `$00` peut faire échouer une DMA en silence (« Nothing will happen and the DMA size registers … unchanged », snesdev-wiki `97b76d7d4e2e34a6` ; seule la révision S-CPU-A est touchée, undisbeliever `8a82d25de6e99ae5`). Le chemin `gsuPresent` fait justement ses DMA framebuffer → VRAM dans les bandes créées par ce HDMA. Mesuré : sur 160 images, 123 (`superfx_game_skeleton`) et 154 (`superfx_3d`) ont une DMA générale sur les lignes 0-224 pendant que le HDMA INIDISP tourne.
- **Errata 1-CHIP.** Sur un 1-CHIP, le passage `$80` → `$0F` fait un fondu sur 72 pixels ou plus à chaque bord de bande, à chaque image. L'arbitre recommande `$8F` (`172b5b77ea6fae37`).
- **luna ne modélise aucune révision de CPU** (`luna state --help` : pas d'option de version), donc aucune mesure ne peut le voir.
- **Correctif (S)**, celui de l'arbitre (`9cde32ae8100f0c1`) : mode 1 vers `$21FF`, octet doublé dans la table, et `$8F` pour les bandes noires. Ajouter l'entrée à `KNOWN_LIMITATIONS.md`.

**01-3 🟠 — Le protocole console ne relève pas ce qui distingue deux consoles, et le preflight ne regarde pas l'image.**
- **Révisions non relevées.** `docs/HARDWARE_VERIFICATION.md` demande « model, region » mais pas les révisions S-CPU (`$4210` bits 0-3), PPU1 (`$213E`) et PPU2 (`$213F`). Aucun exemple ne les affiche (`grep -rln '4210\|213E' examples` : rien d'utile). Or deux bugs dépendent de la révision (E1 v1, E2 v2A) et un troisième du 1-CHIP (E3), et les lignes 5, 6, 18 et 24 exercent HDMA + DMA. Un KO sur ces lignes ne sera pas interprétable.
- **Preflight sans image.** `hardware_preflight.py` ne vérifie que la vivacité (« Nothing is compared to a baseline »). Ma comparaison de framebuffers sous RAM aléatoire (section « Points forts ») est bon marché et devrait en faire partie.
- **Correctif (S)** : une ligne 0 « identité de la console » (une ROM qui affiche les trois versions et la région) et ces champs dans la grille ; dans le preflight, comparer le `fbhash` à la base sous `random=N`.

**01-4 🟡 — Le gate VBlank ne dit pas ce qui reste de la VBlank au jeu.**
- Les lignes `EXTRA` (`nmi_budget.py:93-111`) n'ont **pas de plafond**, seulement une dérive. Le pire mesuré aujourd'hui est `tilemapFlush` à 17 972 mclk (`scene_stack`, image 62). Ajouté au handler (≈ 8 500), cela prend la moitié des 50 468 mclk de la VBlank NTSC. Le « 4 Ko pratiques » de `KL:49-51` tient encore, mais rien ne l'asserte.
- Le budget inclusif (appelés compris) n'a jamais été demandé à luna (`OPEN_luna.md`, rien sur « inclusive »).
- C'est le reste du B PF2 (section 7). **Effort** : S (somme sur une même image ; requête à luna).

**01-5 🟡 — Course entre `sta vblank_flag` et `wai` (`crt0.asm:1534-1538`).**
- Si la NMI tombe entre les deux instructions, le handler fait le travail et efface le drapeau. Le `wai` dort ensuite jusqu'à la NMI suivante, qui est comptée en lag frame : pas de DMA OAM, pas de pads. Le jeu perd une image et `lag_frame_counter` ment.
- La fenêtre est d'une instruction, environ 1/10 000 image pour une boucle à durée aléatoire. Analyse de code, **non reproduit**.
- **Correctif (S)** : tester le drapeau avant `wai` ne suffit pas. Comparer `frame_count` avant/après, ou attendre sans `wai` quand le drapeau est déjà à 0.

**01-6 🟡 — L'OAM de démarrage cache les sprites à X = 256, que la lib a abandonné le 03/10.**
- `InitHardware` écrit X = 0 avec le bit haut, soit X = 256 (`crt0.asm:489`, `:506`). Le commentaire « Must match oamClear() behavior » (`:503`) est faux : `oamClear`/`oamHide` utilisent X = 257 (`sprite.c:121-129, 181`), parce que X = 256 compte comme X = 0 pour range/time (anomie-regs `2304edd2bf6755b9`) et pour **toutes** ses tranches (snesdev-wiki `10ee3c4bff0f187f`).
- **Impact faible** : taille 8×8 par défaut, et `oamInit` appelle `oamClear`.
- **Correctif (S)** : écrire `lda #1` au lieu de `stz`.

**01-7 🟡 — NMI armée sans lire RDNMI** (`crt0.asm:821-823`, `console.c:110-112`). Si l'armement tombe pendant la VBlank avec RDNMI.7 levé, la NMI part aussitôt (errata `172b5b77ea6fae37`) : `frame_count` avance deux fois dans la même VBlank. **Correctif (S)** : `lda $4210` juste avant, comme le propose l'arbitre.

**01-8 🟡 — `hdmaIrisWipe` attend une VBlank à chaque appel, pas seulement au premier.** `hdma.c:513` appelle `WaitForVBlank()` sans condition, alors que `hdma.h:625-628` dit « the FIRST call also waits one VBlank ». L'exemple de l'en-tête (`hdma.h:606-609`) appelle donc deux attentes par itération. **Correctif (S)** : aligner le code ou la doc.

**01-9 🟡 — `hdmaBrightnessGradientStop` écrit `REG_INIDISP = 0x0F`** (`hdma.c:363`) sans passer par `console_force_blanked`/`console_brightness`. Écran éteint, il le rallume ; luminosité réglée à 8, il la remet à 15. **Correctif (S)** : `setBrightness(console_brightness)`.

**01-10 🟡 — `setScreenOn()`/`setScreenOff()` écrivent INIDISP n'importe où dans l'image.**
- Seuls 16 des 84 `main.c` qui l'appellent le précèdent d'un `WaitForVBlank()`. `panel.c:87-90` fait off, DMA, on, en plein jeu.
- Effet : une ligne glitchée à la levée du blank (anomie-regs `efc3c17af0c4d30c`), une image partielle, et le pixel de l'errata « early read ».
- Cosmétique, une image. **Correctif (S)** : documenter dans `console.h`, ou différer l'écriture au NMI comme les scrolls.

**01-11 🟡 — `hdmaSetup` accepte les canaux 0, 1 et 7** (`hdma.asm:88-90` ne refuse que ≥ 8). Le canal 7 est écrasé à chaque image par la DMA OAM (`hdma.h:65`). C'est un contrat sûr seulement par la doc, contraire au principe « type-safe boundaries ». **Correctif (S)** : refuser 0, 1 et 7 (ou les seuls canaux réservés après 01-1), avec un vecteur de test.

**01-12 🟡 — `vram_dma_blank.py` ne regarde que la VRAM, au repos.** CGRAM et OAM ne sont pas vérifiées, ni les chemins pilotés par entrée (le D T8 du 03/10 reste ouvert dans le code). Ma sonde (CPU + DMA sur `$2104`/`$2118`/`$2119`/`$2122`, 119 parcours pilotés) est propre aujourd'hui ; elle est à intégrer. **Effort** : S.

**01-13 🟡 — BRK et COP reviennent en silence.** `EmptyHandler` (`hdr.asm:69-70, 104-108`) fait `rti`. Un saut dans de la RAM nulle (BRK) continue donc sans trace, et la vivacité luna ne le voit pas, puisque la NMI continue. **Correctif (S)** : `stp` ou une boucle qui écrit `SNES_ASSERT`/WDM.

**01-14 🟡 — L'exemple « attendre l'auto-joypad » de `docs/hardware/REGISTERS.md:368-370`** (`while (REG_HVBJOY & 0x01) {}`) ne suffit pas juste après le début de la VBlank, selon l'errata snesdev-wiki `e4cef4cc52ed4ebf`. Le handler du SDK est sûr (mesuré), mais pas la recette documentée. **Correctif (S)** : une phrase dans la doc.

→ expert 05/tests : `testing/CORPUS_COVERAGE.md` est modifié dans le worktree après un `make` (non commité ; à vérifier si c'est attendu).

## Risques

1. **La session console sur une révision S-CPU v1 ou v2A, ou sur un 1-CHIP** (01-2, 01-3). Ce sont les trois écarts luna/console que je juge les plus probables. Les deux errata CPU sont des pannes silencieuses de DMA. Aucun outil du projet ne peut les voir, puisque luna ne modélise pas de révision. Le corpus n'en montre aucun cas hors Super FX : 0 DMA générale sur les lignes affichées pendant un HDMA, sur les 85 ROM au repos. Le risque se concentre donc sur les lignes 18 et 24 du protocole, et sur tout jeu utilisateur qui utilise `hdmaBrightnessGradient`.
2. **Port 2 vide.** La valeur du 17e bit sur un port vide n'est donnée par aucune référence (`crt0.asm:1416-1418`, assumé honnêtement). La ligne 2 du protocole ne demande pas de démarrer **sans** pad en port 2 pour lire `padIsConnected(1)`. C'est à ajouter : une observation de cinq secondes, qui tranchera aussi le point pour le corpus.
3. **Bus ouvert et état PPU au démarrage.** luna randomise les RAM, pas les registres PPU. `InitHardware` écrit presque tout. Restent non écrits la CGRAM au-delà de l'entrée 0 (vidée par `consoleInit`) et COLDATA (`stz $2132` n'écrit aucun canal, `crt0.asm:446` ; c'est sans effet tant que les maths de couleur sont coupées). Risque faible.
4. **Les conflits de canal entre modules** (01-1) sont une classe, pas un cas. La lib a 4 usagers internes de canaux DMA (0, 1, 7, et 1 pour le GSU). Aucune table ne les recense et aucun test ne combine les modules qui les partagent.
5. **Arbitrage outillé imparfait.** Deux fois aujourd'hui, `snes_verify` a rendu `arbiter_covers_topic_only`/`unsettled` alors qu'un arbitre énonce le point mot pour mot. Le cas H = 274 est dans anomie-timing `08a81c8c93552908`, dont la phrase n'est pas dans `evidence`. Le cas du verrou BG1/Mode 7 est dans fullsnes `6b054a12234f8aa4` et snesdev-wiki `78c0e95f65b95a86`. À reporter à snes-rag (`OPEN_snes-rag.md`) : je n'ai pas le droit d'écrire hors de ce rapport. Même chose pour luna : la colonne `hclock` des écritures HDMA vaut 0 dans `--mem-trace`, alors que le HDMA a lieu vers H = 278 (fullsnes `ae801928f9327bcc`). Une demande « révision S-CPU / PPU / 1-CHIP » est aussi à faire.

## Améliorations recommandées

| # | Action | Constat | Sévérité | Effort |
|---|---|---|---|---|
| 1 | Passer `tilemapFlush` et `oamVramQueueUpdate` au canal DMA 7 ; fixture letterbox + texte + sprites dynamiques avec assert d'image ; tableau des canaux réservés dans `hdma.h`/`dma.h`, et correction de `superfx.md:266-267` | 01-1 | 🔴 | S |
| 2 | HDMA INIDISP en mode 1 vers `$21FF`, octets doublés, `$8F` pour le blank (`superfx.asm`, `hdma.c`) ; entrée KL « révisions S-CPU » | 01-2 | 🟠 | S |
| 3 | Ligne 0 « identité console » (ROM qui affiche S-CPU/PPU1/PPU2/région), champs de grille ; ligne « port 2 vide » ; preflight qui compare le `fbhash` sous RAM aléatoire | 01-3, risque 2 | 🟠 | S |
| 4 | Somme handler + hooks sur une même image avec un plafond ; demande d'un `--budget` inclusif à luna | 01-4 | 🟡 | S |
| 5 | `WaitForVBlank` sans fenêtre de course (compare `frame_count`) | 01-5 | 🟡 | S |
| 6 | `crt0` : OAM de démarrage à X = 257 ; `lda $4210` avant d'armer la NMI (crt0 et `consoleInit`) | 01-6, 01-7 | 🟡 | S |
| 7 | `hdmaIrisWipe` : une seule attente ou doc juste ; `hdmaBrightnessGradientStop` par `setBrightness` | 01-8, 01-9 | 🟡 | S |
| 8 | `hdmaSetup` refuse les canaux réservés | 01-11 | 🟡 | S |
| 9 | `vram_dma_blank.py` étendu à CGRAM/OAM/CPU et aux scripts d'entrée des manifestes | 01-12 | 🟡 | S |
| 10 | Doc : `setScreenOn` hors VBlank, recette HVBJOY, BRK piégé | 01-10, 01-13, 01-14 | 🟡 | S |
| 11 | Rapports partenaires : snes-rag (deux faux `unsettled`), luna (`hclock` HDMA, révisions de puces) | risque 5 | 🟡 | S |

## Verdict

Le runtime NMI, le budget VBlank, les écritures PPU hors blank et le démarrage sous RAM aléatoire sont en état de 1.0. Je l'ai mesuré au-delà des gates existants : 119 parcours pilotés propres, 108 images identiques sous RAM aléatoire, et toutes les affirmations matérielles échantillonnées confirmées par l'arbitre. Un défaut 🔴 bloque pourtant : la lib occupe le canal 1 deux fois. Un jeu Super FX avec letterbox qui affiche du texte (ou des sprites dynamiques) reste sur un écran noir, sans aucun signal. Avant la session console qui conditionne la 1.0, deux correctifs bon marché s'imposent : la forme HDMA-vers-INIDISP recommandée par l'arbitre (01-2), et le relevé des révisions de puces dans le protocole (01-3). Sans eux, la session risque de produire des KO qu'on ne saura pas lire.

## Suivi de l'audit du 03/10

| Constat (lettre B, partie runtime/NMI, et D T8 qui porte sur `vram_dma_blank.py`) | État | Preuve d'aujourd'hui |
|---|---|---|
| B l.21 / PF3 — réentrance en callback NMI | **corrigé** | `interrupt.h:49-86` (contrat de `nmiSet`, liste « non sûr ») ; six routines passées en accès `.b` (suivi B du 10-05) |
| B l.31 — canal HDMA 1 pris par le GSU, non dit dans `hdma.h` | **corrigé** | `hdma.h:71-72` ; mais voir 01-1 : le canal 1 est **aussi** pris par la lib dans le NMI, ce que `hdma.h` ne dit pas pour les sprites dynamiques |
| B PF2 — `nmi_budget.py` ne mesure que le temps propre du handler | **partiel** | `EXTRA` couvre `tilemapFlush` (17 972), `oamDynamicNmiFlush` (296), `oamVramQueueUpdate` (312), en dérive seulement, sans plafond ni somme ; aucune demande inclusive à luna (01-4) |
| B PF5 — coût caché de `hdmaIrisWipe` et `WaitForVBlank` interne | **partiel** | coût documenté (`hdma.h:623-628`), mais la doc dit « FIRST call » alors que `hdma.c:513` attend à chaque appel (01-8) |
| D T8 — oracles de corpus au repos seulement | **ouvert** dans le code | `vram_dma_blank.py:54` lance toujours `luna state --until-frame` sans entrée ; ma sonde pilotée (119 parcours) est propre, mais elle n'est pas intégrée (01-12) |
