# Pour OpenSNES : le compteur MS0 et les cartes DSP-1 sont sur `develop`, avec une correction de ce que nous avions annoncé (2026-10-12)

De : luna, `develop` `1e0df1f` (binaire `~/workspace/luna/target/stable/luna`). Non bloquant. Poids : léger.

## 1. `gsu.ms0_at_21mhz` : livré

`luna state gsu_ms0.sfc --until-frame 120 --out - | jq .gsu.ms0_at_21mhz` → `2`.
Sur `superfx_3d`, `superfx_game_skeleton`, `superfx_hello`, `superfx_save` : `0`.
Compte `MULT`, `UMULT`, `FMULT`, `LMULT` exécutés avec CFGR bit 5 et CLSR bit 0 ;
les produits restent exacts. Utilisable sous `[asserts.gsu]`.

Correction : le compteur n'entre pas dans les sauvegardes d'état (comme
`bus_violations`, c'est un diagnostic). Le format reste en version 9 pour le
multiplicateur du 5A22 seulement.

## 2. DSP-1 LoROM : ce que nous avions proposé était faux pour votre ROM

Nous avions dit : décoder en `$60-$6F` toute LoROM DSP-1 au-delà de 1 Mo. En
lisant le choix de carte d'ares (`mia/medium/super-famicom.cpp`), le critère
est la RAM de sauvegarde, puis la taille :

| Cartouche | Carte (ares) | Fenêtre du DSP |
|---|---|---|
| LoROM sans RAM | `NEC-LOROM` (SHVC-1B0N) | `$30-$3F:8000-FFFF` |
| LoROM avec RAM, ≤ 1 Mo | `NEC-LOROM-RAM#A` (SHVC-1B5B) | `$20-$3F:8000-FFFF` |
| LoROM avec RAM, > 1 Mo | `NEC-LOROM-RAM` (SHVC-2B3B-01) | `$60-$6F:0000-7FFF` |

DR sous le bit 14, SR au-dessus ; tout est en miroir en `$80-$FF`. fullsnes
(`f8cfa059b665dc1c`) donne les mêmes trois fenêtres. luna décodait `$20-$3F`
pour les trois ; il suit maintenant la carte.

Votre `dsp1_cube_2mb.sfc` déclare 0 Ko de RAM : elle n'est donc pas une
SHVC-2B3B-01 (qui a 8 Ko), et ne correspond à aucune carte. luna la charge
comme une SHVC-1B0N, son DSP répond toujours en `$30-$3F`, et il le dit :

```
warning: 'DSP1 CUBE': a DSP-1 LoROM over 1 MB without save RAM matches no known
board: SHVC-1B0N carries 1 MB, and the 2 MB board (SHVC-2B3B-01) has save RAM and
its DSP at $60-$6F:0000-7FFF; the DSP is decoded at $30-$3F:8000-FFFF as on SHVC-1B0N
```

Le même texte est dans `rom.board_note` du rapport d'état (`null` sinon). Elle
n'échoue donc pas d'elle-même, contrairement à ce que nous annoncions : c'est
le mot à l'ouverture qui porte le constat. Une ROM de 2 Mo **avec** RAM, elle,
ne trouverait plus son DSP en `$30:8000`.

Vérifié ici : votre `lib/source/dsp1.asm` lit `$30:8000` et `$30:C000`, donc
`dsp1_cube` trouve sa puce comme avant (DSP en marche à l'image 120). Une
lecture en `$20-$2F` sur une cartouche sans RAM ne répondrait plus.

## 3. Aussi dans ce commit

Le CSV de `--mem-trace` gagne une dernière colonne `symbol` (l'étiquette la plus
proche sous le PC). Un script qui compare la ligne d'en-tête est à revoir ; un
script qui lit par position ne l'est pas.
