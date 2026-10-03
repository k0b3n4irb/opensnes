# luna → OpenSNES : seules les cinq dernières releases gardent leurs binaires — et une proposition pour votre dépôt

| | |
|---|---|
| **De** | luna (`k0b3n4irb/luna`, v1.32.0) |
| **Re** | information, sans lien avec une de vos notes |
| **Statut** | fait le 2026-10-03 chez nous ; une conséquence sur vos anciens tags (§2) ; une proposition : le même ménage chez vous (§3) |

## 1. Ce qui a changé

Nous avons supprimé 34 releases GitHub de luna, de `v1.0.0` à `v1.30.1`.
Restent, avec leurs binaires :

`v1.32.0` (Latest), `v1.31.0`, `v1.30.4`, `v1.30.3`, `v1.30.2`.

Les 48 tags git sont tous conservés : une ancienne version se reconstruit
depuis son tag (`git checkout vX.Y.Z && cargo build --release -p luna-cli`).

C'est désormais la règle : cinq releases avec binaires, les plus hautes
par numéro de version. Le but est d'amener les utilisateurs qui ne
compilent pas vers les versions récentes.

## 2. Ce que cela touche chez vous

| Référence OpenSNES | `luna.version` | Binaire téléchargeable |
|---|---|---|
| `develop` | `v1.32.0` | oui |
| `main`, `v0.47.0` | `v1.30.2` | oui, mais c'est la plus ancienne gardée |
| `v0.46.0` | `v1.28.0` | non |
| `v0.45.0` | `v1.27.0` | non |
| `v0.44.0` | `v1.24.0` | non |

Relevé sur votre dépôt à `f02958f1`.

- Vos tags `v0.46.0` et plus anciens ne peuvent plus télécharger leur luna
  épinglée. Si votre CI rejoue ces tags, elle échouera à cette étape.
- `v1.30.2` sortira de la liste à la prochaine version de luna. Votre
  `main` devra alors épingler une version plus récente, ou reconstruire
  luna depuis le tag.

Dites-nous si cette règle vous pose un problème que nous n'avons pas vu.

## 3. Proposition : le même ménage chez vous

Le mainteneur propose d'appliquer la même règle à `k0b3n4irb/opensnes` :
cinq releases avec binaires, et pas d'exécution d'Actions de plus d'un
mois. Chez nous c'est une règle de routine depuis le 2026-10-03
(`.claude/rules/housekeeping.md`, commit `2b0197c`).

Notre script fonctionne tel quel sur votre dépôt. Nous l'avons lancé à
blanc, sans rien supprimer :

```bash
REPO=k0b3n4irb/opensnes /chemin/vers/luna/tools/housekeeping.sh
```

| | Aujourd'hui | Après ménage |
|---|---|---|
| Releases | 32 (`v0.21.3` → `v0.47.0`) | 5 : `v0.43.0`, `v0.44.0`, `v0.45.0`, `v0.46.0`, `v0.47.0` |
| Exécutions d'Actions | 2 199 | 569 (1 630 datent d'avant le 2026-09-03) |

`--apply` supprime. Les seuils se règlent par `KEEP_RELEASES` et
`RUNS_MAX_AGE`. Le script ne supprime jamais un tag.

Ce que nous avons appris en le faisant :

- **Garder les tags.** Supprimer une release sans `--cleanup-tag` laisse
  le tag : l'ancienne version se reconstruit depuis lui. Recréer une
  release supprimée demande de rebâtir les binaires, qui n'auront plus les
  mêmes sommes de contrôle.
- **Trier par numéro de version, pas par date.** Chez nous `v1.30.1` a
  été publiée après `v1.30.2`.
- **Chercher qui épingle une ancienne release avant de supprimer.** Nous
  ne savons pas qui télécharge vos anciens SDK.
- **Le dire aux utilisateurs.** Nous l'avons écrit dans le guide
  d'installation et le CHANGELOG, avec la commande pour reconstruire
  depuis un tag.
- **Durée** : 887 exécutions supprimées en un peu plus de dix minutes, une
  requête par exécution. Comptez une vingtaine de minutes pour 1 630.
- **Pas de tâche planifiée.** Chez nous le script est lancé à la main,
  après chaque release. C'est un choix du mainteneur, pas une contrainte
  technique.

Nous n'avons rien supprimé chez vous : la décision et le lancement vous
reviennent.
