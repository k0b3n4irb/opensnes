# OpenSNES → luna : rapport du 2026-10-08 (v1.34.0)

| | |
|---|---|
| **De** | OpenSNES (`k0b3n4irb/opensnes`, `develop` @ `ad07a4dd`) |
| **Date** | 2026-10-08 |
| **luna utilisée** | v1.34.0 (`luna 1.34.0`), épinglée depuis le 2026-10-06, binaire `linux_arm64` |
| **Statut** | Envoyé tel quel. Chaque point a été rejoué aujourd'hui sur la v1.34.0 avec la commande citée. |
| **ROM jointes** | `2026-10-08_roms/` à côté de ce fichier (sept ROM de 256 Ko, les nôtres) : les commandes de ce rapport s'y lancent telles quelles. |
| **En bref** | Une journée de compilateur entièrement jugée par luna : cinq défauts trouvés, dont deux anciens. Quatre demandes, la plus simple d'abord ; aucune n'est bloquante. Aucun écart de luna à signaler. |

## 1. Ce que luna a rendu possible aujourd'hui

Le contexte : nous faisons passer le compilateur devant celui de PVSnesLib
sur tous les compteurs. Quatre étapes livrées dans la journée, toutes
arbitrées par vos sorties.

- **`luna profile` comme instrument de mesure.** Dix-huit charges de travail
  compilées par les deux SDK, les cycles lus dans `entries[].mclk` par
  soustraction d'une ROM témoin, la profondeur de pile dans
  `stack.low.sp`. De −21,0 % à −38,4 % de cycles en une journée, chaque
  étape mesurée avant d'être gardée. La porte de CI (`+5 %` au total) tourne
  sur votre binaire.
- **Le test différentiel.** 17 000 graines d'expressions et 36 000 graines
  de programmes générés, exécutés sur luna et comparés à un modèle de C
  (`luna state --peek res:2 --peek done:2`). Trouvé aujourd'hui grâce à lui :
  une inférence fausse du QBE d'origine (`(y || K) && 1` rendait `y != 0`)
  et une erreur interne du compilateur.
- **`luna diff` au même numéro de trame** a fait trouver un défaut de notre
  bibliothèque qui n'avait rien à voir avec le changement testé :
  `superfx_3d` sortait à `offset +20`. `luna profile --until-frame 16` a
  nommé la cause en une commande — `_gsu_dff_read_v` : 2 761 198 mclk sur
  16 trames. Notre `gsuDmaFullFrame`, appelée écran éteint avant les bandes,
  attendait la ligne 225 exactement, celle où la NMI commence, et passait
  par chance. Corrigé (`52f2a802`) ; le démarrage dure 8 trames, plus 17 ou
  38 selon le minutage.
- **`luna state --peek` par symbole** a confirmé en une commande un défaut
  silencieux présent depuis un mois : `(far_arr + 8)[-1]` lisait la banque
  `$7F` (`0000` lu, `0710` attendu). Corrigé (`27afe435`).
- **`luna diff --audio`** a dit, pour dix recaptures d'empreintes audio,
  que le son n'avait pas changé (huit à moins de 0,3 %). Les deux autres
  font l'objet de la demande D3.

## 2. Demandes, la plus simple d'abord

### D1 — `assets-dump --until-frame`

- **Nature** : cohérence d'interface. **Coût estimé** : faible.
- **Constat.** `run`, `state`, `profile` et `diff` s'arrêtent à une trame ;
  `assets-dump` ne connaît que `-n` (instructions).

  ```
  $ luna assets-dump --until-frame 200 --out /tmp/ad 2026-10-08_roms/mode2.sfc
  error: unexpected argument '--until-frame' found
  ```
- **Besoin.** Prendre `vram.bin` à la trame qu'un manifeste vérifie
  (`frames = 200`) pour relire un bloc. Avec `-n`, la trame atteinte dépend
  de la vitesse du code, c'est-à-dire de ce que nous sommes en train de
  changer.
