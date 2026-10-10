# OpenSNES → luna : rapport après l'épinglage de la v1.33.1

| | |
|---|---|
| **De** | OpenSNES (`k0b3n4irb/opensnes`, `develop` @ `29ddff19`) |
| **Date** | 2026-10-06 |
| **luna utilisée** | v1.33.1, épinglée ce jour (`testing/luna.version`), binaire `linux_arm64` |
| **Statut** | Envoyé tel quel. Chaque point ci-dessous a été rejoué aujourd'hui sur la v1.33.1 avec la commande citée. |
| **En bref** | L'épinglage est vert sans une baseline qui bouge. Deux de nos demandes sont closes par la v1.33.0. Trois demandes restent, la plus simple d'abord (§2). Votre correctif de mosaïque nous a fait trouver un trou chez nous (§1). |

## 1. Ce que la v1.33 a rendu possible

- **Épinglage sans douleur.** `make tests` complet sur la v1.33.1 : 86 ROM
  vivantes (RAM à zéro puis `--power-on random=1`), 86 images identiques à
  celles de la v1.32.0, 138 manifestes, 48 phases, audio 11/11, oracle WRAM,
  budget NMI, DMA VRAM en blanking, fixtures. Aucune baseline recapturée pour
  une autre raison que le tampon de version.
- **`rom.checksum_computed`** (notre demande du 10-05) : rejoué sur
  `examples/text/print_string/print_string.sfc` dont l'octet `$0100` est mis
  à `$5A` : `checksum` 41211, `checksum_computed` 41285, `checksum_valid`
  `true`. Notre harnais compare désormais les deux champs sur chaque ROM, et
  notre nouvel outil `opensnes-rom inspect` rend le même chiffre (`$A145`)
  par un calcul indépendant.
- **`--dsp-trace`** (notre demande du 10-03) : `luna state
  examples/audio/echo/echo.sfc --until-frame 300 --dsp-trace t.csv` donne 92
  lignes, aucune à 0, la première `95632,$6C,FLG,$20`. Close.
- **Le correctif de mosaïque nous a montré un trou.** La grille descendue
  d'une ligne n'a fait bouger aucune de nos références : aucun oracle ne
  regardait une image avec la mosaïque active (notre manifeste n'affirmait
  que l'ombre de `$2106`). Nous avons ajouté
  `testing/manifests/transition_mosaic_picture.toml`, qui s'arrête sur
  `mosaicFadeOut` au maximum et hache l'image : `9980548a31063b25` sur la
  v1.33.1, `b40b834549507f25` sur la v1.32.0 (le manifeste y échoue). Rien
  à vous demander : c'est un merci.

## 2. Demandes, de la plus simple à la plus coûteuse

### D1 — Un nom de `static` sans suffixe, quand il n'a qu'un candidat

**Nature :** confort, pour le développeur de jeu. **Coût estimé :** faible.

Depuis notre correctif des `static` homonymes, une variable `static` de
niveau fichier s'appelle `nom.<source>` dans le `.sym` (`player_x.main`).
Le nom C seul n'est pas résolu :

```
opensnes init demo --template game && cd demo && opensnes build
# test/boot.toml, [checkpoint.values] : remplacer "player_x.main" = 120 par player_x = 120
luna test test/boot.toml
→ checkpoint@180 values.player_x: not a loaded symbol and not BANK:OFFSET
```

Nous avons renommé nos manifestes et le gabarit de projet, mais celui qui
écrit son premier test tapera le nom qu'il lit dans son C.

Contrat souhaité : si le nom exact est absent et qu'**un seul** label est de
la forme `nom.<quelque chose>`, le résoudre ; s'il y en a plusieurs, refuser
en les nommant. Contrôle négatif prêt chez nous :
`testing/fixtures/compiler/static_dup/static_dup.sym` porte `k.main`
(`00:00c3`) et `k.other` (`00:00c9`) : `k` doit rester ambigu. Même règle
pour `--assert`, `--peek` par nom et le MCP.

### D2 — Les sommes des archives avec chaque release

**Nature :** distribution. **Coût estimé :** faible (une étape du workflow de release).

`gh release view v1.33.1 --repo k0b3n4irb/luna --json assets` : quatre zips,
aucune somme. Notre installateur refuse une archive dont le SHA-256 n'est pas
épinglé ; nous calculons donc nous-mêmes les quatre sommes à chaque
épinglage, à partir de ce que nous venons de télécharger, ce qui ne prouve
rien sur l'origine. Souhaité : un actif `SHA256SUMS` (ou `<zip>.sha256`) par
release, et une ligne dans les notes quand les noms d'actifs changent (le
04-10, un consommateur épinglé n'a vu qu'un 404).

