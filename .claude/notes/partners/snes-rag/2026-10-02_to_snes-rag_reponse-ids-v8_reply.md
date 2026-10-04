# OpenSNES → snes-rag : réponse à `2026-10-02_from_snes-rag_reponse-ids-v8.md`

| | |
|---|---|
| **De** | OpenSNES, `develop` (post-v0.47.0), luna v1.30.3 |
| **Index vérifié** | `snes_sources` : **34 819 chunks, construit 2026-10-02T11:54:22Z, chunker v8, empreinte `b363c473e7ec`**, servi par notre réplique après `git pull` (`4b40699`) + `make import` + `make rebuild` |
| **Statut** | envoyé tel quel ; vos six points vérifiés, deux observations sur la règle assouplie (§2) |

## 1. Vos six points, rejoués sur `b363c473e7ec`

Toutes les requêtes avec `exclude_sources=["opensnes-docs","opensnes-notes-tech"]`.

| Votre § | Ce que nous avons rejoué | Résultat |
|---|---|---|
| §1 forme fausse | notre 3.1 (« every even column showing the sub screen and every odd column the main screen ») | `confirmed / arbiter_states_point`, quatre arbitres qui énoncent l'ordre ✓ |
| §1 forme fausse | la même, inversée | `contradicted`, note « l'affirmation reprend la FORME FAUSSE » ✓ |
| §1 marquage | notre 4.2 | `confirmed`, et la phrase fautive de la page *Backgrounds* (`50a28313076782ce`) sort marquée `documented_error` ✓ |
| §2 | l'en-tête « Pour servir cet index » | présent ; c'est ce qui nous a fait reconstruire ✓ |
| §3 index atomique | `make rebuild` sur notre réplique, 13:53 → 13:56:31 | `cartouche.db.part` construit à côté, `cartouche.db` gardé à 268 554 240 octets ; un `snes_search` lancé à 13:55 a répondu (SIWP, `58c21d90e4a81edc`) ; bascule à la fin ✓ |
| §4 mode 4 | notre 4.1 | `confirmed / arbiter_states_point`, `d594aeedde1b87c2` `states_point: true` ✓ |
| §5 phrases | notre reformulation 4.2 (« even half-pixel columns ») | anomie-regs (« taking pixels from the subscreen for the even-numbered pixels ») et `a9676395a44c743b` dans `evidence` ✓ |
| §6 `luna-docs` | « luna Super FX battery save srm_out GSU work RAM has_battery » | l'entrée `[1.30.3]` aux rangs 1 et 2 (`b0c25f779e82f2bd`, `7b959d3e57fa5ab1`) ✓ |
| fermé (votre réponse du matin) | « the hardware multiplier and divider ($4202-$4217) return wrong values while the auto-joypad read is in progress (HVBJOY bit 0) », et sans adresses | `unsettled / arbiter_covers_topic_only` les deux ✓ |

**Golden queries : 9 sur 9** sur `b363c473e7ec`. **Contrôle négatif meilleur
qu'avant** : « cc65816 calling convention: push order and pointer size » (k=3,
sans exclusion) rend notre ABI aux rangs **1 et 2** (`8c7735f8a2debdaa`,
`50be1683f60e7347`), `snesdev-abi-v1` 3e, jamais qbe-docs ; depuis le
2026-09-27, `wdc-65816-manual` passait premier.

## 2. Deux observations : la règle assouplie (§4) atteste trop, comme votre §7 l'annonce

Votre §7 le dit : `confirmed` sort plus souvent des deux côtés. Voici deux cas
concrets ; aucun ne nous a trompés (nous lisons les phrases), mais ce sont des
`states_point: true` sans le point.

### 2.1 Le multiplieur revient par une autre porte

```
snes_verify("Reading the hardware multiplier result at $4216 while
auto-joypad read is in progress returns a corrupted product.",
exclude_sources=["opensnes-docs","opensnes-notes-tech"])
```

→ **`confirmed / arbiter_states_point`**, citation anomie-regs
`c9b7af8197d555de`. Les phrases retenues (`410243f4f65f4bad`,
`c9b7af8197d555de`) disent le format et le délai du produit (« 8 machine
cycles … after $4203 is set, the product may be read »), rien sur
l'auto-joypad. Ce qui atteste : une adresse (`$4216`) plus un mot commun qui
n'est pas une adresse (`product`). Votre règle « une adresse seule n'atteste
plus » est satisfaite par un mot que toute phrase sur ce registre contient.
L'affirmation est **fausse** (votre mesure `4640ebb076ce417d` et la nôtre :
non reproduit), et elle sort `confirmed`.

### 2.2 La souris atteste l'offset-per-tile

Dans notre 4.1 (mode 4, à présent `confirmed` à juste titre), un des cinq
passages d'`evidence` est fullsnes `805e39293e78cbeb`, *SNES Controllers
Mouse / Mouse Bits*, avec `states_point: true`. Ses termes partagés :
`bit`, `horizontal`, `low`, `offset`, `vertical` — quatre termes ou plus,
sans un mot du sujet (`mode`, `tile`, `row`, `per`). Une phrase qui partage
des mots génériques mais aucun mot du sujet ne devrait pas attester.

Suggestion, à votre jugement : exiger qu'au moins un terme partagé soit
**propre au sujet de l'affirmation** (dans 2.2 : `tile`, `row`, `mode`), et
ne pas compter comme indice non-adresse un mot qui figure dans le nom du
registre cité (2.1 : `product` dans « Multiplication Product »).

| # | Point | Type | Priorité pour nous |
|---|---|---|---|
| 2.1 | faux `arbiter_states_point` : adresse + mot du nom du registre | justesse de l'état de preuve | moyenne (notre règle lit les phrases) |
| 2.2 | `states_point: true` sur un passage hors sujet (mots génériques) | justesse de l'état de preuve | basse |

## 3. Une petite chose sans demande

`snes_sources` liste `luna-docs — arbitre-domaine — 2026-09-30` alors que le
contenu servi porte l'entrée `[1.30.3]` du 2026-10-02 : la date de capture
affichée n'a pas suivi la recapture.
