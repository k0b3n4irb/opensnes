# luna → OpenSNES : `luna record`, une exécution en vidéo légère + chronologie texte

Non bloquant, léger. Rien à faire de votre côté : c'est une annonce.

`develop` est à `fea4a77` et porte une nouvelle commande, pas encore dans une version publiée.

```
luna record <rom> --from-frame N --count K --input "<script>" --out /tmp/base
```

Elle écrit trois choses depuis une seule exécution :

| Fichier | Contenu |
|---|---|
| `base.mp4` | l'image ×2 et le son, H.264 + AAC ; sous chaque image, un bandeau `F<image> T<secondes> <12 boutons>` |
| `base.md` | la chronologie : quand l'image bouge, se fige ou s'éteint, quand la manette change, le son seconde par seconde |
| `base-1.png` … | une image étiquetée par seconde, 36 par planche |

Mesuré sur deux jeux du commerce : 3 Mo la minute en scène calme, 5 Mo
en scène d'action, son compris (`--video-kbps`, 500 par défaut, borne
la taille). La chronologie fait environ 5 Ko par minute.

- La vidéo demande `ffmpeg` sur la machine ; `--no-video` écrit la
  chronologie et les planches sans lui.
- Une ligne « manette » nomme la première image dessinée avec ces
  boutons : c'est l'image du script d'entrées plus un.
- Une image est dite figée au bout de 30 images identiques, l'écran
  éteint au bout de 4.

Non prouvé : la sortie en PAL et en overscan (239 lignes) n'a pas été
regardée ; le chemin libx264 de `ffmpeg` n'a pas été exécuté (cette
machine n'a que libopenh264).

Guide : `book/src/using/cli-analysis.md`, section `luna record`.
