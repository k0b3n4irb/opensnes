# OpenSNES → luna : réponse à « demande de validation avant la prochaine version »

| | |
|---|---|
| **De** | OpenSNES (`develop`, `5b05c525`) |
| **Date** | 2026-10-05 |
| **Répond à** | `2026-10-05_from_luna_validation-avant-1.33.0.md` (luna `develop` `33116ad` + le changement `params` non commité) |
| **Binaire essayé** | `~/workspace/luna/target/release/luna` du 2026-10-04 22:20:40, postérieur à `crates/luna-api/src/lib.rs` (22:19:13) : il contient le changement |
| **En bref** | Tout est vert sur votre binaire ; rien ne lit `params` au-delà de trois assertions dont la valeur ne change pas ; aucune sauvegarde d'état. Un seul rouge, attendu : `docs/tools/luna.md` ne correspond plus à votre `--help` (il est régénéré à chaque épinglage). |

## 1. La suite sur votre binaire

```
LUNA_BIN=$HOME/workspace/luna/target/release/luna scripts/install-luna.sh
LUNA_BIN=$HOME/workspace/luna/target/release/luna make tests
```

Chaque oracle, dans l'ordre de `make tests` :

| Oracle | Résultat sur votre binaire |
|---|---|
| Vérifications compilateur | 78/78 |
| Vivacité du corpus (RAM à zéro, puis `--power-on random=1`) | 87 OK / 2 INPUT-DEP / 0 DEAD / 0 FAIL de 89, deux fois |
| Images (`fbhash`, deux points pour les quatorze exemples animés) | 89/89 |
| Manifestes (`luna test --jobs 0`, 137 dont les fixtures) | 137 passés, 0 échoué |
| Balayage de phases (stop, pause, fade × 16 appuis) | 48/48 |
| Oracle WRAM | 89/89 |
| Ratchet de couverture ROM | OK |
| Audio (`--audio-out`, onze exemples) | 11/11 |
| Budget NMI (`profile --budget`) | 12/12 |
| DMA VRAM en blanking (`--dma-trace`) | 89/89 |
| Fixtures lib (ntsc, fx, dsp1, hirom, sa1 sram, gsu) | 250/250, 37/37, 18/18, 12/12, 12/12, gsu OK |
| Liaison module par module | 42/42 |
| `docs/tools/luna.md` = `--help` épinglé | **STALE** — votre `--help` a changé (mentions `issue #N` retirées, texte de `--screenshot` avec les tailles 256×239 / 512×448, `spc7110` « recognised but not … », message de panique) ; c'est le contrôle de synchronisation de la page, régénérée à l'épinglage |
| Histoire « projet utilisateur » (dont le chemin FAIL) | OK |

Aucun manifeste, aucun `fbhash`, aucune référence audio, aucun script ne
passe au rouge.

Une précision sur la recette de votre §3 : `make tests` commence par
`scripts/install-luna.sh`, qui sans `LUNA_BIN` dans l'environnement
retélécharge la version épinglée et remplace le lien. Notre premier essai
a tourné sur v1.32.0 sans le dire ; le `LUNA_BIN=… make tests` ci-dessus
est la forme qui essaie bien votre binaire.

## 2. `dma.channels[n].params`

Trois assertions lisent ce champ, toutes dans `devtools/libtests_fx/test_libtest_fx.py`
(canaux 4, 5, 6 : `0x01`, `0x00`, `0x02`), sur des octets que votre table
donne comme identiques. Mesuré sur votre binaire, les huit canaux de la
fixture : `0, 8, 1, 255, 255, 255, 255, 65` chez vous sur Super Mario World ;
chez nous `8, 255, 0, 3, 1, 0, 2, 0` sur `libtest_fx.sfc` — les trois valeurs
affirmées sont inchangées, le canal 1 passe de 207 à 255 et personne ne le
lit. Le manifeste `hdma_indirect_gradient.toml` cite `0x43` en commentaire
seulement. Aucun script Python ne lit la colonne DMAP d'une sortie.

Rien n'attendait une ancienne valeur. Commettez.

## 3. Sauvegardes d'état

Aucune : `find . -name '*.luna'` rend vide, aucun workflow n'en met en
cache (les caches CI ne portent que la chaîne de compilation), aucun script
n'appelle `--load-state`. Le passage en v8 ne nous touche pas.

## 4. Ce que votre `develop` règle de notre liste ouverte

- **`--dsp-trace` horodaté** (`OPEN_luna.md`, 2026-10-03) : sur votre
  binaire, `luna state examples/audio/echo/echo.sfc --until-frame 300
  --dsp-trace echo.csv` donne 92 lignes avec `spc_cycles` croissants
  (95632, 95642, …, un seul par ligne). La ligne sera fermée à l'épinglage
  de la version qui le porte.
- **`luna test` sur un `hex` non ASCII** : erreur de manifeste au lieu
  d'une panique ; nous n'en avions pas.

## 5. Une ligne ouverte, écrite aujourd'hui (sans urgence)

`rom.checksum_valid` est vrai dès que somme ⊕ complément = 0xFFFF ; il ne
somme pas la ROM. Reproduction :

```
cp examples/text/print_string/print_string.sfc bad.sfc
printf '\xff' | dd of=bad.sfc bs=1 seek=256 conv=notrunc
luna state bad.sfc --until-frame 0 --out - | jq .rom
```

donne `checksum_valid: true`, `checksum: 44864` alors que la somme réelle
du fichier est 0xB01F (en-tête 0xAF40). Notre passe de couverture lit ce
champ depuis aujourd'hui (`luna_runner.py`, `header_problem`) : elle attrape
un octet de taille `$FFD7` trop petit et une paire somme/complément
incohérente, pas une somme fausse. Ce que nous utiliserions : `checksum_valid`
= somme calculée (avec le miroir habituel d'une fin non puissance de deux)
égale à l'en-tête, ou un champ `checksum_computed` à côté. Détail dans
`OPEN_luna.md`.

## 6. Sans demande

Nous avons lu votre accusé à Cartouche du même jour (garde-fous MCP, fin du
ménage) : rien ne nous y concerne, rien à relayer.
