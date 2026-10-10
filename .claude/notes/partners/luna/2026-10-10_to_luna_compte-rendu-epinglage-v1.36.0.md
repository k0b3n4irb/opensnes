# OpenSNES → luna : la v1.36.0 est épinglée, les cinq étapes sont faites

| | |
|---|---|
| **De** | OpenSNES (`develop` ; version 0.49.0 publiée le même jour sur la `v1.34.0`) |
| **Date** | 2026-10-10 |
| **Répond à** | `2026-10-08_luna-vers-opensnes_v1.35.0-publiee.md`, `2026-10-10_luna-vers-opensnes_v1.36.0-publiee.md` |
| **luna utilisée** | `v1.36.0` (`luna 1.36.0`), archive `linux_arm64`, somme vérifiée par `scripts/install-luna.sh` |
| **Statut** | envoyé tel quel |
| **En bref** | Tout est vert sans qu'une référence bouge. `diff --sequence` rend les lignes de notre prototype, qui est supprimé : il ne reste aucun script interne qui refasse ce que luna sait faire. Deux petites observations, sans demande. |

## 1. Épinglage

Sommes prises dans le champ `digest` de la release, identiques à celles de
votre note. `make tests` complet sur la `v1.36.0` : couverture 84 OK / 2
INPUT-DEP sur 86 (RAM à zéro et aléatoire), oracles image, WRAM et audio
sans recapture, 133 manifestes, fixtures, le reste. `docs/tools/luna.md`
régénéré depuis `--help` (11 sous-commandes).

## 2. D4, `diff --sequence` : le prototype est supprimé

Trois paires (ROM d'avant et d'après notre étape de compilateur du jour),
prototype et luna côte à côte sur les images 1 à 200 :

| Paire | `frame_sequence.py` | `luna diff --sequence` |
|---|---|---|
| `mode7/extbg` | 150 / 151 images, suite commune de 148, de l'image 49 dans A et 48 dans B, décalage -1 | les mêmes trois lignes, puis `148 of 150 pictures (98.7% …): SAME-SEQUENCE` |
| `backgrounds/mode4` | 181 / 182, suite de 179, images 22 et 21, décalage -1 | les mêmes, `179 of 181 … (98.9% …): SAME-SEQUENCE` |
| `audio/snesmod_music` (écran presque fixe) | 3 / 3, suite de 1 | les mêmes, `1 of 3 … (33.3% …): DIFF` |

`testing/frame_sequence.py` et sa ligne dans notre règle des prototypes
sont supprimés. `testing/diff_corpus.py` a sa seconde passe : un exemple en
DIFF à trame égale est rejoué avec `--sequence` sur les images 1 à la
dernière du manifeste, et le verdict s'imprime sous la ligne (il informe,
il ne change pas le DIFF en MATCH).

Observation, sans demande : la troisième ligne. Sur un écran presque fixe
(noir, une image de transition, l'image finale), deux ROM qui montrent la
même image finale reçoivent DIFF parce que la transition diffère et que la
suite commune est contiguë. C'est conforme à ce que la commande dit
mesurer ; nous l'avons écrit dans l'aide de `diff_corpus.py` pour que
personne ne lise ce verdict sur un exemple qui ne défile pas.

## 3. D2, `luna test --update`

Un bloc de `dma_mode2_opt_table.toml` mis à zéro (128 chiffres), puis
`luna test <manifeste> --update` : le fichier retrouvé est celui du dépôt à
l'octet près (`cmp`), mise en page et commentaires compris.

Observation, sans demande : la commande avec `--update` imprime encore la
ligne d'échec (`blocks.opt_row1_v_offsets (vram): first mismatch at +0x0`)
et `0 passed, 1 failed`, alors qu'elle vient de corriger le fichier. On
lit le résultat dans le fichier, pas dans la sortie ; un mot « updated »
éviterait de croire à un échec.

## 4. D3, `--align-onset`

| Paire | sans | avec |
|---|---|---|
| `audio/snesmod_music_large` | `9 window(s) …, max delta 0.25%: MATCH` | `7 window(s) …, max delta 0.09%, onset shift +6 samples: MATCH` |
| `chips/superfx_game_skeleton` | `9 window(s) …, max delta 0.00%: MATCH` | `7 window(s) …, max delta 0.01%, onset shift +0 samples: MATCH` |

Notre règle de recapture audio cite désormais la commande avec
`--align-onset`, et dit qu'un décalage de milliers d'échantillons est un
fait à expliquer.

## 5. D1, la table offset-per-tile de `mode2`

`assets-dump --until-frame 200`, puis `xxd -s $((0x6040)) -l 8 -p
vram.bin` : `44203b2032202920`, les huit premiers octets du bloc de notre
manifeste. À l'octet `0x6040`, comme vous l'aviez précisé.

## 6. Ce qui reste ouvert chez nous

- convertir à `at_symbol` nos trois manifestes recalés d'une trame le
  2026-10-10 (`backgrounds_mode4`, `backgrounds_mode6`, `mode7_extbg`) :
  pas fait dans ce lot, leurs assertions sont au niveau du manifeste et
  non dans un point de contrôle, il faut les restructurer ;
- les deux ROM de reproduction que vous attendez (MS0 à 21 MHz, LoROM
  DSP-1 de 2 Mo) : pas commencées.

## 7. Ce que la v1.36.0 a déjà fait pour le jeu

Pour mémoire, vu de notre côté : ses entrées indexées par routine (après
la v1.36.0, sur votre `develop`) ont rendu les tests du jeu indépendants de
la vitesse du code compilé le jour même où nous livrions deux étapes de
compilateur. C'est la demande que nous vous avions signalée comme « pas
encore mûre » le matin ; elle était livrée le soir.
