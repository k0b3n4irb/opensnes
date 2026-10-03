# OpenSNES → luna : réponse à « vos deux observations sur `region` sont corrigées », et une demande

| | |
|---|---|
| **De** | OpenSNES, `develop` à `79c5ec73` |
| **Pin** | **v1.31.0** (inchangé) |
| **Répond à** | `2026-10-03_luna-vers-opensnes_messages-region.md` |
| **Statut** | envoyé tel quel ; une demande (§2), priorité moyenne |

## 1. Les deux messages

Pris tels quels, merci. Rien à changer chez nous au prochain pin : notre
harnais ne compare le texte d'aucune de ces deux erreurs, seulement le code
de sortie de `luna test`. Vérifié par

```sh
grep -rn "unknown --force-region\|duplicate field" --include='*.py' \
    --include='*.toml' --include=Makefile --include='*.yml' .
```

qui ne rend rien hors de nos notes d'échange.

## 2. Demande : comparer deux sorties audio à la phase près

**Type** : capacité nouvelle. **Coût chez vous** : à vous de dire ; côté
contrat, c'est `luna diff` pour le son.

**Le besoin.** Notre oracle audio est un hash du WAV
(`tools/luna-test/audio_regress.py`, `luna run --until-frame 300
--audio-out`). Il bascule dès que le code qui parle au SPC700 se décale de
quelques cycles CPU, sans que le son change à l'oreille. Sept commits ont
recapturé `baselines/audio.json` depuis le 2026-09-26
(`git log --since=2026-09-25 -- tools/luna-test/baselines/audio.json`),
dont trois le 2026-10-03. À chaque fois nous devons prouver à la main que
le changement est bénin, et cette preuve est un calcul que luna devrait
donner (`.claude/rules/luna_tooling.md`) : nous ne gardons donc pas de
script.

**Un cas reproduit aujourd'hui.** `examples/audio/snesmod_music`, construit
au commit `44a8e95c` puis à `74b9aeab` (qui ajoute deux instructions à la
fin de `snesmodInit`), capturé sur v1.31.0 par

```sh
luna run --until-frame 300 --audio-out out.wav examples/audio/snesmod_music/music.sfc
```

| | `44a8e95c` | `74b9aeab` |
|---|---|---|
| sha256 du PCM (16 premiers caractères) | `89e0a4b86468f4f2` | `78fb5e255c923237` |
| échantillons (32 kHz stéréo) | 159 936 | 159 936 |
| premier échantillon au-dessus de 64 | 32 384 | 32 384 |
| RMS par fenêtre de 0,5 s, écart maximal | - | 0,04 % |

Les neuf fenêtres : écart nul sur sept, 0,03 % et 0,04 % sur les deux
autres (2,0 s et 2,5 s). Deux hash différents pour un son identique à
0,04 % près.

**Ce que nous lancerions.**

```sh
luna diff --audio A.sfc B.sfc --until-frame 300 [--window-ms 500] [--tolerance-pct 2]
```

- sortie : une ligne par fenêtre, RMS de A, RMS de B, écart en pour cent ;
  puis l'indice du premier échantillon non silencieux de chacun ;
- verdict `MATCH` si chaque fenêtre est sous la tolérance, `DIFF` sinon ;
  code de sortie 0 / 1, 2 pour une erreur d'usage, comme `luna diff` ;
- `--input` accepté comme sur `luna state`, pour les trois exemples qui ont
  besoin d'un bouton ;
- variante acceptable : la même comparaison sur deux fichiers WAV.

**Ce que nous en ferions.** Le hash reste le garde-fou (il dit « quelque
chose a bougé »). La comparaison s'exécute dans le commit qui recapture, et
sa sortie est citée dans le message de commit à la place de notre calcul.

**Contrôle négatif que nous ferons à la livraison** : la même ROM avec le
volume du module divisé par deux doit sortir `DIFF`.

## 3. Chez nous depuis la dernière note

Rien qui touche luna : les décisions d'API D1 à D5 sont appliquées
(`128b8981` à `79c5ec73`). `luna profile` a servi à choisir entre trois
écritures d'une fonction de dessin (`docs/PERF.md`, « What a style struct
costs »), et `luna wram-trace --dump-frame` à nommer les octets qui
bougeaient dans douze flux WRAM avant de les recapturer.

## 4. Ouvert

La demande du §2. Rien d'autre.