- **Contrat.** `luna assets-dump --until-frame F [--input …] --out DIR ROM`,
  mêmes fichiers qu'aujourd'hui.

### D2 — `luna test` : les octets réels d'un bloc en échec

- **Nature** : confort de recapture. **Coût estimé** : faible.
- **Constat.** Un `[asserts.blocks]` en échec ne donne que le premier octet
  fautif, en texte comme en JSON. Manifeste de trois lignes :

  ```toml
  rom = "2026-10-08_roms/mode2.sfc"
  frames = 200
  [asserts.blocks]
  row1 = { space = "vram", offset = "6040", hex = "0000000000000000" }
  ```
  ```
  $ luna test <dossier>
  FAIL blk
       blocks.row1 (vram): first mismatch at +0x0 (expected 00, got 44)
  $ luna test <dossier> --report json     # tests[0].failures[0], même phrase
  ```
- **Ce que cela nous a coûté.** Deux blocs VRAM suivent la durée du
  démarrage (la table offset-per-tile de `mode2` et de `mode6`). Après un
  changement voulu, nous les avons remesurés en lançant le manifeste une
  fois par octet et en corrigeant l'hexadécimal à chaque tour : 64 passes
  pour `dma_mode2_opt_table`.
- **Contrat, l'un ou l'autre.**
  1. Dans le rapport JSON, à côté de la phrase : `{"assert":
     "blocks.row1", "space": "vram", "offset": "6040", "expected_hex": "…",
     "actual_hex": "…"}` (la longueur du bloc attendu).
  2. `luna test --update` étendu aux blocs, comme il l'est à
     `asserts.fbhash`, commentaires et mise en forme conservés.
  La première nous suffit ; la seconde supprime notre script.

### D3 — `luna diff --audio` : un départ décalé d'une trame sort en DIFF

- **Nature** : verdict trompeur dans un cas précis. **Coût estimé** : moyen.
- **Constat.** Quand le code qui lance la musique gagne une trame, le son
  est le même, 536 échantillons plus tôt. La fenêtre qui contient le départ
  compare alors 500 ms de musique à 483 ms.

  ```
  $ luna diff --audio music_large_avant.sfc music_large_apres.sfc --until-frame 300
  window   1000 ms: a=   175.43 b=  1188.39 delta=85.24%
  window   1500 ms: a=  4008.49 b=  3876.19 delta=3.30%
  …
  first sample above 64: a=47906 b=47370 (of 159936 / 159936)
  10 window(s) of 500 ms, max delta 85.24% (tolerance 2%): DIFF
  ```
  Vous imprimez déjà les deux départs : l'information est là, le verdict ne
  s'en sert pas.
- **Second effet, même commande.** Les deux captures n'ont pas la même
  longueur et la dernière fenêtre, partielle d'un côté et vide de l'autre,
  donne 100 % :

  ```
  $ luna diff --audio superfx_game_skeleton_avant.sfc superfx_game_skeleton_apres.sfc --until-frame 300
  window   5000 ms: a=  6874.44 b=     0.00 delta=100.00%
  first sample above 64: a=33914 b=33344 (of 160007 / 159945)
  11 window(s) of 500 ms, max delta 100.00% (tolerance 2%): DIFF
  ```
- **Contrat proposé.** `--align-onset` : les fenêtres partent du premier
  échantillon au-dessus du seuil de chaque capture, la comparaison s'arrête
  à la plus courte, et la ligne de verdict dit le décalage (`onset shift
  -536 samples`). Sans l'option, rien ne change. Pour la dernière fenêtre,
  option ou non : ne comparer que des fenêtres complètes des deux côtés.
- **Ce que nous faisons en attendant.** Le message de commit cite votre
  sortie et explique les deux fenêtres à la main (`fb672b2b`).

### D4 — `luna diff --sequence` : mêmes images, autre cadence

- **Nature** : capacité nouvelle, spécifiée par un prototype qui tourne.
  **Coût estimé** : moyen. C'est la demande qui nous servirait le plus.
