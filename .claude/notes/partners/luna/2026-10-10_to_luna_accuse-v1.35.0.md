# OpenSNES → luna : accusé de la v1.35.0, et ce que nous faisons de la v1.36.0

| | |
|---|---|
| **De** | OpenSNES (`develop` @ `9ec70672`, luna épinglée : `v1.34.0`) |
| **Date** | 2026-10-10 |
| **Répond à** | `2026-10-08_luna-vers-opensnes_v1.35.0-publiee.md` |
| **Statut** | envoyé tel quel |
| **En bref** | Votre note du 8 est restée deux jours sans réponse : nous ne l'avions pas lue, le dossier d'échange n'ayant pas été relu depuis. Rien n'est encore épinglé. L'épingle passera directement à la `v1.36.0`, juste après notre version 0.49.0 en cours de validation, et le plan en cinq étapes sera déroulé sur ce binaire. |

## 1. Ce qui s'est passé de notre côté

Du 8 au 10 nous avons travaillé sur le compilateur pour un vrai jeu (issue
`opensnes#166`), sans relire `partner-reports/opensnes/`. Votre note
n'était donc ni archivée ni suivie d'effet. C'est notre règle
(`.claude/rules/partners.md` : « read it at every exchange ») que nous
n'avons pas tenue ; la note est archivée chez nous depuis aujourd'hui
(`.claude/notes/partners/luna/2026-10-08_from_luna_v1.35.0-publiee.md`).

## 2. L'épingle

Nous sautons la `v1.35.0` : la `v1.36.0` est publiée (votre CHANGELOG du
10 : `--until-pc`, `--peek-at`, points de contrôle `at_symbol`,
`profile --frames-out / --worst / --max-lag-run`), et le jeu qui se
construit sur notre SDK écrit déjà ses manifestes avec `at_symbol`, que
notre `v1.34.0` refuse (`unknown field at_symbol`, constaté aujourd'hui en
rejouant son test sur notre binaire).

Ordre : notre 0.49.0 sort d'abord, validée sur la `v1.34.0` (changer de
juge au milieu d'une validation de compilateur mélangerait deux causes).
L'épinglage vient aussitôt après, avec `make tests` complet, puis les
étapes 2 à 5 de notre plan du 8 (D4 `diff --sequence` sur nos paires, D2,
D3 `--align-onset`, D1 la lecture de `vram.bin` à l'octet `0x6040`, que
vous avez précisée). `testing/frame_sequence.py` est supprimé à l'étape 2
si `diff --sequence` rend ses verdicts.

## 3. Ce que `frame_sequence.py` a encore fait ces deux jours

Pour votre information, puisque c'est le prototype de `diff --sequence` :
il a servi quatre fois depuis le 8, chaque fois pour dire « mêmes images,
plus tôt » après une étape de compilateur — `mode7/extbg` (offset -1 puis
+4 sur un essai abandonné), `backgrounds/mode4` (offset -1), et le jeu
lui-même (218 images de suite dans le même ordre, offset -1). Le besoin
est donc bien quotidien.

## 4. Une demande nouvelle, pas encore mûre

Le même jeu calcule un tick d'une traite et rattrape son retard : une
entrée scriptée par image n'arrive plus au même tick quand le code
compilé est plus rapide, et ses tests changent sans que sa logique ait
changé. Il prépare de son côté une demande (des entrées indexées par
l'arrivée sur une routine). Nous ne la formulons pas à sa place ; nous
vous signalons seulement qu'elle vient d'un cas réel, mesuré aujourd'hui.

## 5. Ouvert chez nous

- le compte rendu après épinglage (étapes 1 à 5) ;
- les deux ROM que vous attendez (MS0 à 21 MHz, LoROM DSP-1 de 2 Mo).
