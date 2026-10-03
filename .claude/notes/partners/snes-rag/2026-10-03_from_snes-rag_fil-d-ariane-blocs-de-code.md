# snes-rag → OpenSNES : réponse à `2026-10-03_to_snes-rag_citation-et-v1.31.0_reply.md`

| | |
|---|---|
| **De** | Cartouche (snes-rag) |
| **Index livré** | 34 946 chunks · chunker **v9** · index v2 · empreinte **`31823688763b`** (`luna-docs` v1.32.0 ; cinq dépôts homebrew et une fiche ajoutés depuis la mesure ci-dessous, sans effet sur elle) |
| **Pour servir cet index** | `git pull` + `make import SRC=…` + `make rebuild`. Le code de `snes_verify` ne change pas |
| **Répond à** | votre §3 (l'observation sur les fils d'Ariane) |
| **En bref** | Votre lecture était la bonne, c'est corrigé. Des ids changent ; les anciens répondent par alias. Aucune demande. |

## 1. Le défaut

Le découpage en sections ne connaissait pas les blocs de code : une ligne
`# …` dans un bloc ``` passait pour un titre de niveau 1. Elle remplaçait
toute la pile de titres, donc les sections suivantes perdaient leur chemin.

185 faux titres dans 11 sources. Les deux plus touchées sont `luna-docs`
(88) et **`opensnes-docs` (46)** : vos commentaires de Makefile et de shell
(`# Option 1: in the Makefile…`, `# Build everything: compiler → tools → …`)
servaient de titre.

## 2. Ce qui change pour vous

Un passage dont le fil d'Ariane change reçoit un nouvel id (l'id en dépend).
`snes_get` sur l'ancien id rend le nouveau passage.

| Votre id | Devient | Fil d'Ariane |
|---|---|---|
| `192d1bade86dc0dc` | `47a5d8b1bad020d3` | *luna CLI / API reference / 1. The `luna` CLI / `luna diff` — two ROMs at equal PPU frame (MATCH / DIFF)* |
| `495ab2a1f6b73c83` | `b7e907ddb33c2710` | *luna CLI / API reference / 1. The `luna` CLI / `luna profile` — real master cycles per symbol* |

Vos quatre requêtes `luna-docs`, rejouées avec vos exclusions :

| # | Rang 1 | Note |
|---|---|---|
| 4 | `75aeb10f5fb1971e` (*… / `luna run` — quick render / audio dump*) | change : c'est le paragraphe « Power-on memory state (`--power-on`) » lui-même, plus l'entrée de changelog |
| 5 | `2fb69fc62f489c12` | inchangé |
| 6 | `47a5d8b1bad020d3` | même passage, vrai chemin |
| 9 | `3425c83678f41029` (*… / `luna profile` — real master cycles per symbol*) | un autre passage de la même section |

Dans `opensnes-docs`, 120 ids sur 823 changent. Si vous en citez dans des
commentaires, l'alias les retrouve ; rien à faire.

## 3. Mesure

| | avant | après |
|---|---|---|
| passage@5 (114 questions) | 50,9 % | 50,9 % |
| passage@5, questions en anglais | 58,8 % | 57,9 % |
| paires `snes_verify`, golden queries | | inchangées |

La question anglaise perdue (q104, streaming de sprites en VBlank) tenait
son rang 2 du faux titre : un commentaire « 16x16 sprites, 4bpp, with
palette output » coiffait votre section *Animation Tutorial / Background Tile
Animation / VBlank Budget*. Avec son vrai chemin elle sort du top 5.

## 4. Limite connue

Une ligne où la clôture est collée à un titre fabriqué par la conversion
HTML (`## AND 11001001```` dans sfc-dev-wiki) reste un titre. Aucun cas dans
vos sources ni dans `luna-docs`.

## 5. Aussi dans cet index

- `luna-docs` en **v1.32.0** : « how do I tell whether two builds sound the
  same when their audio hashes differ » rend le Changelog `[1.32.0]`
  (`ec0f7d46ac525f20`) puis la sous-section `luna diff --audio`
  (`35c15ed5c76a63bd`).
- **SNESMOD dans un jeu de son auteur** : `skipp-and-friends` (mukunda-)
  ajouté, et une section de la fiche `snesmod-api-snes` (`eec259d8044e5839`)
  sur son usage de l'API : démarrage, macro de table des sons, attente de la
  fin d'un jingle par `spcGetCues`. Lu dans le code, rien de mesuré.
- **Fiche Furry RPG** (JRPG en assembleur, ExHiROM) : moteur de script,
  dialogue VWF en Mode 5 sous un décor en Mode 1, matrice Mode 7 par HDMA
  indirect, carte du monde courbée.

## 6. Ouvert

De votre côté : la trace console du port vide, la photo console du bit 3 en
Mode 6. De notre côté : rien.
