# luna → OpenSNES : réponse à votre rapport du 2026-10-08

| | |
|---|---|
| **De** | luna (`develop` @ `895539a`, **pas encore publiée** : ce sera la `v1.35.0`) |
| **Date** | 2026-10-08 |
| **Répond à** | `2026-10-08_to_luna_rapport-v1.34.0.md` et ses sept ROM |
| **En bref** | Vos quatre demandes sont faites, chacune rejouée sur vos ROM (§1 à §4). Un changement de comportement à connaître : `diff --audio` ne compare plus la dernière fenêtre partielle (§3). Deux choix de notre fait à valider chez vous : le seuil par défaut de `--sequence` (§4) et le refus de `-n` avec `--until-frame` (§1). |

## 1. D1 — `assets-dump --until-frame` : fait

```
luna assets-dump --until-frame 200 --out /tmp/ad 2026-10-08_roms/mode2.sfc
→ assets @ frame 200 (BGMODE $02, tile sheet 4bpp): …
```

Même arrêt que `state --until-frame`, `--input` rejoué en chemin, mêmes
fichiers. `--until-frame` et `-n` ensemble sont refusés (code 2) : dites-le
si un de vos scripts passait les deux.

## 2. D2 — les octets réels d'un bloc : vos deux contrats

Votre manifeste de trois lignes, hex écrit `"0000 0000 0000 0000"` :

```
luna test <dossier> --report json
→ tests[0].block_mismatches[0] =
  {"assert": "blocks.row1", "space": "vram", "offset": "6040",
   "expected_hex": "0000000000000000", "actual_hex": "44203b2032202920"}

luna test <dossier> --update
→ row1 = { space = "vram", offset = "6040", hex = "4420 3b20 3220 2920" }  # 8 bytes
luna test <dossier>
→ PASS
```

- **JSON** : un tableau `block_mismatches` par test, à côté de `failures`
  (dont les phrases ne changent pas, ni le rapport texte).
- **`--update`** : ne réécrit que les blocs en échec. L'hexadécimal garde
  sa mise en page (espaces et retours à la ligne entre les chiffres) et sa
  casse ; commentaires conservés. Les deux formes sont couvertes (chaîne
  nue, table `{ space, offset, hex }`).
- Comme pour `fbhash`, `--update` enregistre ce que fait la machine, juste
  ou faux.

## 3. D3 — `diff --audio --align-onset` : fait, et la dernière fenêtre

Vos deux paires, `--until-frame 300` :

| Paire | Sans l'option | Avec `--align-onset` |
|---|---|---|
| `music_large` | 9 fenêtres, max 85,24 % : DIFF | 7 fenêtres, max 0,09 %, `onset shift -536 samples` : MATCH |
| `superfx_game_skeleton` | 9 fenêtres, max 3,08 % : DIFF | 7 fenêtres, max 0,23 %, `onset shift -570 samples` : MATCH |

Le décalage est `b - a`. Le JSON gagne `align_onset`, `onset_shift` et
`length_mismatch`.

**Ce qui change sans l'option.** Seules les fenêtres complètes des deux
côtés sont comparées, comme vous le proposiez : la onzième fenêtre de
`superfx_game_skeleton` (7 échantillons contre 0, 100 %) n'existe plus.
Conséquences :

- un `diff --audio` qui finissait sur une fenêtre partielle rapporte une
  fenêtre de moins (vos huit recaptures « à moins de 0,3 % » passeront de
  10 à 9 ou resteront à 10 selon la longueur) ;
- en échange, une capture plus courte que l'autre d'une fenêtre ou plus
  est un DIFF à elle seule (une machine arrêtée), pour ne pas perdre ce
  que l'ancienne règle attrapait ;
- deux captures plus courtes qu'une fenêtre sont comparées en une
  fenêtre, sur leur longueur commune.

Si une seule des deux captures est silencieuse, `--align-onset` rend DIFF.

## 4. D4 — `diff --sequence` : fait, votre prototype est la spécification

```
luna diff sa1_starfield_avant.sfc sa1_starfield_apres.sfc --sequence --from 1 --to 200
frames 1-200
A: 100 pictures, frames per picture [1, 2]
B: 165 pictures, frames per picture [1, 2]
longest common run: 98 pictures in the same order (from frame 6 in A, frame 6 in B, offset +0)
98 of 100 pictures (98.0% of the shorter sequence, at least 90% asked): SAME-SEQUENCE
```

Les quatre lignes du haut sont celles de `frame_sequence.py`, au chiffre
près, en 1,9 s au lieu de 400 lancements. Témoins rejoués : une ROM contre
elle-même, 60 images sur 60 ; `sa1_starfield` contre `mode2`, 1 image sur
100, DIFF. Nous n'avons pas rejoué `superfx_3d` (ROM non jointe).

Le contrat :

- `--from` vaut 1 par défaut, `--to` est obligatoire.
- Seuil : `--min-common N` (images) ou `--min-common-pct P` (part de la
  suite la plus courte). **Sans seuil, 90 %** : c'est notre choix, dites
  s'il vous gêne.
- Sortie JSON (`--out`) : `side_a` / `side_b` (`pictures`,
  `frames_per_picture`), `common_run` (`pictures`, `a_frame`, `b_frame`,
  `offset`, `pct_of_shorter`), le seuil, `status` (`same-sequence` /
  `diff`). Codes de sortie 0 / 1 / 2.
- Comme le prototype, la suite commune est **contiguë** dans les deux ROM.
- `--input`, `--force-display`, `--native-res`, `--power-on` s'appliquent.

Une limite à connaître : une plage qui ne couvre que l'écran noir du
démarrage donne une image de chaque côté, donc `1 of 1`, SAME-SEQUENCE.
Le guide le dit ; `--min-common N` s'en protège.

## 5. Ce qui a changé d'autre depuis la v1.34.0

- **SA-1 en PAL** : le timer HV compte 312 lignes sur une console PAL (il
  rebouclait à 262). NTSC inchangé : vos ROM ne bougent pas.
- **GUI** : un compteur d'images par seconde (*Settings → Video*).

## 6. Ce que nous vous demandons

Quand la v1.35.0 sera publiée (une note suivra, avec ses sommes) :
l'épingler, supprimer `frame_sequence.py` si la sortie vous convient, et
nous dire si le seuil par défaut, le refus `-n` + `--until-frame` ou la
règle des fenêtres complètes cassent quelque chose chez vous.

## 7. Ouvert

Vos deux points retenus (MS0 + 21 MHz, LoROM DSP-1 de 2 Mo) : nous
attendons vos ROM.
