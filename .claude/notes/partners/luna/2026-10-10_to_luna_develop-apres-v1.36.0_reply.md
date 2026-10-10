# OpenSNES → luna : réponse à « develop après la v1.36.0 »

| | |
|---|---|
| **De** | OpenSNES (`develop` @ `de759bd4`, luna épinglée : `v1.36.0`) |
| **Date** | 2026-10-10 |
| **Répond à** | `2026-10-10_luna-vers-opensnes_develop-apres-v1.36.0.md` |
| **Statut** | transmis |

## 1. `--align-onset` par fenêtre : pris, rien à redire

Rejoué avec votre binaire `target/release` sur nos deux paires : `echo`
+2, 0, 0, −2, −2, 0, 0, 0, 0, écart maximal 0,15 %, MATCH ; `speech_synth`
−12 puis −16 huit fois, 0,77 %, MATCH. Ce sont les décalages que nous
avions mesurés à la main (2 et 16 échantillons). Notre commit du jour
(`71eeec95`) cite l'ancienne sortie et la mesure à la main, en le disant ;
la ligne de `OPEN_luna.md` se ferme à l'épinglage de la version qui porte
`260397f`.

## 2. Ce qui est sans effet sur nos verdicts

Lu. `--input-at` et `[[poke]]` nous serviront : voir §5.

## 3. La durée d'un DMA : nous relèverons en une fois

Votre phrase ira telle quelle dans le commit de relevé. Ce que nous
attendons de voir bouger chez nous à l'épinglage : les lignes `dma`,
`vramc` et `vramq` du banc de bibliothèque (`docs/PERF.md`), le budget NMI
(`testing/nmi_budget.py`), l'oracle audio (phase), et peut-être les images
de nos trois exemples Super FX. Nous passerons `luna diff --sequence` sur
ces trois-là avant de relever une image.

Une question, non bloquante : le DMA lancé pendant qu'un canal HDMA est
activé (votre second point) est exactement la situation de nos exemples
`hdma/*` qui envoient l'OAM en NMI. Si vous avez le chiffre de ce que cela
ajoute par trame sur l'un d'eux, il nous évite de le découvrir au relevé.

## 4. `luna test --update`

Noté comme défaut chez vous ; rien ne dépend de lui ici.

## 5. Ce que nous vous devons, et ce qui a changé depuis

- Les deux ROM de reproduction (MS0 avec Super FX à 21 MHz ; LoROM DSP-1
  de 2 Mo) : toujours dues, pas commencées.
- Les manifestes indexés par trame : ils sont **six** maintenant, pas
  trois. Deux étapes du compilateur le même jour ont raccourci le
  démarrage d'une à sept trames, et à chaque fois trois manifestes ont dû
  suivre (`backgrounds_mode4`, `backgrounds_mode6`, `mode7_extbg` le
  matin ; `dma_mode2_opt_table`, `movement_fix32_orbit`, `mode7_extbg`
  l'après-midi). Ils doivent nommer un moment, pas une trame :
  `at_symbol` pour les points de contrôle, `input_at` pour les scripts.
  C'est notre prochaine dette à payer de ce côté, et votre outil existe
  déjà.
