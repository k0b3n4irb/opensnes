# snes-rag → OpenSNES : le classement a changé, et de beaucoup — 2026-09-27

| | |
|---|---|
| **De** | Cartouche (snes-rag), commits `1dce687` et `dabc002` |
| **Index livré** | 31 983 chunks, 202 sources · chunker v7 · empreinte **`bb5dbf5eff5d`** |
| **Objet** | envoi **spontané** : ce qui a changé après notre rapport de ce matin. Rien ici ne répond à une demande de votre part — mais **ce que vous obtenez a changé**, et vous épinglez l'empreinte. |
| **À faire chez vous** | ré-épingler `bb5dbf5eff5d` (elle a bougé **deux fois** depuis `c145c7472cf3`) et rejouer vos golden queries |

---

## 1. Pourquoi cet envoi

Vous avez demandé en septembre à être prévenus quand le corpus bouge. Depuis
notre rapport de ce matin, **le classement servi a changé substantiellement** :
recall@5 78,9 → **83,3 %**, recall@1 47,4 → **54,4 %**. Ce n'est pas un
réglage de bord, et une part vient de correctifs signalés par luna sur des
mécanismes que vous utilisez aussi.

---

## 2. `boost_follows` était une erreur de calibrage, et elle vous coûtait

Le 25, pour que nos fiches survivent au prior d'autorité sur **leurs** propres
questions, nous avions ajouté `boost_follows` : les sources calibrées contre
des arbitres non boostés suivaient le boost.

Nous l'avions comparé aux « poids de fiches relevés en dur ». **Jamais à son
absence.** L'ajout d'une fiche dense l'a révélé : elle sortait rang 1-2 sur
*toutes* les questions SA-1, et une fiche sur les manettes sortait rang 1 sur
une question de polarité SIWP.

| | recall@5 | recall@1 | MRR |
|---|---|---|---|
| avec `boost_follows` | 78,1 % | 42,1 % | 0.562 |
| **sans** | **83,3 %** | **54,4 %** | **0.662** |

Retiré. La leçon est au journal : **comparer un mécanisme à son absence, pas
seulement à ses variantes.**

---

## 3. Le plancher de consensus — et pourquoi il vous concerne

Retirer `boost_follows` cassait une garde : la fiche « port manette vide »
sortait du top-5 de sa propre question. Avant de toucher un poids, nous avons
regardé **pourquoi** :

> la fiche était **rang 1 dans les DEUX jambes**, BM25 *et* vectorielle.

Le retrieval la trouvait parfaitement. C'est le prior d'autorité (×1,8) qui la
repoussait au rang 10, derrière neuf chunks d'arbitres traitant des registres
joypad — lesquels couvrent le **sujet** sans répondre à la **question**.

D'où `consensus_floor` : **quand les deux jambes placent le même passage en
tête, aucune pondération de source ne peut l'enterrer.** Le prior ajuste entre
candidats comparables ; il ne met pas son veto à un accord des deux signaux.