- **Constat.** `--tolerance N` répond à « B montre-t-elle à F±N ce que A
  montre à F ». Deux cas réels d'aujourd'hui lui échappent, tous deux
  bénins :
  1. un démarrage décalé de plus que la tolérance demandée ;
  2. une boucle qui tourne librement et change de cadence. `sa1_starfield`
     recopie 128 sprites depuis l'I-RAM ; le compilateur ayant accéléré
     cette boucle, elle tient plus souvent dans une trame. Aucun décalage
     unique ne fait coïncider les deux ROM :

     ```
     $ luna diff sa1_starfield_avant.sfc sa1_starfield_apres.sfc --frames 200,400 --tolerance 40
     frame 200: DIFF a=84b073e05d705cca b=3944629a106b9f95
     frame 400: DIFF a=e81db5cec89f1309 b=85034387eb04de03
     2 frame(s): 0 match, 2 diff (tolerance ±40)
     ```
- **Le prototype** (`testing/frame_sequence.py` dans notre dépôt, moins
  de cent lignes, il ne fait qu'appeler `luna run --until-frame F --print-fbhash`
  pour chaque trame d'une plage) : il réduit chaque suite de trames
  identiques à une image, puis cherche la plus longue suite d'images que
  les deux ROM montrent dans le même ordre.

  ```
  $ python3 testing/frame_sequence.py sa1_starfield_avant.sfc sa1_starfield_apres.sfc --first 1 --last 200
  A: 100 pictures, frames per picture [1, 2]
  B: 165 pictures, frames per picture [1, 2]
  longest common run: 98 pictures in the same order (from frame 6 in A, frame 6 in B, offset +0)
  ```
  98 des 100 images de A sont dans B, dans l'ordre : même animation, B va
  plus vite.
- **Témoins.** Deux ROM sans rapport (`sa1_starfield` et `mode7/extbg`) :
  `longest common run: 1 pictures` (l'écran noir du démarrage). Une ROM
  contre elle-même sur 120 trames : 66 images sur 66, `offset +0`. Un
  démarrage plus court de dix trames (`superfx_3d` avant et après le
  correctif de bibliothèque du §1) : 91 images communes, `offset -10`.
- **Contrat.** `luna diff A B --sequence --from F1 --to F2` ; sortie texte
  et JSON : nombre d'images distinctes de chaque côté, durées en trames,
  longueur de la plus longue suite commune, trame de départ dans A et dans
  B. Verdict `SAME-SEQUENCE` au-dessus d'un seuil que l'appelant donne
  (`--min-common N` ou une proportion), `DIFF` sinon. Code de sortie comme
  le reste de `diff`.
- **Coût chez nous sans elle** : 400 lancements de luna par paire de ROM
  pour 200 trames. Le prototype est supprimé le jour
  où vous livrez (notre règle « Luna-First »).

## 3. Priorités

| | Demande | Nature | Ce que cela débloque |
|---|---|---|---|
| D4 | `diff --sequence` | capacité | la preuve « même rendu » pour les ROM à boucle libre et les démarrages décalés, sans script chez nous |
| D2 | octets réels d'un bloc | confort | une recapture en une passe au lieu de 64 |
| D3 | `diff --audio --align-onset` | verdict | un MATCH quand seul le départ a bougé |
| D1 | `assets-dump --until-frame` | cohérence | relire la VRAM à la trame d'un manifeste |

Rien n'est urgent : chacune a aujourd'hui un contournement écrit.

## 4. Sans demande

- **Toujours retenus, pas envoyés** : l'absence de diagnostic quand MS0 et
  21 MHz sont actifs ensemble sur le Super FX, et le LoROM DSP-1 de 2 Mo.
  Ils partiront avec une ROM de reproduction ou pas du tout.
- **Aucun écart de luna** n'a été relevé pendant ces quatre étapes : chaque
  différence qu'elle a signalée s'est expliquée chez nous, et deux étaient
  de vrais défauts de notre côté.
- Le message d'erreur de `--peek` sur un format refusé (`expected
  BANK:OFFSET:COUNT, got …`) nous a évité de chercher : merci.
