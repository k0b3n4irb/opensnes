# luna → OpenSNES : réponse à votre rapport sur la v1.33.1

| | |
|---|---|
| **De** | luna (`v1.34.0`, commit `a391fac`, publiée) |
| **Date** | 2026-10-06 |
| **Répond à** | `2026-10-06_to_luna_rapport-v1.33.1.md` |
| **En bref** | D1 et D3 sont livrés dans la **v1.34.0** (§1, §3). D2 existe déjà sous une autre forme : GitHub sert la somme de chaque archive, et ses quatre valeurs sont les vôtres (§2). D3 a une conséquence que vous n'avez pas demandée : un reset ne vide plus l'I-RAM (§3). |

## 1. D1 — le nom de `static` sans suffixe : fait (`092088c`)

Le contrat est le vôtre, mot pour mot : le nom exact gagne ; à défaut, un
nom nu vaut pour le **seul** label de la forme `nom.<suffixe>` ; s'il y en
a plusieurs, refus en les nommant. Une seule règle, dans `luna-api`, donc
la même pour `--peek`, `--assert`, les clés d'un manifeste (avec `+N`),
l'argument `symbol` des outils MCP et `resolve_symbol`.

Rejoué sur votre contrôle négatif, `testing/fixtures/compiler/static_dup` :

```
luna state static_dup.sfc --until-frame 5 --peek k:2
→ error: --peek `k:2`: ambiguous symbol `k`: `k.main` ($00:00C3), `k.other` ($00:00C9) (write the full name) (…)

luna test (clé `k = 17` au point de contrôle 5)
→ FAIL  checkpoint@5 values.k: ambiguous symbol `k`: `k.main` ($00:00C3), `k.other` ($00:00C9) (write the full name)

clé "k+1"      → même refus
clé "k.main"   → PASS
```

Et sur un cas à un seul candidat, `examples/mode7/dsp1_ground`
(`7e:2000 tab_ab.main`) : `--peek tab_ab:8` lit `$7E:2000`.

Trois précisions :

- La règle ne regarde que les **labels** de l'espace CPU. Une constante
  `[definitions]` (`_sizeof_k.main`) ne vaut jamais pour un nom nu.
- Elle s'applique à tout label pointé, pas seulement aux `static` C : vos
  `.sym` portent aussi `rand_seed.console`, `fill_pass.dma`, etc. `rand_seed`
  se résout donc s'il n'a qu'un candidat. Dites-le si cela vous gêne.
- `--budget <symbole>` de `luna profile` n'est pas concerné : il compare
  un nom à ceux du rapport, il ne résout pas d'adresse. Une fonction
  `static` s'y écrit encore avec son suffixe.

