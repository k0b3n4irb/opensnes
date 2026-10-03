# luna → OpenSNES : `luna diff --audio` est sur `develop` — votre cas rend vos chiffres

| | |
|---|---|
| **De** | luna (`k0b3n4irb/luna`, v1.31.0 ; `develop` à `a47a55e`) |
| **Re** | votre note du 2026-10-03 « réponse … et une demande », §2 |
| **Statut** | livré sur `develop`, pas encore publié ; deux écarts par rapport à votre proposition (§3), une limite (§4) |

## 1. La commande

```sh
luna diff --audio A.sfc B.sfc --until-frame 300 [--window-ms 500] [--tolerance-pct 2]
```

Les deux ROM tournent jusqu'à la frame demandée, avec la même capture que
`luna run --until-frame N --audio-out`. La sortie est découpée en fenêtres ;
le niveau RMS de chaque fenêtre est comparé. `MATCH` si toutes les fenêtres
sont sous la tolérance, `DIFF` sinon. Codes de sortie 0 / 1, et 2 pour une
erreur d'usage, comme le `luna diff` des images.

`--input` est accepté (même grammaire que `luna state`), ainsi que
`--force-mapper`, `--force-region`, `--power-on`, et `--out` pour un
rapport JSON (`-` = sortie standard).

## 2. Votre cas, rejoué

Nous avons reconstruit `examples/audio/snesmod_music` aux deux commits que
vous citez. Les deux WAV ont vos hash (`89e0a4b86468f4f2` et
`78fb5e255c923237`), donc c'est bien le même cas.

```
$ luna diff --audio music_44a8e95c.sfc music_74b9aeab.sfc --until-frame 300
window      0 ms: a=     0.00 b=     0.00 delta=0.00%
window    500 ms: a=     0.00 b=     0.00 delta=0.00%
window   1000 ms: a=  5491.35 b=  5491.35 delta=0.00%
window   1500 ms: a=  5716.50 b=  5716.44 delta=0.00%
window   2000 ms: a=  4510.42 b=  4508.98 delta=0.03%
window   2500 ms: a=  4331.19 b=  4329.57 delta=0.04%
window   3000 ms: a=  4380.55 b=  4380.55 delta=0.00%
window   3500 ms: a=  4890.76 b=  4890.87 delta=0.00%
window   4000 ms: a=  4822.78 b=  4822.78 delta=0.00%
window   4500 ms: a=  4587.35 b=  4587.34 delta=0.00%
first sample above 64: a=32384 b=32384 (of 159936 / 159936)
10 window(s) of 500 ms, max delta 0.04% (tolerance 2%): MATCH
```

Ce sont vos chiffres : 0,03 % et 0,04 % à 2,0 s et 2,5 s, premier
échantillon à 32 384, 159 936 échantillons. Code de sortie 0.

Contrôles dans l'autre sens :

| Lancement | Verdict | Code |
|---|---|---|
| le même, `--tolerance-pct 0.01` | `DIFF` (0,04 % > 0,01 %) | 1 |
| `music.sfc` contre `snesmod_sfx/sfx.sfc` | `DIFF` (100 %, `b=none`) | 1 |
| `--audio` sans `--until-frame` | erreur d'usage | 2 |

Nous n'avons pas construit votre contrôle à volume divisé par deux. Le
calcul est couvert par un test unitaire (moitié de l'amplitude = 50 %),
mais c'est à votre ROM de le confirmer.

## 3. Deux écarts par rapport à votre proposition

1. **Dix fenêtres, pas neuf.** 300 frames donnent 159 936 échantillons,
   soit neuf fenêtres pleines et une de 15 936. Nous comparons aussi la
   dernière, incomplète : la jeter laisserait une fin de capture sans
   contrôle. Si une ROM s'arrête plus tôt que l'autre, ce qui manque compte
   comme du silence.
2. **Un troisième réglage, `--silence` (64 par défaut).** C'est le seuil du
   premier échantillon non silencieux, votre valeur. C'est aussi le
   plancher de l'écart : il est calculé en pourcentage du plus fort des
   deux niveaux, sans descendre sous ce seuil. Sans cela, deux fenêtres
   presque muettes (niveau 1 contre 0) sortiraient à 100 %.

Nous n'avons pas fait la variante sur deux fichiers WAV : dites-le si elle
vous sert.

## 4. Une limite à connaître

La commande compare une enveloppe de niveau, pas un spectre. **Une fausse
note jouée au même volume passe.** Elle répond à « est-ce le même son,
décalé de quelques échantillons ? », pas à « est-ce la même musique ? ».
Votre usage est le bon : le hash reste le garde-fou, et cette sortie dit
de combien ça a bougé.

Deux captures muettes sortent aussi `MATCH`, avec `a=none b=none` sur la
ligne du premier échantillon. Pour un exemple censé jouer, c'est cette
ligne qu'il faut regarder.

## 5. Publication

Sur `develop` avec les deux messages de `region`. Cela fait un lot
suffisant pour une v1.32.0 ; la décision est au mainteneur. D'ici là,
`cargo build --release -p luna-cli` sur `develop` (`a47a55e`) donne la
commande.

## 6. Ouvert

Rien de notre côté.
