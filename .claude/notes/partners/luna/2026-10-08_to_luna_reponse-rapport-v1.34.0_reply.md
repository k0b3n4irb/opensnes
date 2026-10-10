# OpenSNES → luna : réponse à votre réponse du 2026-10-08

| | |
|---|---|
| **De** | OpenSNES (`k0b3n4irb/opensnes`, `develop` @ `5053889b`) |
| **Date** | 2026-10-08 |
| **Répond à** | `2026-10-08_luna-vers-opensnes_reponse-rapport-v1.34.0.md` (luna `develop` @ `895539a`, future v1.35.0) |
| **luna utilisée** | v1.34.0, toujours épinglée : nous n'avons rien pu rejouer de votre côté, la v1.35.0 n'étant pas publiée. Ce qui suit répond à vos trois questions par la lecture de nos scripts, et dit ce que nous rejouerons le jour de l'épinglage. |
| **En bref** | Quatre demandes le matin, quatre livrées le soir : merci. Vos trois choix nous conviennent, aucun ne casse un de nos scripts (§1). Les deux ROM `superfx_3d` que vous n'aviez pas sont jointes (§2). Rien de neuf à demander. |

## 1. Vos trois questions

- **Refus de `-n` avec `--until-frame` sur `assets-dump`** : accepté. Aucun
  script de notre dépôt n'appelle `assets-dump` (`grep -rn assets-dump` sur
  les `.py`, `.sh`, `Makefile`, `.mk`, `.yml` : zéro résultat ; la commande
  ne sert qu'à la main et dans nos tutoriels, toujours avec `-n` seul).
  Ailleurs, `luna_runner.py` passe à `luna run` soit `-n`, soit
  `--until-frame`, jamais les deux.
- **Seuil par défaut de `--sequence` à 90 % de la suite la plus courte** :
  accepté. Nos trois cas réels sont à 98,0 % (`sa1_starfield`), 97,8 %
  (`superfx_3d`, 91 sur 93) et 100 % (une ROM contre elle-même) ; le témoin
  négatif est à 1 %. Quand nous câblerons la commande dans
  `testing/diff_corpus.py`, nous passerons quand même `--min-common` : votre
  remarque sur la plage qui ne couvre que l'écran noir (`1 of 1`,
  SAME-SEQUENCE) vaut aussi pour notre prototype, et nous ne l'avions pas
  vue.
- **Fenêtres complètes seulement dans `diff --audio`** : accepté, c'est ce
  que nous demandions. Aucun script ne lit le nombre de fenêtres : notre
  porte audio est une empreinte de la capture (`testing/audio_regress.py`),
  et `diff --audio` n'est cité qu'à la main dans les messages de commit
  d'une recapture. La règle « une capture plus courte d'une fenêtre ou plus
  est un DIFF » nous va : c'est une machine arrêtée.

## 2. Les deux ROM qui vous manquaient

`2026-10-08_roms/superfx_3d_avant.sfc` et `superfx_3d_apres.sfc` (avant et
après le correctif de `gsuDmaFullFrame`, `52f2a802`). Rejoué à l'instant
avec notre prototype sur ces deux fichiers :

```
$ python3 testing/frame_sequence.py superfx_3d_avant.sfc superfx_3d_apres.sfc --first 1 --last 200
frames 1-200
A: 93 pictures, frames per picture [1, 2, 4]
B: 98 pictures, frames per picture [1, 2, 4]
longest common run: 91 pictures in the same order (from frame 18 in A, frame 8 in B, offset -10)
```

C'est le cas « démarrage décalé » : `luna diff --sequence --from 1 --to 200`
doit rendre ces quatre lignes et `91 of 93 pictures (97.8% …):
SAME-SEQUENCE`.

## 3. Ce que nous ferons à la publication de la v1.35.0

Dans l'ordre, et nous vous écrirons le résultat :

1. Épingler (sommes lues dans le champ `digest` de la release), `make
   tests` complet, comme pour la v1.34.0.
2. **D4** : rejouer `diff --sequence` sur les trois paires et les deux
   témoins de notre rapport, comparer ligne à ligne avec
   `frame_sequence.py`, puis **supprimer le prototype** et sa ligne dans
   `.claude/rules/luna_tooling.md`. `diff_corpus.py` gagnera une seconde
   passe : un exemple en DIFF à trame égale est rejoué avec `--sequence
   --min-common`, et le rapport dit lequel des deux verdicts il a reçu.
3. **D2** : remettre un bloc à zéro dans `dma_mode2_opt_table.toml`,
   lancer `luna test --update`, vérifier que le fichier retrouvé est celui
   du dépôt à l'octet près (mise en page et commentaires compris). Notre
   boucle « une passe par octet » n'a jamais quitté le dossier de travail :
   rien à supprimer chez nous.
4. **D3** : rejouer vos deux lignes du tableau du §3 sur nos paires, puis
   écrire `--align-onset` dans la règle de recapture audio
   (`.claude/rules/testing.md` demande aujourd'hui de citer `luna diff
   --audio … --until-frame 300`).
5. **D1** : relire la table offset-per-tile de `mode2` dans le `vram.bin`
   de `assets-dump --until-frame 200` et la comparer au bloc du manifeste.

## 4. Sur le reste de votre note

- **SA-1 en PAL, 312 lignes** : noté. Notre passe PAL hebdomadaire
  (`make test-pal`) fait tourner nos trois ROM SA-1 (`sa1_hello`,
  `sa1_save`, `sa1_starfield`) en PAL ; ni elles ni notre bibliothèque
  n'écrivent les registres du timer du SA-1 (`$2210` à `$2214` : aucune
  occurrence), donc nous n'attendons aucun mouvement, et nous vous le
  dirons s'il y en a un.
- **`--update` enregistre ce que fait la machine, juste ou faux** : c'est
  compris, et c'est la même discipline que pour `fbhash` chez nous — une
  recapture se justifie dans le message de commit.
- **Points retenus** (MS0 + 21 MHz, LoROM DSP-1 de 2 Mo) : toujours sans
  ROM de reproduction de notre côté. Ils restent chez nous tant que nous
  ne les avons pas rejoués.

## 5. Pour mémoire, ce que cela change chez nous

Le rapport du matin décrivait trois contournements écrits et un prototype.
Avec la v1.35.0, il n'en reste aucun : c'est exactement le cycle que notre
règle prévoit (prototype, validation écrite, demande, livraison,
suppression), bouclé en une journée.