Non rejoué chez nous : votre commande `opensnes init demo --template game`
(nous n'avons pas construit le gabarit). C'est le test à faire chez vous.

## 2. D2 — les sommes des archives : déjà servies par GitHub

Nous ne rajoutons pas de fichier `SHA256SUMS` : la page d'une release porte
quatre zips, comme la vôtre, et c'est une décision du mainteneur. Mais la
somme que vous voulez existe, calculée par GitHub à l'envoi de chaque
fichier, et servie par l'API :

```
gh api repos/k0b3n4irb/luna/releases/tags/v1.33.1 --jq '.assets[] | "\(.digest)  \(.name)"'
sha256:863d6493884c38ad5fcd1422f4e4cf979d945152cb5bcd020057d053804cd028  luna_v1.33.1_darwin_arm64.zip
sha256:eba331a8dfb8a6bdf3d42d50780acb329eb04cd366936632d8e43d237c605ea3  luna_v1.33.1_linux_arm64.zip
sha256:aac5bba6eeb95176deb817f53db1de59a83a85c52c324dbdad298089ff5b39af  luna_v1.33.1_linux_x86_64.zip
sha256:753fa41149620eacaf474dd3b73a03196e53179ae691abcb090a5cba3f3e49ee  luna_v1.33.1_windows_x86_64.zip
```

Ce sont, octet pour octet, les quatre sommes que vous avez relevées. Votre
installateur peut donc comparer l'archive téléchargée au champ `digest`,
obtenu par un autre canal que le téléchargement lui-même. Si ce champ ne
vous suffit pas (hors ligne, miroir), dites pourquoi : la question
remontera au mainteneur avec votre raison.

Sur la ligne dans les notes quand un nom d'actif change : d'accord, et
c'est désormais notre règle (une note chez vous le jour même). Le 04-10
elle n'existait pas ; le 404 était de notre fait.

## 3. D3 — la RAM de cartouche sous `--power-on` : fait (`432b075`)

Par défaut, sans nouvelle option : `--power-on random` et `ones` remplissent
aussi la RAM de cartouche **qu'aucune pile ne garde**. Vos deux mesures,
graine 1, trame 0 :

```
superfx_3d   $700000  8E 84 DB 22 1D 73 AC 2D A6 11 DA B0 B5 B9 2A AB   (était 00 …)
             $7F8000  A4 93 96 D7 5A E9 33 23 3D F2 68 6A C9 7F 55 94   (témoin, inchangé)
sa1_hello    $003000  8E 84 DB 22 1D 73 AC 2D A6 11 DA B0 B5 B9 2A AB   (était 00 …)
             $400000  25 C5 BD 9C 0D F5 87 98 B3 1A A5 9D 2B B2 AC EB   (était 00 …)
             $7F8000  A4 93 96 D7 5A E9 33 23 3D F2 68 6A C9 7F 55 94   (témoin, inchangé)
```

Ce qui est rempli, et ce qui ne l'est pas :

| RAM | Remplie quand |
|---|---|
| RAM du Game Pak (Super FX) | l'en-tête ne déclare pas de pile |
| BW-RAM (SA-1) | l'en-tête ne déclare pas de pile |
| I-RAM (SA-1) | toujours (aucun fichier de sauvegarde ne la porte) |
| RAM de sauvegarde LoROM / HiROM / S-DD1 / DSP-1 | l'en-tête ne déclare pas de pile (`$FFD6` = `ROM+RAM`) |
| RAM gardée par pile | jamais : zéro, ou ce que dit le `.srm` |

« Pile » = le quartet bas de `$FFD6` vaut 2, 5, 6, 9 ou A.

**Ce qui bouge chez vous.** La cartouche est tirée en dernier du
générateur : pour une graine donnée, WRAM, VRAM, CGRAM, OAM, RAM de l'APU
et registres sont ceux d'avant. Une ROM sans RAM de cartouche volatile
rend donc les mêmes images. Vos ROM `chips/` sous `--power-on random=1`
peuvent, elles, changer d'image si elles lisent cette RAM avant de
l'écrire : c'est la classe de défaut visée. Mesuré : vos huit ROM
`examples/chips/*` à la trame 300 sous `random=1` rendent le même `fbhash`
avant et après (aucune ne lit donc cette RAM avant de l'écrire, à ce
point de l'exécution) ; et huit titres du commerce (trois SA-1, quatre
Super FX, un simple), sous `zero` et `random=1` : seize images identiques.
Nous n'avons pas joué vos manifestes ni vos 78 autres ROM.

**La conséquence non demandée : le reset et l'I-RAM.** luna vidait l'I-RAM
à chaque reset, comme ares à la mise sous tension (`sa1.cpp:139`), et le
reset de chargement effaçait donc le remplissage. Nous avons retiré ce
vidage : l'I-RAM est de la RAM, le manuel SA-1 propose une pile pour elle
(livre 2, §1.2) et Mesen2 ne la vide jamais (`Sa1.cpp:36`). Avec `zero`,
rien ne change à la mise sous tension. Ce qui change : après un **reset en
cours d'exécution**, l'I-RAM garde son contenu au lieu de revenir à zéro.
Si un de vos tests fait un reset sur une ROM SA-1 et compte sur une I-RAM
propre, il le verra.

## 4. Vos observations

- **« Latest » sur la v1.33.0.** Exact : la republication du 04-10 avait
  daté la v1.33.0 après la v1.33.1. Corrigé le 10-06 ; l'étiquette est
  aujourd'hui sur la v1.34.0.
- **MS0 + 21 MHz, LoROM DSP-1 de 2 Mo.** Notés, nous attendons vos ROM.
- **Mosaïque.** Merci pour `transition_mosaic_picture.toml` : c'est le
  second oracle que nous proposions le 10-05. Nous retenons
  `9980548a31063b25`.

## 5. Ce que nous vous demandons

Épingler la `v1.34.0`, rejouer votre suite, et nous dire : (1) D1 sur le gabarit `game` ; (2) si une ROM
`chips/` change d'image ou d'état sous `random=1` ailleurs qu'à la trame
300, et si c'est un défaut chez vous ou chez nous ; (3) tout test qui
passe au rouge.

## 6. La v1.34.0

Quatre zips, mêmes noms (`luna_v1.34.0_<os>_<arch>.zip`). Leurs sommes,
telles que GitHub les sert (§2) :

```
f7aebc7abfdeb999a536508b5b265476f150ae6d19f98fa6763f3fc88f4fe939  luna_v1.34.0_darwin_arm64.zip
5ebed59bc61796b80e821ba9641eb5b3c4bc44479e89c9f0056938f014959470  luna_v1.34.0_linux_arm64.zip
fb1a5dbc28c5d5f03bd4c8fbc75cc08f7067dd0d9eb9f25f3103d9c27f5ee785  luna_v1.34.0_linux_x86_64.zip
e043162ce931783a514dc431993537052f993cd5459106956a58d4a08f100965  luna_v1.34.0_windows_x86_64.zip
```

Nous avons téléchargé l'archive `linux_arm64` (somme vérifiée) et rejoué
dessus vos deux contrôles : `k` ambigu sur `static_dup`, I-RAM remplie sur
`sa1_hello`. Le reste de la CLI est celui de la 1.33.1.

**Ménage :** la `v1.30.4` n'a plus de page ni de binaires (les cinq
versions les plus hautes en gardent : `v1.31.0` à `v1.34.0`). Votre
épingle, `v1.33.1`, n'est pas touchée. Le tag `v1.30.4` reste.

## 7. Ouvert

Votre retour Windows sur la GUI de la v1.33.1 (note du 10-05).
