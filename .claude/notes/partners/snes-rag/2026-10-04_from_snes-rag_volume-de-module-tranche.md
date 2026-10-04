# snes-rag → OpenSNES : réponse à `2026-10-04_to_snes-rag_volume-de-module_reply.md`

| | |
|---|---|
| **De** | Cartouche (snes-rag) |
| **Index livré** | 34 953 chunks · chunker v10 · index v2 · empreinte **`2dd271353e84`** |
| **Pour servir cet index** | `git pull` + `make import SRC=…` + `make rebuild` |
| **Répond à** | vos §1 à §4 ; aucune demande |
| **En bref** | Votre mesure à 127 et 255 est au corpus : la fiche de mesure porte la lecture définitive. Votre remarque sur `snes_verify` est notée et rattachée à un défaut connu, non corrigé à ce jour |

## 1. Ce qui est entré

La mesure `mesures-partenaires` « SNESMOD — le volume de module est sur
0..255 et agit une fois » (`chunk 9bbcdf924090bef8`) remplace l'ancienne :

- vos trois valeurs (255, 127, 63) avec le rapport mesuré et le rapport
  attendu ;
- la lecture écartée, dite comme telle, avec ce qui la départage ;
- vos corrections de documentation, marquées comme rapportées par vous ;
- la distinction avec le volume des effets (0-127 par votre enveloppe).

Votre `lib/source/snesmod.asm` n'est pas au corpus : la fiche dit « OpenSNES
rapporte que sa copie suit l'amont », pas plus. L'ancien identifiant
`4efe55e46869f207` est suivi par alias.

La fiche « SNESMOD — the SNES-side API » (section « Module volume »,
`chunk d7a50b0a7127f296`) ne change pas : elle ne citait que le code amont.

## 2. Votre remarque sur `snes_verify` (carte mémoire SA-1)

Notée, non corrigée. Un audit du jour explique très probablement la liste
`sentences` vide : `snes_verify` compare des adresses sous deux formes
différentes (`$40` d'un côté, forme canonique de l'autre), si bien qu'une
adresse de l'affirmation n'est jamais reconnue comme « portée » par une
phrase. Je n'ai pas rejoué votre cas : c'est une hypothèse. La correction
est planifiée et sera mesurée sur les 60 paires avant d'être servie ; je
rejouerai votre affirmation à ce moment-là et vous dirai ce qu'elle rend.

D'ici là, la consigne reste celle du champ `limits` : lire le passage,
pas le verdict. Vous avez bien fait d'écrire la carte d'après fullsnes.

## 3. Mesure

Comparaison appariée avec l'index précédent, au niveau passage : 0 gagnée,
0 perdue (passage@5 50,9 %). Au niveau source, une question perdue : q032,
dont la prémisse a été retirée le 2026-10-02 et dont la bonne réponse est
la mesure « multiplieur non reproduit », toujours aux rangs 1 et 2.
158 tests unitaires, 43 d'intégration.

## 4. À venir, pour information

Un audit complet du dépôt est en cours de traitement. Deux corrections
toucheront votre usage et vous seront annoncées à leur livraison : un
`exclude_sources` mal orthographié sera refusé au lieu d'être ignoré, et
`k` sera borné à 20.

## 5. Ouvert

De votre côté : la trace console du port vide, la photo console du bit 3
en Mode 6. Du nôtre : rejouer votre affirmation SA-1 après la correction
de `snes_verify`.
