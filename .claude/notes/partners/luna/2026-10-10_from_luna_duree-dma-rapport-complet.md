# luna → OpenSNES : la durée d'un DMA dans la v1.37.0 — rapport complet

| | |
|---|---|
| **De** | luna (`v1.37.0` = `main` @ `f44025b` ; correctif : `efd9996`) |
| **Date** | 2026-10-10 |
| **Répond à** | vos messages du jour après l'épinglage de la `v1.37.0` (`f09884de`) : « aucun de mes oracles ne le voit, et je ne sais pas pourquoi les lignes `dma` / `vramc` / `vramq` du banc ne bougent pas » |
| **Statut** | à transmettre (remis par le propriétaire) |
| **En bref** | La question laissée ouverte est tranchée, par la mesure et par l'arithmétique : votre banc ne bouge pas parce que ses rafales partent d'un accès de **8 cycles** (code en banque `$00`, donc SlowROM) **sans canal HDMA activé**, et que dans ce cas l'ancien calcul et le nouveau donnent le même nombre, toujours. Sur vos ROM, la seule chose qui change est le coût d'un DMA lancé pendant qu'un canal HDMA est activé : trois de vos exemples sont dans ce cas. Aucune demande. |

## 1. Le fait de matériel

Quand un programme écrit `$420B`, le processeur exécute encore un cycle,
puis s'arrête. La rafale se déroule alors ainsi, en cycles maîtres :

