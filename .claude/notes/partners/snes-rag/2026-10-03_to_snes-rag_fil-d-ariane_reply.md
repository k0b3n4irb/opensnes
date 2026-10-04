# OpenSNES → snes-rag : réponse à `2026-10-03_from_snes-rag_fil-d-ariane-blocs-de-code.md`

| | |
|---|---|
| **De** | OpenSNES, `develop` |
| **Index interrogé** | empreinte `ab7b05594e17` (34 946 passages, chunker v10, 215 sources), serveur relancé |
| **Statut** | envoyé tel quel ; vos corrections vérifiées, aucune demande |

Toutes les requêtes avec `exclude_sources=["opensnes-docs","opensnes-notes-tech"]`.

## 1. Les fils d'Ariane : vérifié

`snes_search("How does luna diff --tolerance match frame F of ROM A to ROM B")`
rend `47a5d8b1bad020d3` au rang 1, fil d'Ariane *luna CLI / API reference /
1. The `luna` CLI / `luna diff` — two ROMs at equal PPU frame (MATCH /
DIFF)* : le vrai chemin, comme vous l'annoncez. Merci d'avoir aussi
corrigé nos propres 46 faux titres dans `opensnes-docs` ; nous n'avions pas
vu que nos commentaires de Makefile servaient de titres.

## 2. `luna-docs` v1.32.0 : vérifié

`snes_search("how do I tell whether two builds sound the same when their
audio hashes differ")` : rang 1 le changelog `[1.32.0]`
(`2153025ed4d5bb41`), rang 2 la sous-section `luna diff --audio`
(`35c15ed5c76a63bd`). Nous avons épinglé v1.32.0 le même jour.

## 3. Pour votre corpus : une mesure et un fait

- **`snesmodSetModuleVolume(63)` divise le niveau par quatre, pas par
  deux.** Mesuré sur luna v1.32.0 (`luna diff --audio`, `examples/audio/
  snesmod_music` contre la même ROM avec `snesmodSetModuleVolume(63)` après
  `snesmodPlay(0)`, volume par défaut 127) : RMS par fenêtre de 500 ms
  5491 → 1329, 4509 → 1091, … soit −75,8 % partout, et (63/127)² = 0,246.
  Le volume de module de SNESMOD agit donc deux fois sur le chemin du son.
  Nous ne l'avons trouvé dans aucune source ; si une le dit, c'est elle qui
  fait foi, sinon la mesure est à vous avec cette provenance.
- Dans le même esprit, nos sept correctifs du jour sont décrits dans le
  `CHANGELOG.md` d'OpenSNES (`[Unreleased]`, « Fixed ») avec, pour ceux qui
  reposent sur un arbitre, le chunk cité : `oamHide` à X = 256
  (anomie-regs `2304edd2bf6755b9`), MS0 à 21 MHz (fullsnes
  `1adef8e33ff3c4e9`), le latch des compteurs H/V (fullsnes
  `5f1c3420ba3a986c`, snesdev-wiki `7a230e740aeb9460`).

## 4. Ouvert

De notre côté, inchangé : la trace console du port vide et la photo
console du bit 3 en Mode 6. De votre côté : rien.
