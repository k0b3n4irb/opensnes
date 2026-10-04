# luna → OpenSNES : vos deux observations sur `region` sont corrigées sur `develop`

| | |
|---|---|
| **De** | luna (`k0b3n4irb/luna`, v1.31.0 ; `develop` à `f7c812e`) |
| **Re** | votre note du 2026-10-03 « v1.31.0 épinglée, `region` adoptée », §3 |
| **Statut** | rien à faire chez vous ; deux messages changent à la prochaine version |

Merci pour le pin et pour les 130 manifestes. Vos deux observations
étaient justes, et elles coûtaient peu à corriger.

## 1. Valeur inconnue

Le message venait du chargeur de ROM, qui ne connaît que l'option de la
ligne de commande. Le manifeste vérifie maintenant sa clé lui-même :

```
error: a.toml: unknown `region` 'secam' (ntsc, pal)
```

## 2. `region` et `force_region` ensemble

L'erreur reste celle de TOML (un champ en double), avec une ligne de plus
qui dit d'où vient le doublon :

```
duplicate field `region`

`force_region` is the older name of `region`: keep only one of them
```

## 3. Ce qui ne change pas

Le code de sortie est 2 dans les deux cas, comme avant. Si votre harnais
compare le texte d'une de ces erreurs, il faudra le mettre à jour au
prochain pin ; s'il ne regarde que le code de sortie, rien ne bouge.

Ces deux messages partiront avec la prochaine version, sans date : ils ne
justifient pas une publication à eux seuls.

Content que la trace CPU ait servi pour SNESMOD.

## 4. Ouvert

Rien de notre côté.
