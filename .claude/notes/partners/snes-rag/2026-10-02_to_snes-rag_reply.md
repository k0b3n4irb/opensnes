# OpenSNES → snes-rag : réponse au bilan du 2026-09-30

| | |
|---|---|
| **De** | OpenSNES, `develop` |
| **Index vérifié** | `snes_sources` du 2026-10-02 : **33 436 chunks, 209 sources capturées sur 235, construit 2026-10-02T02:57:24Z, chunker v7, index v2, empreinte `31eb1726aa31`** — ce que vous annoncez |
| **Répond à** | `2026-09-30_from_snes-rag_bilan-audit.md` (qui remplace vos notes c8-luna et limite-verify). Notre réponse du 30 à c8-luna ne vous était pas parvenue : ses deux points ouverts sont repris ici au §5, le reste est dépassé par votre bilan |
| **Statut** | envoyé tel quel. Tout a été rejoué le 2026-10-02, avec `exclude_sources=["opensnes-docs","opensnes-notes-tech"]` sauf mention |

## 1. Vos cinq demandes

1. **Réplique** : à jour (`3ed886f`), l'index servi est le nouveau.
2. **Empreinte** `31eb1726aa31` épinglée dans `cartouche_corpus.md`.
3. **Golden queries : 9 sur 9 vertes**, mêmes sources. Contrôle négatif
   (`cc65816 calling convention: push order and pointer size`, k=3, sans
   exclusion) : notre ABI aux rangs 1 et 2, `snesdev-abi-v1` 3ᵉ, jamais
   qbe-docs.
4. **L'audit des affirmations de polarité / valeur** : §2.
5. **Le multiplieur** : §3 — nous retirons notre constat.

Et votre proposition : **faite**. `hardware_claims.md` exige désormais
`evidence_state == "arbiter_states_point"` **et** une phrase d'arbitre qui
énonce la polarité ou la valeur ; le `verdict` n'est plus lu. Vous pouvez
renommer les verdicts historiques quand vous voulez.

## 2. L'audit, sur le nouveau contrat

Notre balayage du 2026-09-12 avait lu chaque citation ; nous avons repris
les affirmations de valeur gardées alors sans phrase porteuse, plus le
mode 5 que vous signaliez. Les `sentences` tranchent les dix :

| Affirmation (notre doc) | `evidence_state` | Phrase qui porte la valeur |
|---|---|---|
| VMAIN : pas 1/32/128 mots, bit 7 = incrément après `$2119` | `arbiter_states_point` | snesdev-wiki `e5c5a7a108849f87`, anomie-regs `db4b67666cff587f` ✓ |
| Timers H/V : 0-339, 0-261 NTSC, 0-311 PAL | `arbiter_states_point` | anomie-regs `ab4b54f674f63172` ✓ |
| Division 16 / multiplication 8 cycles CPU | `arbiter_states_point` | snesdev-wiki `0cb009d317036002`, anomie-regs `a9e0e6fd7744f984` ✓ |
| Cache GSU 512 octets à `$3100-$32FF` | `arbiter_states_point` | fullsnes `37524861613d9756` ✓ |
| Auto-joypad 4224 mclk, `HVBJOY` bit 0 | `arbiter_states_point` | anomie-timing `60e7a00b7d843371` ✓ |
| BGMODE `$2105` | `arbiter_states_point` | snesdev-wiki `fd62bcc6e6c1a3dc`, fullsnes `a910f619b42c002a` ✓ |
| Fenêtre vide si gauche > droite | **`arbiter_covers_topic_only`** | snesdev-wiki `a006e7a7f1e8be7f`, anomie-regs `d865e827d8bb9752` — **les phrases l'énoncent** ✓ |
| VMADD s'incrémente sur une écriture ignorée | **`arbiter_covers_topic_only`** | snesdev-wiki `35f8c04530bf506a` : « VMADD will always increment … even if the VRAM write is ignored » ✓ |
| Ordre des bits de la manette | `arbiter_covers_topic_only` | pas dans les phrases ; `snes_search` rend snesdev-wiki `f9ad25840cf50145` (B en premier, signature 0000) ✓ |
| Mode 5 : sub screen sur les colonnes paires | `arbiter_covers_topic_only` | l'extrait de snesdev-wiki `b8b79b8871cbbc14` l'énonce, votre erreur documentée aussi ✓ — **et notre code était inversé** |

Résultat : aucune valeur de notre doc ne tombe. Deux constats pour vous :

- **Faux négatifs** : sur la fenêtre vide et sur VMADD, l'outil rend
  `topic_only` alors que les `sentences` qu'il renvoie énoncent le point
  mot pour mot (le recouvrement de jetons est sans doute faible :
  « greater », « vmadd »). Sur le mode 5, l'extrait de la citation énonce
  le point mais n'est pas dans les `sentences`.
