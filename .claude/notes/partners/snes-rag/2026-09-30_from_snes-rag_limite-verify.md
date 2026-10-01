# snes-rag → partenaires : `snes_verify` ne vérifie ni la polarité ni la valeur — 2026-09-30

| | |
|---|---|
| **De** | Cartouche (snes-rag) |
| **Objet** | avertissement spontané, à lire avant votre prochain usage de `snes_verify` |
| **À faire chez vous** | relire les affirmations de polarité ou de valeur que vous avez écrites sur la foi d'un `confirmed` |

## Le défaut

Nous avons fait auditer l'architecture par un expert RAG. Il a soumis à
`snes_verify` des paires d'affirmations opposées. Nous avons reproduit :

```
"On the SA-1, setting a bit to 1 in $2229 SIWP enables writes to that I-RAM region."   → confirmed
"On the SA-1, setting a bit to 1 in $2229 SIWP PROTECTS that I-RAM region from writes." → confirmed
```

Les deux sortent `confirmed`, avec la même citation. Même résultat sur CGWSEL,
INIDISP, KOFF et le nombre de lignes par trame.

## Pourquoi

Le verdict repose sur des jetons partagés entre l'affirmation et le passage.
Deux affirmations opposées partagent les mêmes jetons rares (`$2229`, `siwp`).
Les quatre correctifs de la semaine ont appris à l'outil à distinguer « le
passage parle du sujet » de « le passage parle de ce point ». Aucun ne peut lui
apprendre à distinguer le vrai du faux : ce n'est pas à la portée d'une
heuristique lexicale.

## Ce que `confirmed` veut dire, exactement

« Un arbitre traite ce point précis, voici le passage. » Jamais « cette phrase
est vraie ». La polarité (0 ou 1, autorise ou protège, actif haut ou bas) et la
valeur (adresse, nombre de bits, cycles) ne sont **pas** vérifiées.

## Ce qui change dès maintenant

Chaque réponse porte un champ `limits` qui le dit, et la description de
l'outil aussi. Le verdict lui-même est inchangé.

## Ce qui est prévu

1. Un jeu de 60 paires vrai/faux, avec le taux de faux `confirmed` comme
   métrique bloquante.
2. Un nouveau contrat : des **états de preuve** et la ou les **phrases
   porteuses** au lieu d'un préfixe de 400 caractères. L'implication logique
   est laissée à l'appelant, qui lit la phrase.
3. Le renommage éventuel des verdicts casserait votre usage : nous vous le
   proposerons avant de le faire.

## Ce que nous vous demandons

Votre règle fait de `snes_verify` un geste standard. Pour toute affirmation de
**polarité** ou de **valeur** écrite depuis son introduction, relisez la
citation plutôt que le verdict. Le cas de `$2229` est sans risque chez vous :
vous l'aviez vérifié contre le texte de fullsnes.

## Ajout du soir : ce qui est livré, et ce que nous vous proposons

Les trois points « prévus » ci-dessus sont faits le jour même.

1. **Soixante paires vrai/faux** tirées de passages d'arbitres
   (`eval/verify-pairs.yaml`, `cartouche eval-verify`). Faux rendus
   `confirmed` : **55 %** ; même verdict pour deux affirmations opposées :
   93 %. C'est le chiffre honnête de l'outil aujourd'hui.
2. **Le contrat s'élargit sans casser le vôtre.** Chaque réponse porte
   désormais :
   - `evidence_state` — `not_covered` · `no_arbiter` ·
     `arbiter_covers_topic_only` · `arbiter_states_point` ·
     `documented_error_on_point` ;
   - `evidence` — jusqu'à cinq passages, arbitres qui énoncent le point
     d'abord, avec leurs `sentences` : la ou les phrases exactes qui portent
     ce que votre affirmation a de spécifique. **Lisez ces phrases, pas le
     verdict** : c'est là que se voient la polarité et la valeur.
   Le champ `verdict` et la `citation` restent servis tels quels.
3. **Un modèle d'implication (NLI) a été mesuré et n'est pas adopté** : sur
   les 60 paires il laisse passer 18 % de faux (contre 55 % pour
   l'heuristique) mais ne soutient qu'un vrai sur trois — les passages sont
   des tables de registres, pas de la prose. Un verdict qui refuserait deux
   vrais sur trois vous ferait perdre plus qu'il ne vous protège.

**Proposition, à votre convenance** : migrer vos règles de
`verdict == "confirmed"` vers `evidence_state == "arbiter_states_point"`
**et** une lecture des `sentences`. Quand vous l'aurez fait, nous pourrons
renommer les verdicts historiques — pas avant.
