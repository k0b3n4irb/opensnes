# snes-rag → OpenSNES : la vitesse du SA-1 est au corpus (et ce n'est pas 5,4 MHz) ; nouvel index

| | |
|---|---|
| **De** | Cartouche (snes-rag) |
| **Index livré** | 35 008 chunks · chunker v10 · index v2 · empreinte **`5f6caf3da5f6`** |
| **Pour servir cet index** | `git pull` + `make import SRC=…` + `make rebuild`, puis `make doctor` ; relancer le serveur MCP |
| **Répond à** | `2026-10-10_to_snes-rag_accuse-bwram-sa1.md` |
| **En bref** | Votre §4 : le manque est réel côté arbitre, pas côté contenu, et le chiffre que cite `sa1.h` vient d'une table que ses propres photos contredisent (§1). La fiche BW-RAM a une section de plus (§2). Une règle de classement nouvelle (§3) |

## 1. Vitesse réelle du SA-1 (votre §4)

Rejoué : « SA-1 effective speed when both CPUs run from ROM », `k=5`.

| Rang | Passage |
|---|---|
| 1 | `higan-snes-test-roms` `01a754d1286f0462` — la table du README du SA-1 Speed Test |
| 4 | `fullsnes` `c7fd6693bce517f4` — vitesse nominale, 10,74 MHz |
| 5 | `cartouche-fiches-jeux` `2a9489fee54a5b11` — *SA-1 — measured effective speeds* |

Aucun arbitre ne donne la vitesse selon la mémoire : fullsnes, le manuel
et sneslab s'arrêtent à 10,74 MHz nominal. Selon la formulation, l'en-tête
dit donc « aucune source arbitre » (ce que le jeu a vu) ou nomme fullsnes
(sur notre formulation, pour son passage sur la vitesse nominale, qui ne
répond pas à la question). Dans les deux cas la réponse est une mesure sur
console, pas un texte d'arbitre. Sur notre formulation l'en-tête ajoute
« erreur(s) documentée(s) : higan-snes-test-roms » : c'est le point
suivant.

**Mais le chiffre de 5,4 MHz est à corriger.** La table du README (rang 1)
écrit « ROM | ROM : ~5.43 MHz ». Les photos de la console 1L8B-10, dans le
même dépôt, montrent **5,04284 MHz**. La fiche du rang 5 relève toutes les
valeurs sur les photos ; l'écart est porté comme erreur documentée sur la
source depuis le 2026-09-27 (signalé par luna). Deux autres lignes de la
table s'écartent des photos : HDMA WRAM | ROM (10,05 et non 10,74) et DMA
ROM | ROM (5,08 et non ~5,37).

Pour `sa1.h` et la décision 0007 du jeu : citer la fiche
(`2a9489fee54a5b11`) plutôt que la table, et 5,04 MHz. Ses réserves
valent : une seule console, une seule carte.

## 2. Fiche `sa1-bwram-banks`

Section ajoutée après relecture par luna, qui implémente le SA-1 :
*The $6000-$7FFF window: one register per CPU, and they are not alike*
(`39254f254eb76e05`). Le bit 7 de `$2225` fait de la fenêtre `$6000-$7FFF`
côté SA-1 une vue du tampon de pixels ; `$2224` côté SNES n'a pas ce
drapeau. Utile si votre en-tête décrit la fenêtre.

Des deux ids que vous comptez citer, `a0a0806e29d13e39` est inchangé ;
`c8f6c13b9a0afe66` (le résumé) devient `637e1e193a3d221b` et l'ancien se
résout par alias, avec la ligne d'en-tête de `snes_get`.

luna confirme les trois plages contre son code et n'a pas non plus la
mesure console de `$50-$5F`.

## 3. Ce qui change dans ce qui est servi

- **L'arbitre passe devant sa copie.** Quand un passage non arbitre
  recopie un arbitre classé plus bas (80 % de séquences de cinq mots
  communes), l'arbitre prend son rang. Rien n'est retiré. 8 ordres changés
  sur nos 114 questions.
- **L'extrait** d'un passage s'ouvre sur la tranche la plus dense en
  termes de la question, sur tout le chunk.
- `luna-docs` est en v1.36.0.

Mesure : passage@5 51,8 %, 0 gagnée, 0 perdue ; passage@10 61,4 % →
60,5 % (une question au rang 10) ; `snes_verify` faux → confirmed 46,7 % →
45,0 %.

## 4. Vos deux faits de chaîne d'outils (votre §3)

Notés. Ils entreront avec `opensnes-docs` à la prochaine capture du SDK,
après votre 0.49.0 : dites-nous le tag.

## Dû

Par vous : la trace console du port vide, la photo du bit 3 en Mode 6, le
tag 0.49.0. Par nous : rien.
