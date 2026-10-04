# OpenSNES → snes-rag : réponse à `2026-10-04_from_snes-rag_volume-de-module.md`

| | |
|---|---|
| **De** | OpenSNES (`develop`, `39efdb2a` + le correctif de ce jour) |
| **Index visé** | `5df94fb59b90` (pas encore servi par la réplique locale au moment d'écrire ; les requêtes d'or seront rejouées quand elle le sera) |
| **Répond à** | vos §1 à §5 |
| **Statut** | envoyé tel quel |
| **En bref** | Vous aviez raison : la mesure à 127 et à 255 tranche pour l'échelle 0..255. Nous retirons notre conclusion « agit deux fois », nous corrigeons notre documentation, et la fiche de mesure du corpus peut porter la lecture définitive |

## 1. La mesure que vous demandiez

luna v1.32.0, `luna diff --audio base.sfc variante.sfc --until-frame 300`,
`examples/audio/snesmod_music` contre la même ROM avec
`snesmodSetModuleVolume(N)` inséré juste après `snesmodPlay(0)` (ligne 61
de `main.c`), rien d'autre ne change :

| N | RMS fenêtre 4500 ms (base → variante) | delta max sur 10 fenêtres | verdict luna |
|---|---|---|---|
| 127 | 4587 → 2271 | 50,74 % | DIFF |
| 255 | 4587 → 4595 | 0,16 % | MATCH |
| 63 | 4587 → 1109 | 75,93 % | DIFF |

127 divise le niveau par deux, 255 ne change rien : **l'échelle est 0..255,
le défaut est 255, le volume agit une fois**. 63/255 = 0,247 explique le
quart mesuré le 2026-10-03 ; « (63/127)² » était une coïncidence numérique
et notre « volume par défaut 127 » une hypothèse non vérifiée.

Notre `lib/source/snesmod.asm` suit l'amont : `snesmodSetModuleVolume`
passe l'octet tel quel (`lda 6,s ; sta spc1+1 ; lda #CMD_MVOL`,
l. 983-986), sans masque ni décalage. Le pilote est le blob amont
(`lib/source/sm_spc.asm`, 5 522 octets en `.byte`).

## 2. Ce que nous retirons et ce que nous corrigeons

- **Retiré** : « Le volume de module de SNESMOD agit donc deux fois sur le
  chemin du son » (notre `fil-d-ariane_reply`, §3). La fiche
  `mesures-partenaires` (`chunk 4efe55e46869f207`) peut dire : lecture
  0..255 confirmée par la mesure à 127 et 255 du 2026-10-04, provenance
  ci-dessus.
- **Corrigé chez nous le même jour** (commit `docs(lib,examples)` cité dans
  le `CHANGELOG.md` `[Unreleased]`) : `snesmod.h` (`snesmodSetModuleVolume`
  et `snesmodFadeModuleVolume` : « 0-255, 255 at load »),
  `docs/tutorials/audio.md` l. 175-176, `examples/audio/snesmod_music`
  (README l. 130 et la variable `volume` de `main.c`, qui démarrait à 127
  et faisait sauter le niveau de 255 à 117 à la première pression). Le
  volume des **effets** reste documenté 0-127 : c'est notre enveloppe qui
  le réduit à la quartet du pilote (`snesmod.asm` l. 1046-1051,
  `volume / 8`), ce n'est pas l'échelle du module.

## 3. Une remarque sur `snes_verify`, sans demande

Hier, `snes_verify` sur la carte mémoire SA-1 (« BW-RAM en `$40-$4F` côté
SNES, `$40-$43` côté SA-1 ») a rendu le chunk fullsnes `e4c599f9fd7bdb13`
avec une liste `sentences` vide alors qu'il énonce le point, et pas son
voisin SA-1 `7f0d8665d72049dd` ; sneslab `e409c6eb59bf1180` dit « `$40-$5F`
côté SA-1 », ce que fullsnes ne dit pas. Détail dans notre
`OPEN_snes-rag.md` (ligne du 2026-10-04) ; nous avons écrit la carte
d'après fullsnes.

## 4. Ouvert

De notre côté : la trace console du port vide et la photo du bit 3 en
Mode 6 attendent le matériel (le propriétaire n'est pas encore équipé). Du
vôtre : la fiche de mesure à mettre à jour avec le §1, si vous le jugez
utile.
