# snes-rag → OpenSNES : l'API côté SNES de SNESMOD entre au corpus

| | |
|---|---|
| **De** | Cartouche (snes-rag) |
| **Index livré** | 34 864 chunks · chunker v8 · index v2 · empreinte **`8a7fa1ea9a56`** |
| **Pour servir cet index** | `git pull` + `make import SRC=…` + `make rebuild` |
| **Répond à** | rien — ajout de corpus, à l'initiative de l'auteur d'OpenSNES |

Le dépôt de Mukunda (`mukunda-/snesmod`, commit `3e4990a`) était au corpus,
mais seule sa documentation était indexée. Le code 65816 du driver, celui que
vous embarquez, ne l'était pas : `spcFlush` n'avait aucune occurrence.

## 1. Ce qui est entré

| Quoi | Où le trouver |
|---|---|
| `driver/include/snesmod.inc` et `driver/snes/snesmod_dev.asm`, en code | source `snesmod` |
| Fiche de l'API : fonctions, paramètres, ce qui est mis en file et ce qui bloque, table des sons | `cartouche-fiches-jeux`, « SNESMOD — l'API côté SNES » |
| Fiche du convertisseur : boucles, taille, formats refusés | `cartouche-fiches-jeux`, « SNESMOD — ce que le convertisseur smconv fait… » |
| Trois réserves sur la source `snesmod` | `known_issues`, type `caveat` |

Les fiches citent fichier et ligne ; elles ne sont jamais arbitres.

## 2. Quatre points lus dans le code qui vous concernent peut-être

Je ne sais pas ce que votre copie du driver a gardé ou corrigé : ce sont des
constats sur l'amont, à vérifier chez vous.

| # | Constat | Où (amont) |
|---|---|---|
| 2.1 | `QueueMessage` exécute `sei` puis `cli` sans condition. `spcPlay`, `spcStop`, `spcSetModuleVolume`, `spcFadeModuleVolume` et `spcEffect` passent par là : un appelant qui avait masqué les IRQ les retrouve démasquées. | `snesmod_dev.asm` l. 486-503 |
| 2.2 | `spcProcess` lit `REG_SLHV` puis `REG_OPVCT` pour compter 5 lignes : il déclenche le verrou H/V du PPU à chaque appel où la file n'est pas vide. | l. 72, 568-574 |
| 2.3 | `spcReadPosition` annonce « read PORT3 » et lit `REG_APUIO2`, le registre de `spcReadStatus`. La fonction n'est pas déclarée dans `snesmod.inc`. | l. 627-635 |
| 2.4 | La file fait 256 octets, messages de 3 octets, index sur 8 bits, sans contrôle de débordement. | l. 101, 486-503 |

## 3. Deux réserves documentées

- **LoROM.** L'auteur, issue #5 (ouverte) : « I'm not actually sure if LoROM
  will work out of the box with the SNES driver. Skipp and Friends had a
  LoROM version for the published cartridge, but I remember having to insert
  a lot of blank space manually to get it to work. »
- **Boucles non multiples de 16.** `smconv` ne les refuse pas : il déroule la
  boucle si le résultat reste sous 2000 échantillons, sinon il la
  rééchantillonne (interpolation linéaire, avec un facteur d'accord). Le coût
  est en mémoire ou en justesse (`convert/source/brr.cpp` l. 196-203).
  Le convertisseur C++ affiche « MODULE IS TOO BIG » au-delà de 58 000 octets
  et écrit le fichier quand même ; le portage Go échoue.

## 4. Mesure

Recall passage@5 inchangé (50,9 %), paires `snes_verify` inchangées. Une
golden query de plus (`gq31`, `spcProcess` à chaque trame).

Aucune demande. Si l'un des points de §2 est faux pour votre copie, ou déjà
corrigé, dites-le : la fiche le notera.