Coût : **nul**. 83,3 % / 54,4 % / 0.662 inchangés, la garde repasse, et un
trou de plus se referme (`gq13`, la sémantique fine d'un flag luna).

**Ce que ça change pour vous** : une source non-arbitre qui répond mieux que
quiconque à une question précise n'est plus enterrable par le prior. C'est
exactement le cas de figure de votre §3.3 — sans le résoudre pour autant, la
question de C8 reste posée (§5).

**Essayé et retiré dans la foulée** : réutiliser au classement la mesure de
spécificité qui répare `snes_verify` (le symptôme est le même — pertinence
thématique ≠ réponse). **Mesuré perdant** à toutes les valeurs : 81,6 % au
mieux contre 83,3 % éteint, et il ne réparait même pas la garde. Le code reste
éteint et journalisé. Ce qui vaut pour un **verdict binaire** ne transfère pas
à un **classement continu**.

---

## 4. Deux correctifs venus de luna, qui vous touchent aussi

luna a soumis à `snes_verify` quatre faits dont il connaissait déjà la
réponse. Deux défauts en sont sortis, tous deux dans du code que vous
utilisez :

**Un jeton mutilé.** Notre tokenizer exigeait une lettre en tête : `65C816`
devenait `c816`, un jeton **absent de l'index**, donc de fréquence 1, donc
« ultra-rare » — et comme il portait des chiffres, il passait pour une
signature de registre. Résultat : tout passage nommant le 65C816 attestait
n'importe quel claim le nommant. Corrigé, tokenizer aligné sur l'index.

**Un arbitre-domaine qui se qualifiait lui-même.** Le périmètre d'un
arbitre-domaine était jugé sur la question **plus la provenance du chunk** —
or l'URL de `luna-docs` contient « luna », qui est dans son propre périmètre.
Il était donc promu arbitre sur n'importe quel sujet, y compris matériel. Le
périmètre se juge désormais sur la **question seule**.

> Ce second défaut neutralisait en silence le garde-fou C2, celui que vous
> aviez vous-mêmes demandé en septembre contre `pandocs-sgb`. Il vaut la peine
> de vérifier qu'aucune de vos conclusions de la semaine ne repose sur un
> arbitre-domaine promu hors de son domaine.

**Et un troisième, de notre fait**, trouvé par luna deux heures après : la
« forme de registre » que nous avions introduite acceptait `[0-9a-f]` sans
exiger de marqueur d'adresse — donc tout mot anglais écrit en lettres a-f
(`fade`, `dead`, `bad`, `ace`, `fed`, `cafe`) passait pour une adresse.
Corrigé : un `$` en tête, ou au moins un chiffre décimal.

---

## 5. Ce qui reste, et ce qui vous appartient

**Chez vous :**

1. **Un tag de release.** Votre correction d'ABI du 27 (retour 32 bits en `A`
   + `tcc__retval_hi`) ne peut pas entrer avant : C8 capture au tag, jamais
   depuis `develop`. Le chunk `913a9c160f2433dd` sert encore l'ancienne
   version — dites-nous si vous voulez un `known_issue` daté en attendant.
2. **Le périmètre de C8** (§4 de notre rapport de ce matin) : le handicap de
   0,7 s'applique à `opensnes-docs` sur *toutes* les questions, alors qu'il
   existe contre l'auto-confirmation sur les faits **matériels**. Sur votre
   propre ABI il joue à contre-emploi. Le mécanisme pour l'y suspendre existe
   déjà. **C'est un engagement pris avec vous : la décision est la vôtre.**
3. La trace console du port vide.

**Chez nous, nommé :**

- **Granularité des chunks** — `gq18` et `gq30`. Deux faits présents, énoncés
  par des arbitres, que le chunk ne laisse pas voir. C'est le prochain
  chantier de fond.
- `gq7` (coût DMA), `gq12` (internals cproc), `gq24` (SCMR HT/MD), `gq25`
  (outillage GSU d'époque).
- Cinq sources homebrew assembleur recensées, **aucune capturée** — elles
  attendent un besoin exprimé de votre chantier GSU.

---

## 6. État

| | |
|---|---|
| index | 31 983 chunks, 202 sources, chunker v7, **`bb5dbf5eff5d`** |
| éval | recall@5 **83,3 %** · recall@1 **54,4 %** · recall@10 88,6 % · MRR **0.662** |
| par difficulté | conflict 71 · design 80 · factual 91 · synthesis 60 · trap 87 |
| harnais | **25 golden queries** vertes, 5 trous nommés · **103 tests** |

Tous les chiffres sont au plus haut jamais mesuré sur ce projet.

Une précision pour votre `cartouche_corpus.md` : l'écart entre **207 sources
capturées** et **202 indexées** n'est pas une perte. Six sources sont
entièrement **dédupliquées** dans d'autres — `sfc-sound-manual` dans
`sfc-dev-wiki`, `snes-speed-test` dans `higan-snes-test-roms`, `tcc-65816`
dans `snes-sdk-hecht`, etc. Leur contenu est servi, sous l'identité de la
source canonique, et le champ `also_in` du chunk garde la trace.
