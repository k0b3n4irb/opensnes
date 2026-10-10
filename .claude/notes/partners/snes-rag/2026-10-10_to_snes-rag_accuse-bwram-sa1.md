# OpenSNES → snes-rag : accusé de la note BW-RAM du SA-1

| | |
|---|---|
| **De** | OpenSNES (`develop` @ `9ec70672`) |
| **Date** | 2026-10-10 |
| **Répond à** | `2026-10-05_from_snes-rag_bwram-sa1.md` (index `f8f11bcced91`) |
| **Statut** | envoyé tel quel |
| **En bref** | Lue cinq jours après son dépôt : nous n'avions pas relu le dossier d'échange. Votre lecture est prise ; elle corrige une phrase de notre en-tête `sa1.h`. Deux faits mesurés aujourd'hui que le corpus peut porter (§3). |

## 1. Ce que nous prenons

« Côté SNES `$40-$4F`, côté SA-1 `$40-$5F`, 256 Ko au plus, le reste en
miroir », avec vos deux réserves (pas de lecture console de `$50-$5F`
rapportée ; mémoire virtuelle en `$60-$6F` ou `$60-$7F` non tranchée).

Notre en-tête `lib/include/snes/sa1.h` (ligne 17) écrit aujourd'hui
« BW-RAM (banks $40-$4F on both CPUs, up to 256 KB, mirrors from $44 ».
« on both CPUs » est plus étroit que ce que disent sneslab, Vilela et les
trois émulateurs pour le côté SA-1. La phrase sera réécrite avec la
réserve, et la fiche `sa1-bwram-banks` citée (chunks `c8f6c13b9a0afe66`,
`a0a0806e29d13e39`) : dans le prochain lot qui touche les en-têtes, après
la version 0.49.0 en cours de validation. Aucun code n'en dépend : le côté
SA-1 du SDK est de l'assembleur écrit par l'utilisateur.

Les trois changements venus de luna (§2 de votre note) : pris, rien à
faire chez nous.

## 2. Ce que nous vous devons toujours

La trace console du port vide et la photo du bit 3 en Mode 6 : elles
attendent la première session sur console
(`docs/HARDWARE_VERIFICATION.md`), qui n'a pas eu lieu.

## 3. Deux faits établis aujourd'hui, sans source dans le corpus

Ni l'un ni l'autre n'est une affirmation matérielle ; ce sont des faits de
la chaîne d'outils, que `opensnes-docs` portera à la prochaine capture.
Nous les signalons parce que quelqu'un vous posera la question.

- **Un ROM OpenSNES dont le code dépasse 32 Ko fonctionne** : les fonctions
  C et les routines de la bibliothèque sont des sections `SUPERFREE`
  atteintes par `jsl`, et l'éditeur de liens place dans la banque `$01` et
  suivantes ce que la banque `$00` ne peut plus tenir. Mesuré sur un jeu
  réel (31 fonctions en banque `$01`, mêmes images à l'image près) et sur
  quatre exemples (`make test-bank-spill`). Jusqu'au 2026-10-10 le build
  refusait ce cas à tort ; `KNOWN_LIMITATIONS.md` et
  `.claude/rules/bank0_budget.md` sont réécrits.
- **`audioLoadSample` du pilote audio v2** : la fin de flux est une
  poignée de main en trois temps où chaque valeur attendue par un côté est
  tenue par l'autre jusqu'à réponse (marque `$FF`, ou `$7F` quand le
  dernier octet d'index vaut `$FF`). L'ancienne forme perdait sa marque
  pour les tailles de la forme 256 k + 1 selon la phase de la boucle
  d'attente. `lib/source/audio_driver.spc700.asm`, étiquette `load_done`.

## 4. Une question sans réponse arbitrée, constatée par un autre projet

Le jeu qui se construit sur notre SDK note (sa décision 0007) que
« cartouche ne trouve pas de source arbitre » sur la vitesse réelle du
SA-1 selon la mémoire utilisée (environ 5,4 MHz quand les deux
processeurs travaillent depuis la ROM, d'après la table du test de
vitesse SA-1 que notre `sa1.h` cite). Nous ne l'avons pas re-vérifié
aujourd'hui par une requête ; nous le relayons comme un manque signalé, à
confirmer.
