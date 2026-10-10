# OpenSNES → luna : un bloc de mémoire affirmé à un `[[checkpoint]]`

| | |
|---|---|
| **De** | OpenSNES (`develop` @ `4ae15903` poussé ; luna épinglée : `v1.37.0`) |
| **Date** | 2026-10-10, nuit |
| **Statut** | à transmettre |
| **En bref** | Une demande, simple : pouvoir écrire `[checkpoint.blocks]`. Elle vient de la conversion de nos manifestes indexés par trame vers `at_symbol` / `input_at`, commencée ce soir avec votre v1.37.0. |

## 1. Ce que votre v1.37.0 a rendu possible ce soir

Deux manifestes qui cassaient à chaque fois qu'une étape du compilateur
raccourcissait le démarrage sont maintenant cadencés par le programme, sur
la N-ième arrivée à `WaitForVBlank` (que la boucle principale appelle une
fois par tour, et le démarrage jamais) :

- `testing/manifests/movement_sprite_swarm.toml` : trois points de
  contrôle `at_symbol = "WaitForVBlank"`, `hit = 55 / 155 / 295`. Le
  compteur de tours vaut alors N par construction. La conversion a montré
  un défaut de l'ancienne version : à une frontière de trame, elle lisait
  des positions déjà avancées à côté d'un compteur pas encore incrémenté —
  un état à cheval sur deux tours. Votre message d'échec
  (`checkpoint@WaitForVBlank#55 (frame 58, line 30)`) donne la trame et la
  ligne de l'arrivée : c'est ce qui nous l'a fait comprendre.
- `testing/manifests/backgrounds_mode4.toml` : `input_at =
  "WaitForVBlank"`, l'appui indexé par les mêmes arrivées.

Ce que nous n'avions pas vu dans la doc avant de buter dessus, pour qui
passera après nous : un manifeste à `at_symbol` exige `frames` (l'horizon),
et les `[asserts]` finaux se lisent à cet horizon, pas à la dernière
arrivée. Le message d'erreur le dit très bien.

## 2. La demande : `[checkpoint.blocks]`

Nature : champ de manifeste. Priorité : basse (contournée). Coût estimé de
votre côté : la table existe déjà pour `[asserts.blocks]`.

Ce que nous avons écrit, dans `backgrounds_mode4.toml` :

```toml
[[checkpoint]]
at_symbol = "WaitForVBlank"
hit = 125
[checkpoint.blocks]
opt_row_h_words = { space = "vram", offset = "6000", hex = "0820082010201020" }
```

Réponse de luna v1.37.0 :

```
TOML parse error at line 45, column 13
unknown field `blocks`, expected one of `at_frame`, `at_symbol`, `hit`,
`input`, `input2`, `mouse`, `superscope`, `values`, `ppu`, `gsu`, `delta`
```

Pourquoi `[asserts.blocks]` ne convient pas ici : il se lit à l'horizon
`frames`, c'est-à-dire à une trame, et le contenu de cette rangée de VRAM
change à chaque tour de boucle. C'est exactement la dépendance au temps de
calcul que `at_symbol` nous a servi à ôter.

Ce que nous avons fait à la place : affirmer les quatre premiers mots du
tampon en RAM avec `[checkpoint.values]` (`"owords.main" = { eq = 0x2018,
width = 2 }`, etc.). Cela vérifie le calcul, pas le chemin jusqu'à la VRAM.

Contrat que nous utiliserions : dans un `[[checkpoint]]`, la même table
que `[asserts.blocks]` (`space`, `offset`, `hex`), lue à l'arrivée comme
`values` et `ppu`. `luna test --update` la réécrirait comme il réécrit un
bloc final.

## 3. Observation, sans demande

`[asserts.values]` à l'horizon reste utile à côté de points de contrôle
sur une routine : dans `movement_sprite_swarm.toml`, il porte la seule
affirmation qui parle de temps (« à la trame 300, le compteur de tours est
entre 290 et 299 » : aucune trame perdue), pendant que les points de
contrôle parlent de logique. Les deux horloges dans un même manifeste,
chacune pour ce qu'elle sait dire.

## Dû par nous

Les deux ROM de reproduction ; et quatre manifestes indexés par trame
restent à convertir (`backgrounds_mode6`, `mode7_extbg`,
`dma_mode2_opt_table`, `movement_fix32_orbit`).
