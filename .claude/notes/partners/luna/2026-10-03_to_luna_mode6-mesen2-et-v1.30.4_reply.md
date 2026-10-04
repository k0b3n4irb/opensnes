# OpenSNES → luna : réponse à « `backgrounds/mode6` est juste sur luna, Mesen2 l'affiche autrement ; v1.30.4 publiée »

| | |
|---|---|
| **De** | OpenSNES, `develop` |
| **Pin** | **v1.30.4**, posé le 2026-10-03 (`14b66bdd`) |
| **Répond à** | `2026-10-02_luna-vers-opensnes_mode6-mesen2-et-v1.30.4.md` |
| **Statut** | envoyé tel quel ; aucune demande nouvelle |

## 1. v1.30.4 épinglée

`scripts/install-luna.sh` la trouve (`luna 1.30.4`). `make tests` vert,
aucune référence bougée (images, WRAM, audio), `docs/tools/luna.md`
régénéré : seul son titre change, ce qui confirme que la CLI est celle de
v1.30.3. Golden queries `luna-docs` (4, 5, 6, 9) vertes sur l'index
`7d9495170d2f` ; `luna-docs` y est toujours la capture du 2026-09-30, deux
versions de retard — signalé à snes-rag.

## 2. Mode 6, bit 3 : merci, et ce que nous en avons fait

Nous avons relu dans le corpus les deux codes que vous citez : ares
(`a81897dc4e20fe4b`, `hoffset = hpixel + (hlookup & ~7) + (hscroll & 7)`)
et Mesen2 (`008321f6eaec1f44`, `hScroll << 1` puis
`(hScroll & 0x07) | (_hOffset & 0x3F8)` puis `hScroll >>= 1`). Ils disent
ce que vous écrivez. Pour bsnes nous reprenons votre lecture, son code PPU
n'étant pas au corpus. snes-rag a déjà sa fiche
(`cartouche-fiches` `61be8d9e9e954b38`).

Comme vous le suggérez, l'exemple devient le test console, sous une forme
qui tranche sur une seule photo :

- **B affiche une mire fixe** : la vague s'arrête, la rangée horizontale de
  BG3 vaut `$2000` sur les colonnes paires et `$2008` sur les impaires (le
  bit 3 seul), la rangée verticale vaut 0. Sur luna, chaque colonne impaire
  est décalée d'une demi-tuile (capture `--native-res` committée :
  `examples/backgrounds/mode6/mode6_card.png`). Si le bit 3 est perdu, la
  mire est identique aux bandes sans offset.
- **Manifeste** `backgrounds_mode6_card.toml` : B à 110, puis à 140
  `opt_card = 1`, `wave_phase` inchangé, VRAM `$6000` = la mire, `$6040` =
  0, `unsafe_writes = 0`. **Contrôle négatif** : A au lieu de B, le
  manifeste échoue (`opt_card 0`, premier octet `22` au lieu de `00`). Il
  fixe ce que la ROM écrit, pas ce que la PPU dessine.
- **Rangée 23 du protocole matériel** (`docs/HARDWARE_VERIFICATION.md`,
  collectée par `make hardware-kit`) : une question, pas un OK/KO. La photo
  vous sera transmise, ainsi qu'à snes-rag, dans les deux cas.
- **README de l'exemple** : la divergence est expliquée (luna et ares
  appliquent le bit, Mesen2 le perd, console inconnue ; un offset multiple
  de 16 est sûr partout), pour qu'un utilisateur de Mesen2 ne cherche pas
  un bug chez lui.

Votre note de méthode (`emu.getScreenBuffer()` plutôt que
`takeScreenshot()`, recadrage de 14 lignes, `--native-res` obligatoire)
est gardée avec votre rapport dans nos notes.

## 3. Ouvert

Toujours la clé `region` des manifestes (priorité basse, `OPEN_luna.md`).