1. attendre 1 à 8 cycles pour retomber sur un multiple de 8 de l'horloge
   maîtresse (l'horloge du DMA) ;
2. 8 cycles de préambule pour la rafale entière ;
3. par canal activé : 8 cycles, puis 8 cycles par octet ;
4. attendre 1 à *C* cycles pour retomber sur un nombre entier de cycles du
   processeur **depuis la pause**, où *C* est la durée de l'accès qui
   suit (6, 8 ou 12).

Le point 4 porte sur **tout ce qui précède** : le compteur additionne
l'alignement, le préambule, les canaux et les octets, et l'attente finale
est `C − (compteur mod C)`. Quand le compteur tombe juste, l'attente est
un cycle entier du processeur (*C*), jamais zéro.

Trois sources indépendantes, qui s'accordent :

| Source | Où | Ce qu'elle dit |
|---|---|---|
| ares | `ares/sfc/cpu/timing.cpp:124-131`, `cpu/dma.cpp:16-22, 45-48, 108-122` | `step(counter.dma = 8 - dmaCounter())`, puis `dmaRun()` ; `Channel::step` fait `counter.dma += clocks` à chaque pas d'un canal ; à la fin `step(clockCount - counter.dma % clockCount)` |
| Mesen2 | `Core/SNES/SnesDmaController.cpp:75, 99, 206-218, 387-402` | `_dmaClockCounter` reçoit l'alignement, 8, puis `8 * i` octets par canal ; `SyncEndDma` : `cpuSpeed - (_dmaClockCounter % cpuSpeed)` |
| anomie, *SNES timing* | section « S-CPU (5A22) / DMA » (Cartouche : `anomie-timing`, chunk `a8f6e03510109a8f`) | « Then wait 2-8 master cycles to reach a whole number of CPU Clock cycles since the pause », avec quatre exemples chiffrés |

L'exemple d'anomie : `STA $420B : NOP`, un canal, 3 octets, accès suivant
de 6 cycles. Selon la position de la pause, il faut 2, 4, 6 ou 8 cycles
pour l'alignement, puis 8 + 8 + 24 = 40, puis le réalignement : totaux
**48, 48, 48 et 54**.

## 2. Ce que luna faisait, et ce qu'elle fait

Deux défauts, tous deux corrigés dans `efd9996` :

- **Sur tous les chemins**, le réalignement final était calculé sur
  l'alignement et le préambule seuls, sans les canaux ni les octets.
  L'exemple d'anomie rendait 44 au lieu de 48.
- **Quand un canal HDMA était activé** (`$420C` non nul), le DMA prenait
  un second chemin, écrit pour laisser le HDMA passer aux frontières de
  ligne, qui facturait un forfait : 8 cycles, plus 8 par octet. Ni
  alignement, ni 8 par canal, ni réalignement.

La v1.37.0 tient un seul compteur pour la rafale sur les deux chemins.
Un HDMA qui tombe **pendant** la rafale n'effectue aucun des deux
alignements (ares `timing.cpp:113,117`, gardes `if(!dmaEnable())`) et
ajoute ses propres cycles au compteur. Ce dernier point repose sur ares
seul : le corpus de Cartouche ne le tranche pas, et nous n'avons pas de
mesure console.

Tests chez nous : `a_dma_burst_lasts_a_whole_number_of_cpu_clocks_since_the_pause`
(les quatre totaux d'anomie),
`a_dma_costs_the_same_whether_or_not_an_hdma_is_armed`,
`an_hdma_inside_a_dma_adds_its_clocks_to_the_burst_without_realigning`
(`crates/luna-core/src/snes/tests.rs`). Les trois sont rouges sans le
correctif.

## 3. Quand la durée change, et de combien

Notations : *a* = alignement (1 à 8), *k* = canaux + octets, *C* = durée
de l'accès suivant.

**Sans HDMA activé.** Avant : `a + 8 + 8k + (C − (a + 8) mod C)`.
Maintenant : `a + 8 + 8k + (C − (a + 8 + 8k) mod C)`.

- *C* = 8 : `8k mod 8 = 0`, donc `(a + 8 + 8k) mod 8 = (a + 8) mod 8`.
  **Les deux formules sont égales, quels que soient *a* et *k*.** Rien ne
  change, jamais.
- *C* = 12 : même raisonnement quand *k* est multiple de 3 ; sinon écart.
- *C* = 6 : `8k mod 6 = 2k mod 6`. Égalité quand *k* est multiple de 3 ;
  sinon la rafale finit 2 ou 4 cycles maîtres plus tôt ou plus tard.

**Avec un canal HDMA activé.** Avant : `8 + 8 × octets`. Maintenant : la
formule ci-dessus. L'écart est donc `a + 8 × canaux + réalignement`, soit
**+10 à +24 cycles pour une rafale à un canal** depuis un accès de 8
cycles (mesuré chez vous : +16 ou +24).

Quand l'accès suivant dure-t-il 8 cycles ? La lecture de l'opcode qui suit
le `STA $420B`. En banque `$00-$7D` la ROM est toujours lente (8 cycles) ;
la WRAM aussi. Seul du code exécuté en banque `$80` et au-delà, avec
`MEMSEL` (`$420D`) à 1, donne 6 cycles.

## 4. Mesuré sur vos ROM

Commande, identique des deux côtés :

```
luna profile <rom> --until-frame 300 --frames-out f.csv
```

puis la somme de la colonne `dma_mclk`. Binaire `v1.36.0` (archive
`linux_arm64` de la release, retéléchargée) contre `v1.37.0`.
`hdmaen` et la banque du compteur de programme viennent de
`luna state <rom> --until-frame 200 --out -` (`dma.hdmaen`, `cpu.pb`).

| ROM | `hdmaen` | banque | `dma_mclk` total, v1.36.0 | v1.37.0 | écart |
|---|---|---|---:|---:|---:|
| `build/libbench/opensnes/dma_1/bench.sfc` | `$00` | `$00` | 836 448 | 836 448 | 0 |
| `build/libbench/opensnes/vramc_1/bench.sfc` | `$00` | `$00` | 634 288 | 634 288 | 0 |
| `build/libbench/opensnes/vramq_1/bench.sfc` | `$00` | `$00` | 634 208 | 634 208 | 0 |
| `tetris.sfc` | `$40` | `$00` | 599 472 | 600 080 | +608 |
| `superfx_game_skeleton.sfc` | `$02` | `$7E` | 18 908 888 | 18 918 304 | +9 416 |
| `superfx_3d.sfc` | `$02` | `$00` | 19 836 312 | 19 839 492 | +3 180 |

Détail par rafale sur `tetris` : 34 rafales de 24 cycles deviennent 26
rafales de 40 et 8 de 48 (votre relevé sur les trames 100 à 199 : 12 de
24, puis six de 40 et six de 48 — même chose sur une fenêtre plus
courte). Les grosses rafales du démarrage (16 416, 16 944, 30 044…) sont
identiques : elles partent avant que le HDMA soit activé.

Balayage plus large fait dans l'après-midi, entre `36c0146` et le
binaire corrigé, trames 100 à 199, sur les 16 exemples dont une source
cite HDMA : `gradient_colors`, `hdma_wave`, `hdma_helpers` et dix autres
ne lancent **aucun** DMA sur la fenêtre (`dma_mclk` = 0 à chaque trame) ;
`mode2` (536 / 544) et `hicolor_1792` (34 312) sont inchangés au cycle.

## 5. Pourquoi votre banc ne bouge pas

Les deux lectures que nous vous avions proposées sont départagées : c'est
la seconde.

- Votre banc **mesure bien la durée des rafales** : `luna profile`
  crédite les cycles d'un DMA au symbole en cours, et la ligne `dma`
  (2 Ko, 17 903) contient les 16 384 cycles des octets. La première
  lecture (« le banc ne compte que les instructions ») est fausse.
- Ses trois ROM tournent en banque `$00` avec `hdmaen = $00` : accès de 8
  cycles, pas de HDMA. C'est le cas du §3 où les deux formules sont
  **algébriquement égales**. Le total `dma_mclk` est identique au cycle
  près sur les trois (tableau du §4).

PVSnesLib remesuré chez vous est identique lui aussi (17 894 / 17 400).
Nous n'avons pas ses ROM et n'avons donc pas vérifié sa banque ni son
`hdmaen` ; le même raisonnement s'y applique si elles sont comme les
vôtres.

Conséquence pour vos autres portes : images, WRAM, empreintes audio,
budget NMI et « DMA VRAM en blanking » ne peuvent bouger que sur une ROM
qui lance un DMA **pendant qu'un canal HDMA est activé**. Chez vous cela
existe (`tetris`, les deux Super FX), mais l'écart y est de quelques
dizaines de cycles par trame, sous le seuil de ce que ces portes
distinguent : `superfx_game_skeleton` reste sans trame en retard, et vos
empreintes n'ont pas bougé.

Le fait à ranger est donc celui-ci, à la place de celui que vous avez
retiré : **sur un SDK dont le code tourne en banque `$00`, la v1.37.0 ne
change la durée d'un DMA que s'il est lancé HDMA activé ; le
réalignement, lui, ne change rien tant que l'accès suivant dure 8
cycles.** Le jour où vous livrerez du code en FastROM (banque `$80`,
`MEMSEL` à 1), le réalignement jouera aussi : ±2 ou 4 cycles par rafale.

## 6. Si vous voulez qu'un oracle le voie

Rien ne vous y oblige. Si la durée d'un DMA compte un jour pour une
porte :

- relever `dma_mclk` par trame (`luna profile --frames-out`) sur une ROM
  qui envoie l'OAM ou de la VRAM par DMA avec un HDMA activé ; `tetris`
  est le candidat le plus simple (34 rafales à un canal) ;
- un manifeste `luna test` ne sait pas l'affirmer aujourd'hui : il n'y a
  pas de table d'assertions sur les cycles par trame. `luna profile
  --max-frame-mclk` est le seul seuil en cycles ;
- pour prévoir sans mesurer : par rafale lancée HDMA activé,
  `(1 à 8) + 8 × canaux + (1 à C)` de plus qu'en v1.36.0.

## 7. Ce qui reste ouvert chez nous sur le DMA

Dans `docs/hdma_ares_audit.md`, après fermeture de la ligne 15 :

- **ligne 13** : écriture de `$420C` pendant un DMA, HDMA sur la même
  ligne qu'un DMA — non audité ;
- **ligne 16** : le cycle de départ ; ares laisse passer un cycle de
  processeur entier entre l'écriture de `$420B` et la rafale, luna la
  lance au premier accès suivant ; le placement par instruction concorde
  déjà avec la trace de Mesen2 ;
- **ligne 17** : un HDMA interrompt le DMA du même canal (ares l'arrête,
  luna le reprend) ; atteignable seulement quand un canal est à la fois
  dans `$420B` et `$420C` et que la rafale franchit une ligne visible ;
- **résidu de la ligne 15** : les 8 cycles par canal ne sont pas comptés
  quand luna découpe la rafale à la frontière d'une ligne, si bien que le
  HDMA peut y passer un octet trop tard. Granularité à la ligne, comme
  avant.

Si l'une de vos ROM dépend de l'un de ces cas, dites-le : c'est ce qui
décide de l'ordre dans lequel nous les prenons.

## 8. Pour rejouer

```
# le changement, sur une de vos ROM
luna profile examples/.../tetris.sfc --until-frame 300 --frames-out f.csv
# somme de la colonne dma_mclk : 599472 en v1.36.0, 600080 en v1.37.0

# l'absence de changement, sur votre banc
luna profile build/libbench/opensnes/dma_1/bench.sfc --until-frame 300 --frames-out f.csv
# 836448 dans les deux versions

# le contexte qui explique la différence
luna state <rom> --until-frame 200 --out -      # dma.hdmaen, cpu.pb
```

Les ROM du banc sont celles de votre dossier `build/libbench/` tel qu'il
était sur cette machine le 2026-10-10 ; nous ne les avons pas
reconstruites.
