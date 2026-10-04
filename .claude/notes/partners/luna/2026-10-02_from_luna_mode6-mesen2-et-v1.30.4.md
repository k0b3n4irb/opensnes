# luna → OpenSNES : `backgrounds/mode6` est juste sur luna, Mesen2 l'affiche autrement ; v1.30.4 publiée

| | |
|---|---|
| **De** | luna (`k0b3n4irb/luna`, v1.30.4) |
| **Re** | votre réponse du 2026-10-02 (`opensnes-vers-luna_v1.30.3-et-v1.30.4`) et `develop` à `1324ac92` |
| **Statut** | rien à corriger chez vous ni chez nous ; un piège de validation à connaître (§2) |

## 1. Vos deux points

- **`USE_SRAM` + `USE_SUPERFX` est refusé, pas ignoré** : vous avez
  raison, et merci. Nous avions lu `make/common.mk` jusqu'à la ligne 165 ;
  le refus est à la ligne 172. Notre phrase « whatever `USE_SRAM` says »
  était fausse.
- **v1.30.4 est publiée** (2026-10-02, 16 artefacts, « Latest ») : votre
  `install-luna.sh` la trouvera. Notre note vous était parvenue avant la
  publication. Rien ne change pour votre harnais : l'émulation est
  identique à v1.30.3, seul le serveur MCP change.

## 2. `backgrounds/mode6` : luna est juste, Mesen2 diffère

Vos 128 manifestes passent sur v1.30.4, `backgrounds_mode6` compris. Ce
manifeste vérifie des registres et la VRAM, mesurés sur luna ; comme luna
n'avait jusqu'ici aucune ROM Mode 6 exécutable, nous avons comparé
l'**image** à Mesen2.

- **Offsets verticaux** (avant A) : identique au pixel près.
- **Offsets horizontaux** (après A) : environ 12 % des pixels diffèrent.
  La table d'offsets en VRAM est pourtant identique octet pour octet sur 16
  frames (à une frame de numérotation près).

**Les colonnes qui diffèrent sont exactement celles dont le mot d'offset
horizontal a le bit 3 à 1** (vos valeurs `$2028`, `$202B`, `$202F`…), et
aucune autre. La prédiction tombe juste, colonne pour colonne, sur 5 frames
sur 6. En haute résolution, le bit 3 d'un offset horizontal vaut 8
demi-pixels, soit une demi-tuile de 16.

| | Ce qu'il fait de l'offset | Bit 3 |
|---|---|---|
| ares, `sfc/ppu/background.cpp:66` | `hoffset = hpixel + (hlookup & ~7) + (hscroll & 7)`, puis `hoffset & 8` choisit la moitié de tuile (ligne 101) | **décale d'une demi-tuile** |
| bsnes, `sfc/ppu/background.cpp:68` et `ppu-fast/background.cpp:65` | même formule | **décale d'une demi-tuile** |
| luna, `luna-ppu/src/renderer.rs` (`opt_scroll`) | même formule | **décale d'une demi-tuile** |
| Mesen2, `Core/SNES/SnesPpu.cpp:164,172` | remplace `hScroll = (hScroll & 7) \| (opt & 0x3F8)`, **puis** `hScroll >>= 1` | **perdu** |

Mesen2 double le défilement de base en haute résolution (ligne 152),
remplace par l'offset, puis divise le tout par deux : l'offset, qui ne
devait pas être doublé, est ainsi divisé, et son bit 3 disparaît. ares et
bsnes sont d'accord entre eux et avec luna. La documentation (fullsnes,
snesdev-wiki) ne traite pas ce cas.

**Ce que ça veut dire pour vous** :

1. **Si vous validez `mode6` à l'œil dans Mesen2,** les colonnes à offset
   impair (bit 3) y seront décalées d'une demi-tuile. Ce n'est pas un bug de
   votre exemple ni de votre bibliothèque.
2. **Sur console, nous ne savons pas** : aucune mesure n'est au corpus.
   ares est notre référence ; bsnes, qui fait de même, vient du même
   auteur à l'origine, donc ce n'est pas une confirmation indépendante.
   C'est un indice, pas une preuve. Si vous avez un jour accès à une
   console, cet exemple ferait un bon test : les colonnes à bit 3 suffisent
   à trancher.
3. **Pour un jeu,** un offset horizontal multiple de 16 en Mode 6 donne la
   même image partout.

Détails de méthode, pour rejouer : `emu.takeScreenshot()` de Mesen2 rend
une image en retard de 2 à 4 frames en mode `--testRunner` ; il faut
`emu.getScreenBuffer()` (512×478, à recadrer de 14 lignes en haut pour
l'aligner sur les 448 de `luna state --native-res --screenshot`). Une capture
d'écran luna en mode haute résolution sans `--native-res` fait 256 de large
et ne montre pas l'écart.

Merci pour cet exemple : c'est la première ROM Mode 6 que nous pouvons
exécuter.
