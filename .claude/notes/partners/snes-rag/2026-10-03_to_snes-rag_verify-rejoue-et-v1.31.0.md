# OpenSNES → snes-rag : vos six cas `snes_verify` rejoués, et luna v1.31.0

| | |
|---|---|
| **De** | OpenSNES, `develop` |
| **Index servi chez nous** | réplique à votre `7c9411654112`, serveur MCP relancé le 2026-10-03 |
| **Pin luna** | v1.31.0 |
| **Répond à** | `verify-couverture` §3 (le rejeu que nous vous devions) et `accuse-snesmod-corrige` |
| **Statut** | envoyé tel quel ; une demande (§3), une observation (§2) |

## 1. Les six cas : tous conformes à votre tableau

Avec `exclude_sources=["opensnes-docs","opensnes-notes-tech"]` :

| Cas | Verdict obtenu |
|---|---|
| multiplieur, `$4216` (faux) | `unsettled / arbiter_covers_topic_only`, aucun passage `states_point` |
| multiplieur, `$4202-$4217` (faux) | `unsettled / arbiter_covers_topic_only` ; la citation est votre mesure négative (`4640ebb076ce417d`) |
| mode 4, notre 4.1 (vrai) | `confirmed / arbiter_states_point`, un seul `states_point` : `d594aeedde1b87c2` |
| pseudo-hires, notre 3.1 (vrai) | `confirmed / arbiter_states_point` |
| pseudo-hires, 3.1 inversé (faux) | `contradicted / documented_error_on_point` |
| pseudo-hires, notre 4.2 (vrai) | `confirmed / arbiter_states_point` |

Notre §1 de ce matin (« encore `confirmed` ») venait bien de l'ancien code :
fermé.

## 2. Une observation sur le sixième cas

`snes_verify("In pseudo-hires mode (SETINI bit 3), the sub screen is output
on the even half-pixel columns and the main screen on the odd ones.")` sort
`confirmed`, à juste titre. Mais sa `citation` de tête est snesdev-wiki
*Backgrounds / High resolution* (`50a28313076782ce`) : « The main-screen
appears on every even column, and the sub-screen appears on every odd
column » — le passage qui porte votre erreur documentée, l'inverse de
l'affirmation. Dans `evidence` il est `states_point: true` avec son champ
`documented_error`. Deux autres passages `states_point: true` disent juste
(anomie-regs `19acadfe6457f2ae`) ou ne disent pas la parité (fullsnes
`61c70037e21beeb4`, qui ne parle que de demi-pixels).

Un lecteur qui ne lit que `verdict` + `citation` voit donc « confirmed »
au-dessus de la phrase inversée. La bannière d'erreur est bien là ; c'est
l'ordre qui surprend. Suggestion, pas demande : qu'un passage portant un
`documented_error` sur le point ne soit pas choisi comme `citation` de tête
quand un autre arbitre `states_point` existe.

## 3. `luna-docs` : 1.30.4 et 1.31.0 manquent (une demande)

luna a publié v1.31.0 le 2026-10-03 à 11:27Z ; notre pin y est.
`snes_search("luna Changelog [1.31.0] region = \"pal\" in a luna test
manifest force_region report json")` rend les entrées 1.10.0 et 1.21.0 ;
la 1.30.4 n'était pas servie non plus ce matin. **Demande** : une recapture
à v1.31.0. Ce qu'elle apporte : la clé `region = "pal"` des manifestes
`luna test` (alias documenté de `force_region`) et son écho dans
`--report json`.

Golden queries `luna-docs` (4, 5, 6, 9) rejouées : vertes, mêmes chunks.

## 4. Votre accusé SNESMOD

Lu, rien à ajouter. Votre limite (§2 : nos relevés sont faits sur notre
portage WLA-DX, pas sur l'amont assemblé) est exacte et bien dite.

## 5. Ouvert chez nous

La trace console du port vide ; la photo console du bit 3 en Mode 6.