Pour mémoire, les sommes que nous avons relevées pour la v1.33.1 :

```
aac5bba6eeb95176deb817f53db1de59a83a85c52c324dbdad298089ff5b39af  luna_v1.33.1_linux_x86_64.zip
eba331a8dfb8a6bdf3d42d50780acb329eb04cd366936632d8e43d237c605ea3  luna_v1.33.1_linux_arm64.zip
863d6493884c38ad5fcd1422f4e4cf979d945152cb5bcd020057d053804cd028  luna_v1.33.1_darwin_arm64.zip
753fa41149620eacaf474dd3b73a03196e53179ae691abcb090a5cba3f3e49ee  luna_v1.33.1_windows_x86_64.zip
```

### D3 — `--power-on random` ne remplit pas la RAM de la cartouche

**Nature :** capacité de test. **Coût estimé :** moyen.

L'aide dit « Fills WRAM, VRAM, CGRAM, OAM and APU RAM ». Mesuré à la
trame 0, graine 1 :

```
luna state examples/chips/superfx_3d/superfx_3d.sfc --until-frame 0 --power-on random=1 --peek 70:0000:10 --peek 7F:8000:10
  $700000  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00      ← RAM du Game Pak (Super FX)
  $7F8000  A4 93 96 D7 5A E9 33 23 3D F2 68 6A C9 7F 55 94      ← WRAM (témoin)

luna state examples/chips/sa1_hello/sa1_hello.sfc --until-frame 0 --power-on random=1 --peek 00:3000:10 --peek 40:0000:10 --peek 7F:8000:10
  $003000  00 …   ← I-RAM (SA-1)
  $400000  00 …   ← BW-RAM (SA-1)
  $7F8000  A4 93 96 D7 5A E9 33 23 3D F2 68 6A C9 7F 55 94
```

Une lecture d'un framebuffer Super FX ou d'un octet d'I-RAM jamais écrit ne
peut donc pas se voir sur luna, alors que c'est exactement la classe de
défaut que `--power-on random` nous attrape en WRAM (notre passe de vivacité
la joue à chaque `make tests`). Souhaité : le même remplissage, de la même
graine, sur la RAM de cartouche sans pile (RAM du Game Pak, BW-RAM, I-RAM),
par défaut ou derrière un `--power-on-cart`. La RAM sauvegardée par pile
chargée depuis un `.srm` doit évidemment rester ce que dit le fichier.

## 3. Priorités

| | Demande | Pour qui | Priorité |
|---|---|---|---|
| D1 | nom de `static` sans suffixe | le développeur de jeu, à son premier test | haute |
| D2 | sommes des archives | tout consommateur épinglé | moyenne |
| D3 | RAM de cartouche aléatoire | nos tests des puces | moyenne |

## 4. Observations sans demande

- Deux points de notre liste ne sont **pas** envoyés, faute de les avoir
  rejoués aujourd'hui : l'absence de diagnostic quand CFGR.MS0 et CLSR
  (21 MHz) sont posés ensemble, et le chargement d'un LoROM DSP-1 de 2 Mo.
  Ils reviendront avec une ROM de reproduction ou pas du tout.
- La v1.33.1 est notée après la v1.33.0 dans les versions, mais c'est la
  v1.33.0 qui porte l'étiquette « Latest » sur la page des releases
  (`gh release list --repo k0b3n4irb/luna --limit 3`). Nous avons épinglé
  la v1.33.1.
- Le corpus snes-rag sert déjà votre changelog `[1.33.0]`
  (`6c16d06712970f52`) : rien à leur signaler de notre côté.
