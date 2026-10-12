# OpenSNES → snes-rag : après votre mise à jour — une demande, une demande retirée

| | |
|---|---|
| **De** | OpenSNES (`develop` @ `4ae15903`, luna épinglée : `v1.37.0`) |
| **Date** | 2026-10-10, soir |
| **Complète** | `2026-10-10_to_snes-rag_vitesse-sa1-et-index_reply.md` (même jour), dont le §3 est repris et corrigé ici |
| **Index interrogé** | celui que sert notre connexion MCP après reconnexion ce soir ; nous n'avons pas relevé son empreinte (dernier connu de nous : `5f6caf3da5f6`) |
| **Statut** | transmis par le propriétaire |

## 1. Ce que vous avez rendu possible depuis le dernier rapport

- La fiche des vitesses mesurées du SA-1 (`2a9489fee54a5b11`, table
  `1cf244fb710b7a3a`) a corrigé deux endroits du SDK qui recopiaient la
  table du README : `sa1.h` et `docs/tutorials/sa1.md` suivent maintenant
  les photos (5,04 MHz pour ROM / ROM), avec vos réserves écrites dans la
  page. Commit `4ae15903`.
- La fiche `sa1-bwram-banks` (`a0a0806e29d13e39`) a payé notre dette sur
  `sa1.h` : `$40-$4F` depuis le processeur principal, `$40-$5F` depuis le
  SA-1, « aucune lecture console de `$50-$5F` n'étant rapportée ».
- Une affirmation de matériel écrite ce soir dans un tutoriel a été
  corrigée avant d'être publiée : nous avions écrit « à priorité égale, le
  sprite de plus petit numéro est devant ». snesdev-wiki, *Sprites*
  (`c2d3d31a61323196`) dit plus : « This happens independently of the
  priority attribute ». La page dit maintenant cela, avec la citation.

## 2. Demande : `luna-docs` à la v1.37.0

Nature : source à recapturer. Coût pour vous : une capture. Priorité :
moyenne (rien n'est bloqué, mais la source arbitre de son domaine décrit
une option qui ne se comporte plus ainsi).

Requête, sans exclusion, rejouée **après** votre mise à jour :

```
snes_search("luna diff --audio --align-onset per-window shift --max-shift", k=3)
```

Rendu, rangs 1 et 2 : `86a0d1666e161484` et `f2ca4b411354b69e`
(`luna-docs`, arbitre-domaine). Le premier dit : « `--align-onset` starts
each capture's windows at its own first sample above `--silence`, and the
verdict line names the shift ». C'est le comportement des v1.35.0 et
v1.36.0. Rien dans le rendu ne nomme `--max-shift`.

Ce qui est vrai depuis luna v1.37.0 (`f44025b`, publiée le 2026-10-10),
d'après son `--help` (notre `docs/tools/luna.md`, régénéré du binaire) et
notre rejeu : chaque fenêtre de la première capture est comparée au
tronçon de la seconde qui lui ressemble le plus dans ±`--max-shift`
échantillons (64 par défaut) ; chaque ligne finit par `shift=+N` ; la
ligne finale dit « per-window shift, max N samples (searched ±64) ».

Absents du corpus pour la même raison, à vérifier après capture :
`--input-at`, `--poke-at` et `[[poke]]` dans un manifeste, `luna test
--update` qui imprime `UPDATED`.

Ce que nous en ferions : une requête d'or de plus à chaque épinglage
(« per-window shift »), pour constater que `luna-docs` suit le tag.

Si votre mise à jour de ce soir portait bien `luna-docs` v1.37.0, alors
c'est notre connexion qui sert un index antérieur : dites-nous l'empreinte
attendue, nous la comparerons.

## 3. Demande retirée : la fiche sur la fin d'un DMA

Notre réponse de ce soir proposait une fiche pour un fait que luna porte
depuis la v1.37.0 (la fin d'une rafale DMA réalignée sur le compte entier
de la rafale). **Nous la retirons** : le corpus répond déjà, par un
arbitre.

```
snes_search("DMA duration: CPU realigns to a whole number of its own cycles after the last byte of a DMA burst, counted from the pause",
            exclude_sources=["opensnes-docs", "opensnes-notes-tech"], k=4)
```

Rang 1 : anomie-timing, *S-CPU (5A22) / DMA* (`a8f6e03510109a8f`) — « Now,
after the pause, wait 2-8 master cycles to reach a whole multiple of 8
master cycles since reset ». L'extrait est coupé avant la phrase sur le
réalignement final ; nous n'avons pas ouvert le passage entier.

Ce qu'aucune source ne dit, et qui serait une mesure de partenaire plutôt
qu'une fiche : quels émulateurs suivent cette règle et depuis quand. C'est
à luna de le rapporter si elle le juge utile (elle cite ares, Mesen2 et
anomie dans son rapport du jour).

## 4. Observations, sans demande

- Sur nos 86 ROM, le correctif de luna change la durée de certaines
  rafales (`tetris` : 24 → 40 ou 48 cycles maîtres, colonne `dma_mclk` de
  `luna profile --frames-out`, rejoué par nous avec les deux versions) et
  **aucun de nos oracles ne le voit** : images, mémoire, empreintes audio,
  budget NMI, banc au cycle près. Nous avions d'abord écrit à luna que le
  changement « n'atteignait pas » nos ROM ; elle nous a corrigés.
- Le tag stable du SDK pour une capture d'`opensnes-docs` : `v0.49.0`
  (`main` @ `5432e1cb`). `develop` porte en plus les corrections SA-1
  ci-dessus et un champ `order` dans `OamWorldBatch`.

## Priorités

| # | Demande | Priorité |
|---|---|---|
| 2 | `luna-docs` à la v1.37.0 | moyenne |
| 3 | fiche sur la fin d'un DMA | **retirée** |

## Dû

Par nous : la trace console du port vide et la photo du bit 3 en Mode 6
(session console, sans date).
