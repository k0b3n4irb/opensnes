# luna → OpenSNES : ce qui est sur `develop` après la v1.36.0, et un correctif de durée de DMA en attente

| | |
|---|---|
| **De** | luna (`develop` @ `260397f` ; dernière version publiée : `v1.36.0` @ `86f9eba`) |
| **Date** | 2026-10-10 |
| **Répond à** | `2026-10-10_opensnes-vers-luna_compte-rendu-epinglage-v1.36.0.md`, et votre demande du jour sur `diff --audio --align-onset` (dossier `2026-10-10_roms_audio-onsets/`) |
| **Statut** | transmis |
| **En bref** | Rien de ceci n'est dans une version publiée : il n'y a pas de date pour la 1.37.0. Quatre changements sont sur `develop` ; un seul change un verdict chez vous (`--align-onset`, à votre demande). Un cinquième, **prêt mais pas encore commité**, déplacera vos références en cycles autour d'un DMA : il est décrit au §3 pour que vous le releviez en une fois le jour de l'épinglage. |

## 1. Sur `develop`, à votre demande : `diff --audio --align-onset` par fenêtre (`260397f`)

L'option coupait les deux captures à leur premier échantillon au-dessus du
niveau de silence : un seul décalage pour tout, pris sur le premier son.
Votre paire `echo` (deuxième son décalé de 2 échantillons, bord de fenêtre
dans son attaque) sortait DIFF à 21,83 %.

Désormais chaque fenêtre de A est comparée au tronçon de B qui lui
ressemble le plus (somme des écarts absolus, échantillon contre
échantillon) dans ±`--max-shift` échantillons.

- `--align-onset` garde son nom ; `--max-shift N` est nouveau, 64 par
  défaut, plus court qu'une fenêtre.
- Chaque ligne de fenêtre finit par `shift=+N` (b − a : positif quand le
  son de B vient plus tard).
- La ligne finale dit la méthode, comme vous l'avez demandé :
  `9 window(s) of 500 ms, max delta 0.15% (tolerance 2%), per-window shift, max 2 samples (searched ±64): MATCH`.
- Fenêtre silencieuse ou égalité entre plusieurs décalages : le plus
  proche de zéro est gardé, et imprimé.
- Avant l'échantillon 0 la machine ne tournait pas : c'est lu comme du
  silence. Au-delà de la fin de B, rien n'est essayé.
- Un son parti une image plus tôt (534 échantillons) reste un DIFF à 64 et
  sort avec `--max-shift 534`.
- Rapport JSON : `onset_shift` disparaît ; `shift_limit`, `largest_shift`
  et un `shift` par fenêtre le remplacent.

Rejoué chez nous sur `echo` : décalages +2, 0, 0, −2, −2, 0, 0, 0, 0,
MATCH. Rejoué chez vous (votre message) : `echo` identique, et
`speech_synth` à −12 puis −16 sur huit fenêtres, écart maximal 0,77 %,
MATCH. Nous n'avons pas rejoué `speech_synth` nous-mêmes.

Votre règle `testing.md` reste vraie sans retouche ; une sortie citée
d'avant ce commit porte « onset shift », une d'après « per-window shift ».

## 2. Sur `develop`, sans effet sur vos verdicts

- **Largeur d'une assertion de valeur** (`14a6bb8`, déjà annoncée) : une
  variable que le `.sym` donne sur deux octets est comparée en entier. Vos
  133 manifestes rendent le même verdict avant et après.
- **`--input-at SYMBOLE`** (`60b05e9`), dans `luna state`, `luna profile`
  et un manifeste (`input_at`) : l'entrée `N:` d'un script d'entrées
  s'applique au N-ième passage sur la routine, au lieu de la trame N. Un
  script ne glisse plus d'un tour de boucle quand le code accélère.
- **`--poke-at SYMBOLE --poke-hit N --poke NOM=HEX`, et `[[poke]]` dans un
  manifeste** (`4a1c9a7`) : écrire en mémoire au N-ième passage sur une
  routine, pour placer un programme dans un état qu'il n'atteint pas seul
  en test, sans code de test dans la ROM. Un passage jamais atteint est
  une erreur.
- **Ménage** (`6ac602f`) : la version que votre `main` épingle garde ses
  binaires quel que soit son rang. La release `v1.32.0` a été supprimée
  après votre fusion 0.49.0 (son tag reste).

## 3. Prêt, pas encore commité : la durée d'un DMA

Il attend une validation à l'œil avant d'entrer sur `develop` ; nous vous
annoncerons le commit. Décrit ici parce qu'il **bougera vos références en
cycles** (lignes `dma` et `vramq` de votre banc, budget NMI, et l'oracle
audio par ricochet).

Le fait de matériel (ares `cpu/timing.cpp:124-131` et `cpu/dma.cpp:45-48`,
Mesen2 `SnesDmaController.cpp:75,99,206-218`, document de timing
d'anomie) : après le dernier octet, le processeur attend de retomber sur un
nombre entier de ses propres cycles **depuis la pause**. Ce compte porte
sur toute la rafale : alignement, 8 cycles maîtres de préambule, 8 par
canal, 8 par octet.

Ce que luna faisait :

- le réalignement final était calculé sur le préambule seul, sur tous les
  chemins. Une rafale lancée depuis un accès de 6 cycles finissait 2 ou 4
  cycles maîtres à côté quand `canaux + octets` n'est pas multiple de 3.
  Depuis un accès de 8 cycles, rien ne change. L'exemple d'anomie (un
  canal, 3 octets, accès de 6) rendait 44 cycles au lieu de 48 ;
- un DMA lancé pendant qu'un canal HDMA est activé était facturé à plat :
  ni alignement, ni 8 cycles par canal, ni réalignement.

Phrase pour votre commit de relevé : « luna réaligne maintenant la fin
d'un DMA sur le compte entier de la rafale, comme ares, Mesen2 et le
document d'anomie ; la durée d'un DMA lancé depuis un accès de 6 cycles
bouge de 2 ou 4 cycles maîtres. »

Mesuré chez nous avec ce correctif : toute notre suite passe (1161 tests),
18 captures de jeux commerciaux à HDMA identiques à l'octet, et une seule
image de référence bouge, sur un jeu Super FX : les mêmes images dans le
même ordre, chacune jusqu'à trois trames plus tôt ou plus tard. Vos ROM
Super FX peuvent donc bouger de la même façon : c'est le cas que
`luna diff --sequence` tranche.

## 4. Vos deux observations du compte rendu d'épinglage

- `--sequence` rend DIFF sur un écran presque fixe dont seule l'image de
  transition diffère : conforme à ce que fait l'option, rien de prévu.
- `luna test --update` imprime encore la ligne d'échec et « 0 passed,
  1 failed » après avoir corrigé le manifeste : c'est un défaut chez nous,
  il sera corrigé. Pas encore commencé.

## 5. Ce que nous attendons de vous, sans date

- Les deux ROM de reproduction : MS0 avec Super FX à 21 MHz, et LoROM
  DSP-1 de 2 Mo.
- Vos trois manifestes convertis à `at_symbol`, quand vous y serez.
