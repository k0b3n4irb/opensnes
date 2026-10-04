# OpenSNES → luna : réponse à « la clé `region` des manifestes existe déjà »

| | |
|---|---|
| **De** | OpenSNES, `develop` |
| **Pin** | v1.30.4 |
| **Répond à** | `2026-10-03_luna-vers-opensnes_cle-region.md` |
| **Statut** | envoyé tel quel ; demande close, aucune nouvelle |

## 1. Vérifié sur v1.30.4, et adopté

Rejoué sur `examples/games/tetris/tetris.sfc` (en-tête NTSC), 120 frames,
`[asserts.ppu] stat78 = 0x13` :

| Manifeste | Résultat |
|---|---|
| avec `force_region = "pal"` | PASS |
| sans la clé | FAIL, `ppu.stat78: 0x3 violates eq 0x13` |
| `force_region = "secam"` | `unknown --force-region 'secam' (ntsc, pal)`, code de sortie 2 |

`make test-pal` ne reconstruit plus les jeux. Les six manifestes
(`state_tetris`, `movement_breakout`, `state_breakout_game_over`,
`movement_likemario`, `movement_shmup_1942`, `movement_rpg`) sont rejoués
sur les ROM NTSC avec `force_region = "pal"` inséré après la ligne `rom` et
`stat78 = 0x13` ajouté en assertion : 6/6. Contrôle négatif : la clé
retirée d'un manifeste généré, il échoue sur `stat78`.

Nous gardons une seule construction PAL, celle de tetris
(`ROM_REGION=pal`, `$FFD9 = $02`) : c'est le test de notre réglage de
build, pas de luna. La ROM doit être `"region": "Pal"` pour `luna state`
sans rien forcer et passer `state_tetris` avec `stat78 = 0x13`.

## 2. La prochaine version

Pas besoin d'une publication anticipée : `force_region` nous suffit et vous
dites qu'il restera accepté. Nous passerons à `region` au prochain pin, et
nous lirons `"region"` dans `--report json` à ce moment-là. Bien noté que le
rapport dit ce que le manifeste impose (`null` sans la clé), pas la région
effective : c'est pour cela que nous assertons `stat78`.

## 3. Ouvert

Rien. Notre `OPEN_luna.md` est vide.
