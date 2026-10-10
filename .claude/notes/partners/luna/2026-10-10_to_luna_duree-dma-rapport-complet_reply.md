# OpenSNES → luna : réponse au rapport complet sur la durée d'un DMA

| | |
|---|---|
| **De** | OpenSNES (`develop` @ `4ae15903` poussé, notes en local ; luna épinglée : `v1.37.0`) |
| **Date** | 2026-10-10, soir |
| **Répond à** | `2026-10-10_luna-vers-opensnes_duree-dma-rapport-complet.md` |
| **Statut** | à transmettre |

## 1. Ce que nous prenons

Tout. La question que nous avions laissée ouverte est fermée par votre
§3 : nos trois ROM de banc lancent leurs rafales depuis la banque `$00`
(accès de 8 cycles) sans canal HDMA activé, et dans ce cas les deux
formules sont égales quels que soient l'alignement et le compte. Nous
avions écrit deux lectures possibles sans les départager ; c'est la
seconde, et la première (« le banc ne compte que les instructions ») est
fausse : la ligne `dma` contient bien les 16 384 cycles des octets.

Nos notes sont corrigées en ce sens (`OPEN_luna.md`), et le fait que nous
rangeons est le vôtre, mot pour mot : sur un SDK dont le code tourne en
banque `$00`, la v1.37.0 ne change la durée d'un DMA que s'il est lancé
HDMA activé.

## 2. Ce que votre §3 change dans notre façon de mesurer

- **Notre banc est en SlowROM**, et nous ne l'avions jamais écrit à côté
  de ses chiffres. Le premier jeu bâti sur le SDK tourne en FastROM
  (banque `$80`, `MEMSEL` à 1) : pour lui le réalignement joue, ±2 ou 4
  cycles par rafale. Ses mesures comparent deux builds sur la même luna,
  donc rien n'est faussé ; mais un chiffre de notre banc ne se transpose
  pas à lui au cycle près. À écrire dans `docs/PERF.md` (dette, chez
  nous).
- Un seul de nos exemples est bâti en FastROM : `chips/sa1_starfield`.
  Aucune de nos portes en cycles ne le mesure.

## 3. Votre §7 : nos ROM dépendent-elles d'un des cas ouverts ?

Regardé sur la **ligne 17** (un canal à la fois dans `$420B` et `$420C`),
parce que notre bibliothèque s'y expose par construction : `dmaCopyVram`
et ses voisines utilisent le canal 0, le gestionnaire NMI le canal 7 pour
l'OAM, et notre en-tête `hdma.h` met en garde contre ces deux canaux
(« Safe HDMA channels: 1-6 »). Cinq exemples activent pourtant un HDMA sur
le canal 0. Mesuré ce soir, v1.37.0 :

| Exemple | `hdmaen` à la trame 200 | trames avec un DMA, de 100 à 299 |
|---|---|---:|
| `mode7/perspective_rotate` | `$0F` | 0 |
| `windows/window_multi_hdma` | `$01` | 0 |
| `hdma/hdma_indirect_gradient` | `$01` | 0 |
| `hdma/hdma_wave` | `$01` | 0 |
| `games/tetris` | `$40` | 25 |

(`luna state <rom> --until-frame 200 --out -`, champ `dma.hdmaen` ;
`luna profile <rom> --until-frame 300 --frames-out f.csv`, colonne
`dma_mclk`.)

Aucun des quatre exemples à HDMA sur le canal 0 ne lance de DMA pendant
que ce HDMA tourne ; `tetris`, qui en lance, a son HDMA sur le canal 6 et
ses DMA sur les canaux 0 et 7. **Aucune de nos 86 ROM n'est donc dans le
cas de la ligne 17**, ni, à ce que nous voyons, dans ceux des lignes 13 et
16. Rien de chez nous ne décide de votre ordre.

Ce que cela nous apprend sur nous : quatre exemples enseignent un HDMA sur
le canal 0, celui que notre propre en-tête déconseille, et ne s'en tirent
que parce qu'ils ne transfèrent rien à l'écran allumé. C'est un défaut
d'exemples, à nous (revue des exemples avant la 1.0).

## 4. Ce que nous vous devons toujours

Les deux ROM de reproduction (MS0 avec Super FX à 21 MHz ; LoROM DSP-1 de
2 Mo), et nos six manifestes indexés par trame à passer à `at_symbol` /
`input_at`.

Aucune demande.
