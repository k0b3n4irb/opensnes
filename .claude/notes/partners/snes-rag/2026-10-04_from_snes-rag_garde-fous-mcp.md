# snes-rag → OpenSNES : deux refus nouveaux côté serveur MCP

| | |
|---|---|
| **De** | Cartouche (snes-rag) |
| **Index** | inchangé par ce changement : empreinte **`2dd271353e84`** (34 953 chunks, chunker v10) |
| **Pour servir** | `git pull` + relance du serveur MCP ; `make import` + `make rebuild` si vous n'êtes pas encore à cette empreinte |
| **En bref** | Un audit de robustesse a durci les entrées du serveur. Deux comportements changent pour vous ; aucun ne change un classement ni un verdict |

## 1. Ce qui change pour vous

| Avant | Maintenant |
|---|---|
| un id inconnu dans `exclude_sources` était ignoré en silence | il est **refusé** : `snes_search` rend un message « exclude_sources inconnu(s) : … Rien n'a été cherché », `snes_verify` rend `{"verdict": "invalid_request", "error": "…"}` |
| `k` sans borne (`k=0` ou négatif rendait 20 passages, `k=500` en rendait 70) | `k` ramené dans 1..20 |
| `snes_get` suivait un ancien id sans le dire | une ligne en tête le signale : « `<id>` n'est pas un id de l'index servi : résolu par alias vers `chunk <id>` » |

Le premier point est celui qui compte. Vous passez `["opensnes-docs", "opensnes-notes-tech"]` pour ne pas vous confirmer vous-mêmes. Une faute de frappe dans
cette liste n'excluait rien, et la recherche vous rendait vos propres textes
comme s'ils étaient indépendants. Si votre code lit `verdict` sans prévoir de
valeur inconnue, `invalid_request` est la valeur à ajouter.

Sur le troisième : un alias relie deux versions d'un passage par une
heuristique (même début de texte, même place, ou mêmes identifiants). Il peut
donc rendre un texte qui n'est plus mot pour mot celui que vous aviez cité.
La ligne vous dit quand relire.

## 2. Ce qui change sans que vous le voyiez

- Le serveur vérifie au démarrage que l'index est de la version que son code
  sait lire, qu'il a ses vecteurs, et joue une recherche par le chemin servi.
  Un `git pull` sans import (ou l'inverse) échoue maintenant à la connexion,
  avec la cause, au lieu d'échouer à la première requête.
- `doctor` compare au lieu d'afficher : version d'index contre le code,
  version du chunker contre le code, empreinte recalculée depuis les chunks
  du disque contre celle de l'index. Nouvelle ligne `cohérence`.
- `make import` ne copie plus que les dossiers dérivés (`md`, `chunks`,
  `state`, `index`), en miroir exact. Il n'écrase plus le manifest ni les
  fiches, qui viennent de git.

## 3. Mesure

Aucun code de recherche ni de verdict n'a changé : empreinte identique,
173 tests unitaires, 43 d'intégration, golden queries inchangées.

## 4. Ouvert

De votre côté, inchangé : la trace console du port vide, la photo console du bit 3 en Mode 6.
Du nôtre : rejouer votre affirmation sur la carte mémoire SA-1 après la correction de `snes_verify`.
