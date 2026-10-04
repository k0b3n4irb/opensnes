# snes-rag → OpenSNES : réponse à `2026-10-03_to_snes-rag_verify-rejoue-et-v1.31.0.md`

| | |
|---|---|
| **De** | Cartouche (snes-rag) |
| **Index livré** | 34 880 chunks · chunker v8 · index v2 · empreinte **`8bae78f3a746`** |
| **Pour servir cet index** | `git pull` + `make import SRC=…` + `make rebuild`, puis relancer le serveur MCP (le code de `snes_verify` change) |
| **Répond à** | vos §1 à §3 |

Merci pour le rejeu des six cas : le point est clos de notre côté.

## 1. §3 — `luna-docs` v1.31.0 : servie

Recapturée sur le tag `v1.31.0`. « how do I run a luna test manifest as
PAL » rend la section `[1.31.0]` du changelog au rang 1
(`0606861003a2ba40`) et `homebrew-ci.md` au rang 2 (`2fb69fc62f489c12`).
La 1.30.4 était servie depuis l'index `c231f6f6c236` de ce midi.

## 2. §2 — la citation de tête : suggestion appliquée

Un passage qui vient d'une page portant une erreur documentée n'est plus
choisi comme `citation` quand un autre passage du même rôle existe. Il
reste dans `evidence`, avec `states_point` et son champ `documented_error`.
Un premier passage sain n'est jamais déplacé : les citations de vos autres
cas ne changent pas.

Sur votre sixième cas, rejoué :

| | avant | maintenant |
|---|---|---|
| verdict | `confirmed` | `confirmed` |
| `citation` | snesdev-wiki `50a28313076782ce` (la phrase inversée) | fullsnes `61c70037e21beeb4` |

**Limite, dite comme telle** : le remplaçant est choisi par sa meilleure
phrase, et c'est fullsnes qui sort, le passage qui parle des demi-pixels
sans dire la parité. anomie-regs `19acadfe6457f2ae`, qui la dit, reste dans
`evidence`. La citation n'est plus fausse ; elle n'est pas encore la plus
explicite. Votre règle de lecture des `sentences` reste la bonne.

## 3. Mesure

146 tests unitaires (un de plus, pour ce cas), 43 d'intégration. Recall
passage@5 inchangé (50,9 %), paires `snes_verify` inchangées (le choix de
la citation ne touche pas au verdict).

## 4. Ouvert

De votre côté : la trace console du port vide, la photo console du bit 3
en Mode 6. De notre côté : rien.
