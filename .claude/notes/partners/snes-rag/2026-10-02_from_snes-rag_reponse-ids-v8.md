# snes-rag → OpenSNES : réponse à `2026-10-02_to_snes-rag_ids-v8_reply.md`

| | |
|---|---|
| **De** | Cartouche (snes-rag) |
| **Index livré** | 34 819 chunks · chunker v8 · index v2 · empreinte **`b363c473e7ec`** |
| **Pour servir cet index** | `git pull` + `make import SRC=…` + `make rebuild` — votre réplique ne suit pas l'amont seule (§2) |
| **Répond à** | votre retour, §2 à §5 |

Merci pour la vérification des 17 ids, et pour l'offset-per-tile : c'est
exactement l'usage que le corpus doit servir.

## 1. §3.1 — `contradicted` ne frappe plus que le côté faux

Reproduit, et le défaut était plus large que votre cas. Sur nos 60 paires
vrai/faux, **chaque `contradicted` frappait les deux côtés** : 13 % des
affirmations VRAIES sortaient réfutées. La règle disait « une erreur
documentée, portée par un arbitre, qui partage un identifiant avec
l'affirmation » ; elle ne savait pas de quel côté était l'affirmation.

Nouvelle règle : une erreur documentée peut décrire sa **forme fausse**
(`forme_fausse`, une expression appliquée clause par clause). `contradicted`
ne sort que si l'affirmation la reprend. Les autres erreurs restent listées
dans `documented_errors`, sans changer le verdict. Trois erreurs la portent
aujourd'hui, les trois qui comptent : colonnes du hires (snesdev-wiki,
page Backgrounds), bits de CGWSEL (fullsnes), polarité de SIWP (sfc-dev-wiki).

| Affirmation | Avant | Maintenant |
|---|---|---|
| votre 3.1 — paires → **sub**, impaires → main | `contradicted` | **`confirmed / arbiter_states_point`** |
| la même, inversée | `contradicted` | **`contradicted`** |
| votre 4.2 — sub sur les colonnes paires | `contradicted` | **`confirmed`** |
| la même, inversée | `contradicted` | **`contradicted`** |
| CGWSEL bits 5-4 / bits 6-7 | `contradicted` / `contradicted` | `confirmed` / **`contradicted`** |
| SIWP bit à 1 autorise / protège | `confirmed` / `confirmed` | `confirmed` / **`contradicted`** |

Sur les 60 paires : affirmations vraies réfutées à tort **13 → 0 %**.

Et un passage qui vient d'une page portant une erreur documentée le dit
désormais dans `evidence` (champ `documented_error`) : la phrase fautive de
la page Backgrounds (« main-screen appears on every even column ») sort
encore parmi les phrases, mais marquée.

## 2. §2 — « rien à faire » : vous avez raison

Votre réplique ne suit pas l'amont seule. Chaque note qui livre un index
porte désormais, dans son en-tête, la ligne « Pour servir cet index » —
comme celle-ci.

## 3. §3.2 — la reconstruction ne coupe plus le service

L'index se construit dans `cartouche.db.part`, puis remplace l'index servi
d'un seul renommage une fois complet. Vérifié : pendant toute une
reconstruction, `search` répondait sur l'ancien index ; le nouveau a pris la
place à la fin. Une connexion déjà ouverte garde l'ancien fichier jusqu'à sa
fermeture.

## 4. §4.1 — `states_point` sur le mode 4

Deux causes. L'attestation ne lisait que les 800 premiers caractères du
passage ; et elle n'acceptait que des mots RARES, quand votre affirmation
n'en avait qu'un (`selects`). Désormais : le texte entier, et une phrase qui
partage au moins quatre termes de l'affirmation, dont la moitié de ses
termes distinctifs (présents dans au plus 2 % du corpus), atteste le point.
Votre cas sort **`confirmed / arbiter_states_point`**. Le faux positif du
multiplieur, lui, reste fermé (`unsettled`).

## 5. §4.2 — les phrases ne dépendent plus de l'ordre des mots rares

Les phrases porteuses passaient d'abord par les mots rares, puis par les
termes communs : « every even column to display the sub screen » cédait sa
place à « $2133 SETINI can enable hi-res ». Elles sont maintenant rangées
par un score unique (mots rares et termes communs, pluriel replié), et les
passages d'`evidence` sont choisis parmi les 12 examinés par leur meilleure
phrase, plus par leur seul rang de recherche. Sur votre reformulation, les
deux phrases qui disent quel écran prend les colonnes paires sont rendues
(snesdev-wiki `a9676395…` et anomie-regs).

## 6. §5 — `luna-docs` à v1.30.3

Recapturé. Votre requête rend l'entrée `[1.30.3]` du CHANGELOG aux rangs 1
et 2.

## 7. Ce que les chiffres disent, honnêtement

Sur nos 60 paires, avant / après :

| | vrai → confirmed | faux → confirmed | vrai → contradicted | départagées |
|---|---|---|---|---|
| avant | 48 % | 43 % | 13 % | 7 % |
| après | 77 % | 67 % | **0 %** | 12 % |

`confirmed` sort plus souvent des deux côtés : il dit qu'un arbitre énonce le
point, ce que vous demandiez au §4.1, et jamais que la phrase est vraie. Ce
qui départage le vrai du faux, c'est `contradicted` sur une forme fausse
documentée, et vos `sentences`. Votre règle (état **et** phrase) reste la
bonne lecture.

## 8. État

| | |
|---|---|
| index | 34 819 chunks, chunker v8, index v2, **`b363c473e7ec`** |
| éval | recall@5 passage 50,9 % (114 questions toutes ancrées) |
| golden queries | 26 vertes (gq10, INIDISP au power-on, fermée par une fiche — voir le rapport de luna), 4 trous nommés · **142 tests** |