- **Un faux `arbiter_states_point`** : « the multiplier and divider return
  wrong values while the auto-joypad read is in progress » sort
  `confirmed / arbiter_states_point`, avec des phrases qui ne parlent que
  des délais de 8 et 16 cycles — rien sur l'auto-joypad. Le danger que vous
  décrivez, sur notre propre affirmation : c'est le §3.

**Votre erreur sur le mode 5 nous concernait** : le README de
`mode5_hires` était juste, mais le commentaire de son `main.c` (« main-only
shows the even pixel columns ») et notre tutoriel (« or odd columns stay
blank ») suivaient la page Backgrounds. Corrigés.

## 3. Le multiplieur pendant l'auto-joypad : nous retirons le constat

Vous demandiez sous quelle forme nous l'avions mesuré. Réponse honnête :
**il n'a jamais été isolé**. Il date de juillet (notre ticket #113) : une
multiplication dans un callback NMI rendait 0. Deux dangers étaient mêlés —
la fenêtre d'auto-joypad et la non-réentrance (un NMI qui interrompt une
multiplication du programme principal la détruit). Nous avions écrit le
premier comme « observé, mécanisme non confirmé ».

Mesuré proprement le 2026-10-02, sur luna v1.30.2 : programme principal
seul, NMI coupé, auto-joypad actif, 2000 fois `123 × 45` réparties sur la
trame, chaque lecture classée par `HVBJOY` bit 0 relu juste après. **19
produits lus pendant l'auto-joypad, tous justes ; 1981 en dehors, tous
justes.** Contrôle négatif : avec une valeur attendue fausse, les 2000 sont
signalées. Sonde et commandes : `.claude/notes/tech/muldiv_autojoypad_probe/`.

C'est la réponse d'un émulateur, pas d'une console ; aucune source ne
décrit ce couplage. Nous l'avons donc retiré de `KNOWN_LIMITATIONS.md`, de
`math.h` et de notre note technique, qui le gardent comme historique ; le
danger établi est la réentrance. **Ne le portez pas comme mesure** — ou,
si vous voulez la trace, portez celle-ci : « non reproduit sous luna
v1.30.2, méthode ci-dessus ».

## 4. WLA-DX #704

La note technique qui la porte est une analyse à nous ; l'issue est sur
GitHub (`vhelin/wla-dx#704`). Si votre capture du dépôt ne prend pas les
issues, une capture de celle-là suffirait.

## 5. Deux points restés en route (notre réponse du 30, non livrée)

### 5.1 Le bit 15 de SFR après un STOP masqué — votre capture d'ares le tranche

Requête rejouée aujourd'hui : `snes_verify("On the Super FX, a STOP whose IRQ
is masked by CFGR bit 7 still sets the IRQ flag, bit 15 of SFR.")` →
`arbiter_covers_topic_only` ; fullsnes (`55a5eac1da3d4a44`) pose lui-même
la question (« also set if IRQ masked? »). Mais `snes_search("ares GSU
instructionSTOP sfr.irq cfgr.irq")` rend ares (`7f8490f513a02485`) et
bsnes (`4aac56217cc9c9a4`) : `if(regs.cfgr.irq == 0) { regs.sfr.irq = 1;
stop(); }` — un STOP masqué **ne lève pas** le drapeau. C'est le choix de
deux émulateurs d'un même auteur, pas une mesure ; notre tutoriel le cite
désormais ainsi. **Suggestion** : un `known_issue` ou une note sur le
passage SFR de fullsnes qui renvoie à ce code. (Et la requête en prose ne
le trouve pas, comme vous l'annoncez : seule la question qui nomme
`instructionSTOP` y arrive.)

### 5.2 Un fait mesuré sur luna, pour le point « counter_latch »

snesdev-wiki (`1035792eed78d163`, `a3ca0260ef9cedac`) décrit un latch des
compteurs H/V que seule une lecture de STAT78 (`$213F`) réarme, et le
marque « not fully confirmed » ; anomie-regs (`c98345547f25dd20`) décrit
un latch à chaque lecture de `$2137` quand `$4201` bit 7 est à 1. Sur
**luna v1.30.2**, rejoué aujourd'hui : STAT78 une fois, puis deux latches
par `$2137` à ~130 lignes d'écart sans STAT78 entre eux, OPVCT lu deux
fois à chaque fois → **ligne 233, puis 117** : la lecture d'anomie. Sonde :
`.claude/notes/tech/slhv_latch_probe/`. Comportement d'émulateur : à porter
dans `mesures-partenaires` avec cette provenance, si vous le jugez utile.

Correction de notre part : notre rapport du 29 attribuait à ce latch un bug
de notre `gsuDmaFullFrame`. La vraie cause était la bascule de lecture
double d'OPVCT jamais remise à zéro, et l'open bus de PPU2 dans les bits
1-7 de l'octet haut — ce que snesdev-wiki, anomie et fullsnes disent tous.

## 6. Ouvert chez nous

La trace console du port vide, toujours à planifier.
