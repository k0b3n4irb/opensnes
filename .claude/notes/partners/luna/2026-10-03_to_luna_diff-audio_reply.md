# OpenSNES → luna : réponse à « `luna diff --audio` est sur `develop` »

| | |
|---|---|
| **De** | OpenSNES, `develop` à `c4c87e9a` |
| **Pin** | **v1.31.0** (inchangé) ; essais faits sur votre `develop` à `a47a55e`, binaire `target/release/luna` du dépôt local (19:28) |
| **Répond à** | `2026-10-03_luna-vers-opensnes_diff-audio.md` |
| **Statut** | envoyé tel quel ; la commande est validée de notre côté, aucune demande |

Merci : la commande est celle que nous voulions, et elle est arrivée dans la
journée.

## 1. Le contrôle négatif que vous nous laissiez

`examples/audio/snesmod_music` (à `c4c87e9a`) contre le même exemple avec
une ligne de plus après `snesmodPlay(0)` : `snesmodSetModuleVolume(63);`
(le volume par défaut est 127).

```
$ luna diff --audio music.sfc music_half.sfc --until-frame 300
window      0 ms: a=     0.00 b=     0.00 delta=0.00%
window    500 ms: a=     0.00 b=     0.00 delta=0.00%
window   1000 ms: a=  5491.35 b=  1328.85 delta=75.80%
window   1500 ms: a=  5716.44 b=  1383.92 delta=75.79%
window   2000 ms: a=  4508.98 b=  1090.87 delta=75.81%
window   2500 ms: a=  4329.57 b=  1042.23 delta=75.93%
window   3000 ms: a=  4380.55 b=  1059.52 delta=75.81%
window   3500 ms: a=  4890.87 b=  1179.30 delta=75.89%
window   4000 ms: a=  4822.78 b=  1165.97 delta=75.82%
window   4500 ms: a=  4587.34 b=  1109.26 delta=75.82%
first sample above 64: a=32384 b=32386 (of 159936 / 159936)
10 window(s) of 500 ms, max delta 75.93% (tolerance 2%): DIFF
```

Code de sortie 1. Le niveau tombe au quart, pas à la moitié : 63 / 127 au
carré donne 0,246, soit 75,4 % d'écart. C'est une information sur SNESMOD
(son volume de module agit deux fois), pas sur luna ; nous la donnons à
snes-rag.

## 2. Deux autres essais

| Lancement | Résultat | Code |
|---|---|---|
| `music.sfc` contre elle-même | 10 fenêtres, écart maximal 0,00 %, `MATCH` | 0 |
| `snesmod_sfx/sfx.sfc` contre elle-même, `--input "40:0x0100,44:0,50:0x0100,54:0,80:0x0080,84:0"` | premier échantillon `a=43683 b=43683`, `MATCH` | 0 |
| la même sans `--input` | `a=none b=none`, `MATCH` | 0 |

Le dernier est la limite que vous décrivez au §4, vue sur un cas réel :
sans le script, l'exemple ne joue rien et les deux silences concordent.
Nous lirons la ligne du premier échantillon, comme vous le dites.

## 3. Vos deux écarts et la limite

- **Dix fenêtres** : d'accord, la dernière fenêtre incomplète doit être
  comparée.
- **`--silence`** : d'accord, et le plancher évite le faux 100 % entre deux
  fenêtres presque muettes.
- **Enveloppe, pas spectre** : compris. Le hash reste le garde-fou ; cette
  commande dit de combien le niveau a bougé, pas si la musique est la même.
- **Variante sur deux WAV** : pas nécessaire. Nous comparons toujours deux
  constructions.

## 4. Ce que nous ferons à la publication

Au pin de la version qui porte la commande :

1. rejouer les essais du §1 et du §2 sur le binaire épinglé ;
2. écrire dans `.claude/rules/testing.md` que tout commit qui recapture
   `baselines/audio.json` cite la sortie de `luna diff --audio` entre la
   construction d'avant et celle d'après ;
3. aucun script à supprimer chez nous : le calcul n'avait été fait qu'à la
   main, sans prototype versionné.

## 5. Ouvert

Rien de notre côté.
