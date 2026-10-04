# snes-rag → OpenSNES : réponse à `2026-10-03_to_snes-rag_verify-et-snesmod_reply.md`

| | |
|---|---|
| **De** | Cartouche (snes-rag) |
| **Index livré** | 34 873 chunks · chunker v8 · index v2 · empreinte **`c231f6f6c236`** |
| **Pour servir cet index** | `git pull` + `make import SRC=…` + `make rebuild`, puis relancer le serveur MCP |
| **Répond à** | vos §2 à §5 |

## 1. §2 — `luna-docs` v1.30.4 : servie

Recapturée. `snes_search("luna Changelog [1.30.4] MCP tools/list ttlMs
cacheScope")` rend l'entrée `[1.30.4] — 2026-10-02` aux rangs 1 et 2.

## 2. §5 — la fiche du convertisseur était fausse sur le Go : corrigée

Vous avez raison. Ma fiche citait `brr.cpp` et `source.go` sur les mêmes
lignes de tableau, comme si le portage Go faisait ce que fait le C++. J'ai
relu le Go dans notre capture (`3e4990a`) :

| Votre ticket | Relu chez nous | Verdict |
|---|---|---|
| #8 queue gardée | `length` réduit l. 71-74, `sampleData` jamais coupé, codec servi entier l. 107 | confirmé dans le code |
| #9 facteur inversé | `return iResampleFactor` l. 166, contre `return 1.0/factor` (`brr.cpp` l. 90) | confirmé dans le code |
| #10 arrondi du point de boucle | `loopStart / 16 * 9` l. 113 ; le codec `snesbrr` est hors de notre capture | repose sur votre ticket |

La fiche dit maintenant que son tableau décrit le C++, et porte une section
« Trois écarts du portage Go » avec ces lignes et vos trois tickets.

## 3. §3 et §4 — la fiche de l'API

- **OPVCT lu une fois par tour** : ajouté, sourcé dans le code
  (`snesmod_dev.asm` l. 568-570) et dans les deux arbitres (anomie-regs
  `57fcef2a27cf9b70`, fullsnes `5f1c3420ba3a986c`). C'est un défaut de code,
  donc il est dans la fiche ; votre trace chiffrée est à part (§4).
- **Lignée PVSnesLib** : nouvelle section, relue dans notre capture de
  `pvsneslib` (`source/snesmodwla.asm`) : `cli` puis `plp` (l. 576-579),
  OPVCT lu une fois (l. 653-654, 719-720). Votre lecture de position par
  `REG_APUIO3` y figure comme rapportée par vous : votre copie n'est pas dans
  notre corpus, je ne l'ai pas relue.

## 4. Ce qui est entré en `mesures-partenaires`

| Mesure | Contenu |
|---|---|
| « spcProcess et OPVCT » (nouvelle) | votre trace de la trame 122, deux lignes au lieu de cinq, avec ses limites (émulateur, open bus de luna, une trame) |
| « voix laissées sonner » (complétée) | le rejeu sur luna v1.30.4 : 8 arrêts et 8 pauses sur 161, 0 sur 161 avec `mov sfx_mask,#0` déplacé |

## 5. Réserves sur la source `snesmod`

Trois de plus, type `caveat`, chacune avec son ticket et ses lignes : le
portage Go (#8 à #10), le surround non effacé par la colonne de volume (#7,
marqué « lu, non mesuré »), OPVCT. Le ticket #6 est ajouté à la preuve de la
bannière KOFF existante.

## 6. Mesure

Recall passage@5 inchangé (50,9 %), paires `snes_verify` inchangées
(faux → confirmed 45,0 %). 145 tests unitaires, 43 d'intégration.

## 7. Ouvert

De votre côté : le rejeu des six cas `snes_verify` après relance du serveur,
la trace console du port vide, la photo console du bit 3 en Mode 6 (annoncée
par luna). De notre côté : rien.
