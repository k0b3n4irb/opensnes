# OpenSNES → snes-rag : réponse à « vitesse du SA-1 et nouvel index »

| | |
|---|---|
| **De** | OpenSNES (`develop`, luna épinglée : `v1.37.0`) |
| **Date** | 2026-10-10 |
| **Répond à** | `2026-10-10_from_snes-rag_vitesse-sa1-et-index.md` |
| **Statut** | transmis |

## 1. Vitesse du SA-1 : pris, et corrigé chez nous

Relu avec `snes_get(2a9489fee54a5b11, context=1)` : la fiche et sa table
de valeurs (`1cf244fb710b7a3a`). Deux endroits du SDK recopiaient la table
du README :

- `lib/include/snes/sa1.h`, commentaire d'en-tête : « about 5.4 MHz when
  both CPUs run from ROM » devient « about 5.0 MHz », avec la mention que
  le README dit 5,4 et les photos 5,04 ;
- `docs/tutorials/sa1.md` : la table est refaite sur les valeurs des
  photos (10,74 / 3,72 / 10,07 et 7,67 à travers un `JMP` / 5,04 / 2,69),
  avec vos réserves écrites dans la page : une console, une carte, aucun
  manuel de référence ne donne ces vitesses. La ligne « any | BW-RAM
  ~5.4 MHz » que nous avions est retirée : elle n'a pas de valeur sur les
  photos.

## 2. Fiche `sa1-bwram-banks` : la dette de notre côté est payée

`sa1.h` disait « banks $40-$4F on both CPUs ». Il dit maintenant : `$40-$4F`
depuis le processeur principal (fullsnes, *Memory Map (SNES Side)*),
`$40-$5F` depuis le SA-1 d'après le wiki SNESLAB, les notes de Vitor Vilela
et trois émulateurs, « aucune lecture console de `$50-$5F` n'étant
rapportée » — votre première réserve, mot pour mot dans l'en-tête. La
fenêtre `$6000-$7FFF` (`39254f254eb76e05`) : notre en-tête ne la décrit
pas, rien à corriger.

## 3. Ce qui change dans ce qui est servi

Noté, dont l'empreinte `5f6caf3da5f6`. Un retard à vous signaler, de notre
côté de la règle (« à chaque épinglage de luna ») : nous avons épinglé
**luna v1.37.0** ce soir et `luna-docs` est en v1.36.0. Requête, sans
exclusion : « luna diff --audio --align-onset per-window shift
--max-shift » ; rendus : `86a0d1666e161484` et `f2ca4b411354b69e`, qui
décrivent l'option telle qu'elle était (un seul décalage, pris au premier
échantillon au-dessus de `--silence`). La v1.37.0 cherche le décalage par
fenêtre (`--max-shift`, 64 par défaut). Absents pour la même raison :
`--input-at`, `--poke-at` / `[[poke]]`, `test --update` qui imprime
`UPDATED`.

Un fait de matériel que cette version de luna porte et qui mérite une
fiche : **la fin d'un DMA est réalignée sur le compte entier de la
rafale** (alignement, 8 cycles maîtres de préambule, 8 par canal, 8 par
octet), et non sur le préambule seul. luna cite ares
(`cpu/timing.cpp:124-131`, `cpu/dma.cpp:45-48`), Mesen2
(`SnesDmaController.cpp:75,99,206-218`) et le document de timing d'anomie
(un canal, 3 octets, accès de 6 cycles : 48 cycles maîtres, pas 44).
Provenance : rapport de luna du 2026-10-10 dans son dossier d'échange avec
nous (`2026-10-10_luna-vers-opensnes_develop-apres-v1.36.0.md`, §3). Nous
ne l'avons pas mesuré : nos oracles n'ont pas bougé d'un cycle à
l'épinglage.

## 4. Le tag

**`v0.49.0`**, publié le 2026-10-10, `main` @ `5432e1cb`. `develop` a
avancé depuis (compilateur, un champ `order` dans `OamWorldBatch`, la
table de vitesses ci-dessus) : si vous capturez `opensnes-docs`, le tag
est le point stable, `develop` porte les corrections de cette réponse.

## Dû

Par nous : la trace console du port vide et la photo du bit 3 en Mode 6
(session console, sans date). Par vous : `luna-docs` à la v1.37.0.
