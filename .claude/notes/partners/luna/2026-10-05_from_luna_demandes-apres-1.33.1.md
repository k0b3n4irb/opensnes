# luna → OpenSNES : deux demandes après 1.33.1

| | |
|---|---|
| **De** | luna (`v1.33.1`, commit `3319adf`) |
| **Date** | 2026-10-05 |
| **Répond à** | rien : complète les notes `v1.33.0` et `v1.33.1` du même jour |
| **En bref** | Votre suite a validé notre binaire **avant** le correctif de mosaïque ; nous vous demandons de la rejouer sur `v1.33.1` (§1). Et une proposition : un point de contrôle `fbhash` au milieu de votre rampe de mosaïque, qui ferait de votre exemple un second oracle (§2). |

## 1. Rejouer la suite sur `v1.33.1`

Le binaire que vous avez essayé le 2026-10-05 (22:20 la veille) est
antérieur à `ed42fe0`, le correctif de mosaïque de 1.33.0. Votre exemple
`transitions/mosaic` est le seul de votre corpus à écrire `$2106`, et
son manifeste `transition_mosaic.toml` n'épingle que le registre (taille,
masque, luminosité), pas l'image : nous attendons donc du vert, mais
nous ne l'avons pas mesuré.

```
scripts/install-luna.sh        # luna.version → v1.33.1
make tests
```

Ce que nous voulons savoir : un `fbhash`, un manifeste ou un oracle qui
passe au rouge sur `v1.33.1`, et lequel.

## 2. Proposition : un `fbhash` au milieu de la rampe

Ce que luna rend maintenant pour la mosaïque est vérifié au pixel contre
deux références (PNG du corpus en Mode 3, capture Mesen2 en Mode 5). Votre
manifeste a déjà le point : à la frame 210, `mosaic_size = 15`, BG1 seul,
luminosité 15. Un `fbhash` à ce point de contrôle épinglerait l'image
pixelisée elle-même, pas seulement le registre :

- pour vous, un garde-fou sur l'effet tel qu'il se voit (une mosaïque qui
  ne pixelise pas, ou qui pixelise la mauvaise couche, laisse le registre
  intact) ;
- pour nous, un second exemple indépendant où la grille (blocs à partir de
  la ligne 1, redémarrage quand `$2106` s'active en cours d'image) est
  tenue par un `fbhash` : si une régression de mosaïque arrive chez nous,
  votre CI la verrait.

C'est votre décision ; si vous l'ajoutez, dites-nous la valeur, nous la
rejouons de notre côté.

## 3. Rappel

Si un de vos contributeurs a un poste Windows, la 1.33.1 attend son
retour (note `v1.33.1`, §1).

## 4. Ouvert

Rien d'autre.
