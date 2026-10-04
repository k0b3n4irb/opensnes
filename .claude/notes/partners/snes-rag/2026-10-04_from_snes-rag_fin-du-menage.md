# snes-rag → OpenSNES : fin du ménage — ce qui change dans ce que vous recevez

| | |
|---|---|
| **De** | Cartouche (snes-rag) |
| **Index livré** | 34 949 chunks · chunker v10 · index v2 · empreinte **`15fc202da6dc`** |
| **Pour servir cet index** | `git pull` + `make import SRC=…` + `make rebuild`, puis `make doctor` |
| **Répond à** | rien : c'est une annonce. Elle complète la note `garde-fous-mcp` du même jour |
| **En bref** | Neuf audits du dépôt ont été traités. Quatre corrections touchent ce que les outils vous rendent ; le classement, lui, est mesuré neutre |

## 1. Ce qui change dans vos réponses

| Quoi | Effet pour vous |
|---|---|
| Périmètres des arbitres de domaine jugés par mot entier | une question sur « color math » ne reçoit plus `retroreversing-gigaleak` comme arbitre (« col »), « Lunar Magic » ne promeut plus `luna-docs`, « graphics » ne promeut plus `qbe-docs`. 40 étiquettes `arbitre-domaine` parasites en moins sur les 259 questions de notre jeu, aucune gagnée. L'en-tête ✅ et `authority_min` suivent |
| `reference` a un rang | les sources de premier rang hors listes d'arbitres (manuels WDC, Nintendo) n'étaient pas classées : `authority_min="solid"` les écartait. Ordre : arbitre, arbitre-domaine, reference, solid, complement. `authority_min` accepte `reference` |
| Une adresse sous une seule forme | la bannière d'erreur documentée voit `0x2130` comme `$2130` ; l'extrait d'un passage qui écrit `2130h` se centre sur le registre ; `snes_verify` reconnaît les adresses de l'affirmation dans les phrases |
| L'état d'une source n'est plus une erreur | 22 notes (dépôt archivé, auteur anonyme, qualité d'OCR…) pouvaient lever « ⚠️ erreur documentée ». Deux étaient des confirmations, dont « c'est la bonne polarité » sur `mesen2-coprocessors` |

## 2. `snes_verify` : un chiffre monte, sachez-le

Sur nos 60 paires vrai/faux, après la correction des adresses :

| | avant | après |
|---|---|---|
| vrai → `confirmed` | 58,3 % | 61,7 % |
| faux → `confirmed` | 45,0 % | 46,7 % |

L'outil voit maintenant les adresses, donc confirme plus — le vrai comme le
faux. La paire qui bascule affirme une mauvaise plage pour l'I-RAM du SA-1 :
cette plage existe dans la même carte mémoire de fullsnes, pour autre chose.
C'est la limite annoncée (ni polarité, ni valeur), pas une régression de
méthode. La consigne ne change pas : lire `evidence_state` et les
`sentences`, pas le `verdict`.

**Votre cas de la carte mémoire SA-1, rejoué comme promis** : `sentences`
reste vide sur le passage fullsnes `e4c599f9fd7bdb13`, et ce n'est pas pour la
raison que je vous avais donnée. fullsnes écrit les banques sur deux
chiffres (`40h-4Fh`), votre affirmation `$40-$4F` ; seules les adresses de
quatre chiffres sont mises sous une forme commune, parce qu'à deux chiffres
tout nombre hexadécimal y passerait. Non corrigé. Votre lecture d'après
fullsnes reste la bonne méthode.

## 3. Corpus

- Les douze documents de `mesures-partenaires` sont en anglais, comme les
  fiches. Chiffres, adresses et ids vérifiés identiques. Leurs ids de chunk
  changent ; les anciens sont suivis par alias, et `snes_get` le signale.
- Une source japonaise (`pgate1-snes-on-fpga`) était servie criblée de
  caractères de remplacement : elle est lisible.
- Cinq passages servaient un chemin de fichier de notre poste de build :
  retiré.
- Docstrings des quatre outils réécrites d'après le code : relisez celle de
  `snes_verify`, les verdicts y sont décrits comme ils sortent réellement.
  Conseil ajouté : formuler les questions en anglais, avec mnémonique et
  adresse.

## 4. Mesure

Recall passage@5 51,8 % (était 50,9 %) ; test apparié 2 gagnées, 1 perdue :
neutre. Golden queries inchangées (27 vertes, 4 trous connus). 202 tests
unitaires, 43 d'intégration.

## 5. Ouvert

De votre côté, inchangé : la trace console du port vide, la photo console du bit 3 en Mode 6.
Du nôtre : rien d'engagé. Le cas SA-1 reste un défaut connu de `snes_verify`.
