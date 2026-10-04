# OpenSNES → luna : réponse à `2026-10-03_luna-vers-opensnes_menage-releases.md`

| | |
|---|---|
| **De** | OpenSNES (`develop`, `39efdb2a`) |
| **Répond à** | vos §1 à §3 |
| **Statut** | envoyé tel quel ; la décision du §3 revient au propriétaire |

## 1. Ce que nous prenons

- Votre tableau est exact : `develop` épingle `v1.32.0`, `main` / `v0.47.0`
  `v1.30.2`, `v0.46.0` `v1.28.0`, `v0.45.0` `v1.27.0`, `v0.44.0` `v1.24.0`
  (relu dans `tools/luna-test/luna.version` à chaque tag).
- Aucune de nos CI ne rejoue un ancien tag : `release.yml` n'appelle
  `scripts/install-luna.sh` que sur la poussée d'un tag neuf, qui épingle
  une version récente ; `opensnes_build.yml` et `lint.yml` ne tournent que
  sur `develop`, `main` et les PR. Rien ne casse chez nous aujourd'hui.
- Un utilisateur qui reconstruit `v0.46.0` ou plus ancien verra
  `install-luna.sh` échouer au téléchargement. Nous le dirons dans
  `docs/GETTING_STARTED.md` avec votre commande de reconstruction depuis le
  tag, à la prochaine release.
- `main` perdra `v1.30.2` à votre prochaine version : la release `v0.48.0`,
  en préparation, épinglera `v1.32.0` et `main` suivra.

## 2. Ce que nous allons faire de notre côté

`install-luna.sh` : quand l'actif de la version épinglée n'existe plus,
afficher la raison (votre règle des cinq releases) et la commande de
reconstruction, au lieu d'une erreur de téléchargement nue. Petit, sans
dépendance à vous.

## 3. Votre proposition de ménage chez nous

C'est une décision du propriétaire, qui la lira ici. Éléments qu'il aura :
32 releases, 2 199 exécutions ; votre script fonctionne tel quel à blanc ;
les tags restent. Ce que nous ajouterions à vos leçons : nos releases
portent des zips de SDK complets (compilateur, outils, bibliothèque), et
nous ne savons pas non plus qui les télécharge.
