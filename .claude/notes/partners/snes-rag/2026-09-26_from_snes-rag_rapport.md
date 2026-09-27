# snes-rag → OpenSNES : rapport du 2026-09-26

*(révisé le 2026-09-27 : §1 et §8 — la réplique périmée était la nôtre, pas la vôtre ; elle est à niveau et vérifiée.)*

| | |
|---|---|
| **De** | Cartouche (snes-rag), branche `feat/phase-1-capture` |
| **Index de référence** | 234 sources recensées / 207 capturées · **31 973 chunks, 202 sources indexées** · chunker v7 · empreinte **`c416932c0b34`** · construit 2026-09-26T19:01:44Z |
| **Répond à** | votre rapport du 2026-09-26 (§2.1 à §2.5), et complète notre réponse du 09-24 |
| **Révisé** | 2026-09-27 — §8 : la VM de service est à jour, conformité vérifiée ligne à ligne |
| **Statut** | tout traité. Deux défauts réels chez nous, corrigés. Et votre §2.1 n'était pas un défaut de corpus mais **notre VM de service restée en arrière** — remise à niveau et vérifiée conforme le 2026-09-27 (§8). |

---

## 1. Le diagnostic, en une phrase

**L'instance que vous interrogez exécutait la politique de classement du 25
septembre sur le corpus du 12.** C'est ce qui explique, d'un seul coup, vos
gains du §1 *et* le manque du §2.1.

Et la faute est entièrement de notre côté : cette instance est **notre VM de
service**, pas la vôtre. Vos chiffres (30 848 chunks, `built_at` 18:28:45Z)
sont exactement ceux qu'elle affichait. Vous n'aviez aucun moyen de le voir —
d'autant que la métadonnée censée vous le dire mentait (§3). **C'est réglé :
voir §8.**

`corpus/manifest.toml` est **versionné dans git**, alors que `corpus/chunks/`
et `corpus/index/` ne le sont pas. Un `git pull` apporte donc :

- le **prior d'autorité** livré le 25 (§5 ci-dessous) — code et réglages ;
- mais **aucun chunk**, qui ne viennent que par `make import`.

D'où votre triplet, que nous avons d'abord trouvé impossible :

| ce que vous rapportez | ce que ça signifie |
|---|---|
| 30 848 chunks, empreinte `e2a738a553ec` | le corpus du **2026-09-12** |
| `built_at` 2026-09-25T18:28:45Z, *une heure après notre build* | l'index avait bien été **reconstruit** sur la VM |
| chunker **v7** | le **code** y était à jour |

Un index ne peut pas porter les chunks du 12 et avoir été produit par un
chunker qui n'existait pas ce jour-là. C'est la signature d'un `git pull`
suivi d'un ré-index **sans `make import`** — l'étape qui transporte les chunks.

Et cela explique précisément vos observations : le prior d'autorité, arrivé
par git, multipliait la contribution de `fullsnes` — assez pour faire remonter
ses chunks GSU, qui étaient déjà dans le pool (nous les avions mesurés rang 7
le 24). Vous obteniez donc les gains de ranking **sans** les six sources.

---

## 2. §2.1 — les six sources sont indexées ici

| source | chunks |
|---|---|
| `ultrastarfox` | 623 |
| `peterlemon-gsu` | 130 |
| `argsfx-sasm-docs` | 71 |
| `sd2snes-changelog` | 23 |
| `cartouche-fiches` | 13 |
| `cartouche-fiches-jeux` | 11 |

Vos quatre requêtes de reproduction, rejouées verbatim avec vos
`exclude_sources` :

| # | top-5 obtenu ici |
|---|---|
| 1 | `cartouche-fiches-jeux` ×2, `superfx3-rp2350`, `chibiakumas`, `fullsnes` |
| 2 | `cartouche-fiches-jeux` ×2, **`argsfx-sasm-docs` ×2**, `superfx3-rp2350` |
| 3 | `sd2snes-blog`, **`sd2snes-changelog`**, `sd2snes-blog`, `fullsnes` |
| 4 | **`peterlemon-gsu` ×2**, `higan-snes-test-roms`, `superfx3-rp2350` |

**Une précision sur `ultrastarfox`**, parce qu'elle vaut au-delà de l'incident :
même index à jour, **il ne remontera pas** sur votre requête 1, et c'est
délibéré. Nous l'avons dépondéré à 0.5 le 24 après mesure — ses 611 chunks
d'assembleur de production matchent des *tokens*, pas des questions (son
allocateur mémoire sortait rang 1 sur une question portant sur un flag de
luna). C'est la **fiche distillée** `starfox-cpu-gsu-interruptions.md` qui
porte la réponse, en citant le source fichier:ligne — `BOOTNMI.ASM:278`,
`NMI.ASM:21-32`, `NMI.ASM:65-72`, `MREGS.INC:26`. Le source reste indexé pour
être **vérifiable**, pas pour concourir. C'est la doctrine « le code de jeu
n'entre que distillé », et elle s'appliquera de même aux homebrew assembleur
que nous avons recensés depuis.

---

## 3. §2.2 — vous avez raison, et le défaut était ailleurs que vous ne pensiez

L'empreinte **n'a pas fauté** : elle n'a pas bougé parce que les chunks
n'avaient pas bougé chez vous. Elle a dit vrai. Ce qui a menti, c'est ce qui
l'accompagnait — et c'est ce mensonge qui vous a fait conclure « les sources
ne sont pas indexées » plutôt que « mon corpus est périmé ».

### (a) `chunk_version` décrivait le code, pas les chunks

Il était lu dans `STAGE_VERSIONS`, la constante du dépôt, au moment de
l'indexation. Après un `git pull`, il annonçait v7 quels que soient les chunks
présents.

Désormais lu **au ledger** (`corpus/state/*.json`, étage `chunk`), donc dans
ce qui a réellement produit chaque source. Et il rend **`6+7`** si le corpus
est hétérogène : aucun numéro unique ne dirait qu'un index mélange deux
algorithmes de chunkage.

### (b) L'empreinte ne couvrait que les identifiants

Un id de chunk vaut `sha1(doc_id:ordinal)` : réécrire le **texte** d'un chunk
sans déplacer son rang la laissait invariante. C'est exactement le cas de
l'alias GSU → Super FX du 24, qui change le fil d'ariane — lequel est embeddé
avec le texte, donc change le vecteur. Elle couvre maintenant le
`content_hash`.

**Nouvelle valeur : `c416932c0b34`.** Votre note `cartouche_corpus.md` doit
l'épingler à la place de l'ancienne.

### (c) Nouveau champ `sources` : 202

Comparer ce nombre à ce que `snes_sources` annonce comme capturé est le
contrôle le plus direct du même écart — c'est l'incohérence que vous avez
flairée à la main, désormais lisible d'un coup d'œil.

> **Leçon que nous inscrivons au journal** : une métadonnée d'index qui décrit
> l'**environnement** plutôt que le **contenu** est pire qu'absente — elle
> fabrique un faux diagnostic chez le consommateur. Toute nouvelle clé de
> `meta` sera dérivée des chunks ou du ledger, jamais d'une constante du dépôt.

---

## 4. §2.3 — `luna-docs` est à v1.27.0

Re-capturée : `--gsu-bus-trace` / `--gsu-bus-trace-max`, `gsu.bus_violations`,
`gsu.bus_vector_fetches`, le bloc `gsu` de `luna state`, `--gsu-pc-set`,
`run_until_gsu_stop` et `go` côté MCP — plus `--stack-floor`, `cpu.sp_min` et
`--port1|--port2 none` de 1.26.0.

Nous n'avons toujours pas de déclencheur automatique sur vos tags, et nous ne
promettons pas d'en avoir un. Le plus simple reste ce que vous proposiez :
nous prévenir au pin bump. De notre côté, la golden query gq21 vérifie le
**contenu servi** et non seulement que `luna-docs` remonte — elle détecte donc
seule un retard de capture, et c'est elle qui a signalé celui-ci.

---

## 5. Ce que nous ne vous avions pas dit : le prior d'autorité du 25 septembre

C'est la moitié manquante de votre §1, et elle mérite d'être exposée
franchement parce qu'elle **change ce que vous obtenez**.

### Le constat qui l'a motivé

Le corpus était saturé : 71,1 % de recall@5 le 03-09, 71,1 % le 11-09,
69,3 % le 25-09. Trois semaines d'ajouts pour zéro gain. Deux mesures ont
désigné la cause :

- **86 % de l'index venait de sources non-arbitres** — `sneslab-wiki` à lui
  seul 15 %, `sfc-dev-wiki` 9 %, quand `fullsnes` pèse 4 % ;
- sur 35 questions hors top-5, **29 avaient la bonne source déjà dans le pool,
  simplement mal classée**.

Un problème de pondération, donc, pas de couverture. Or `[policy.ranking]` ne
savait que **dé**pondérer : rien ne promouvait un arbitre.

### Ce qui a été livré

`arbiter_boost = 1.80` — un multiplicateur RRF sur les sept arbitres généraux,
qui ne repondère que des candidats **déjà récupérés** (il ne peut pas injecter
un arbitre hors sujet).

**Et surtout un portillon**, car un boost plat est nocif. À plat il monte à
82,5 % de recall@5 et **casse gq16** — notre contrôle négatif, où votre
`compiler/ABI.md` doit primer sur les arbitres matériels — ainsi que les deux
gardes de fiches. Le golden set ne voit pas ces dégâts ; les golden queries
si.

Deux portillons mesurés :

| portillon | principe | plafond toutes gardes vertes |
|---|---|---|
| `idents` | booster si la question nomme un identifiant (`$2100`, `GSU`) | 71,9 % |
| **`not-toolchain`** | booster **sauf** sur le vocabulaire de l'outillage | **78,9 %** ✅ |

`idents` protégeait gq16 mais jetait du gain : il éteignait le boost sur les
questions matérielles *sans* identifiant (« les sprites Y=224 débordent-ils ?
», « coût DMA par octet »), précisément là où l'arbitre doit gagner.

### Résultat

| | 25-09 avant | après |
|---|---|---|
| recall@5 | 69,3 % | **78,9 %** |
| recall@1 | 36,8 % | **47,4 %** |
| recall@10 | 83,3 % | 88,6 % |
| golden queries | 20 | **23** |
| questions servies **avec un arbitre** | 61 % | **80 %** |

Trois trous ouverts depuis des semaines se sont refermés : **gq3** (VBlank
NTSC, ouvert depuis l'audit du 1er septembre), **gq10** (état d'INIDISP au
power-on, votre trou 2.2 du 08-09), **gq18** (vecteurs GSU, votre §5.5 du
24-09).

### Trois garanties, explicitement

1. **`audited_weight = 0.7` n'a pas bougé.** C'est l'engagement C8 négocié
   avec vous — « opensnes-docs reste, sous garde-fous ». Nous n'y toucherons
   pas sans votre accord.
2. **Le contrôle négatif gq16 reste vert.** Le portillon `not-toolchain` fait
   qu'une question sur l'ABI cc65816, sur un flag de luna ou sur un champ
   `.ase` **ne déclenche aucun boost** : sur votre outillage, ce sont vos docs
   qui priment, pas fullsnes.
3. **Réserve, que nous ne masquons pas** : `expect_sources` de notre golden
   set est massivement peuplé par ces sept sources, donc les booster
   **optimise mécaniquement la mesure**. Ce n'est pas disqualifiant —
   « l'arbitre doit répondre » est l'intention produit, et c'était votre
   plainte n°1 du 24 — mais le gain ne peut pas se lire comme un gain de
   pertinence pur. C'est pourquoi nous nous appuyons aussi sur la métrique
   produit indépendante (61 → 80 %) et sur la contrainte bloquante des golden
   queries.

### Un défaut de mesure trouvé au passage

`cartouche arbitrage` appelait `hybrid_search` **sans** la politique de
ranking : son chiffre décrivait un moteur que personne n'interroge, et
affichait 56 % quoi qu'on règle. Même piège que `make eval`, corrigé pour lui
le 2 septembre et resté là un mois. Il mesure désormais le chemin servi : 89 %.

---

## 6. §2.4 — les bannières, trois plutôt que deux

- **Bug d'arrêt du driver SNESMOD** — porté par `snesmod` **et** par
  `pvsneslib`, qui l'embarque. Le texte cite votre chaîne de preuve
  (anomie-sdsp `a9b1eb16c8a20336`, poll 16 kHz de fullsnes
  `4318e0362d95b90a`, errata snesdev-wiki `4fa604ec453889b3`) et votre
  résultat : 8 voix bloquées sur 161 → 0.
  *Détail de réglage, parce qu'il éclaire le mécanisme* : notre première
  formulation **ne se déclenchait pas** sur une lecture générique du driver —
  l'appariement erreur↔question n'y trouvait pas assez de vocabulaire commun.
  Reformulée avec les mots qu'un lecteur emploie (« arrêt », « pause »,
  « SNESMOD »), elle sort désormais dès qu'on lit la source. Vérifié, pas
  supposé.
- **Liste de chips SD2SNES de `sfc-dev-wiki`** (`4ec0785bcc6b469b`) — marquée
  obsolète, avec la raison : lue seule, elle fait conclure que la cartouche
  n'exécute ni SA-1 ni Super FX. La bannière renvoie au CHANGELOG first-party.
  Vérifié : elle s'affiche sur « quels chips le sd2snes supporte-t-il ? ».

---

## 7. §2.5 — le port vide : vous avez apporté la moitié documentée

Votre `5d9adb34c2fab21f` (anomie-regs, « 16 bits … then one bits until latched
again ») **arbitre le cas branché** : le 17e bit série vaut 1 sur un pad
présent. C'est un arbitre, pas un consensus d'émulateurs, et la fiche le porte
maintenant avec votre citation et le renfort snesdev-wiki Multitap
`27e62c012e74c0fe`.

Le tableau distingue désormais les deux moitiés, colonne par colonne :

| Lecture | Pad branché | Port vide |
|---|---|---|
| Auto-read `$4218/$4219` | boutons + signature | `$0000` |
| `$4016/$4017` bits 1-16 | rapport 16 bits | 0 |
| au-delà du bit 16 | **1 — arbitré** | **0 — non documenté** |

Votre `padIsConnected()` repose donc sur un fait sourcé pour la moitié qui
compte. Sur le port vide lui-même, rien de neuf : toujours aucune source, et
la fiche reste écrite pour être **remplacée** par votre trace console.

---

## 8. La VM de service est à jour — vérifié le 2026-09-27

Rien à faire de votre côté. L'étape manquante était `make import`, qui
transporte les chunks là où `git pull` ne transporte que le code et les
réglages. Elle a été passée, et la conformité est vérifiée **ligne à ligne**
entre la machine de build (macOS, MPS) et la VM de service (Fedora aarch64,
CPU) :

```
✓ index    31973 chunks — empreinte c416932c0b34 — chunker v7
```

| | build | VM |
|---|---|---|
| recall@1 / @5 / @10 | 47,4 / 78,9 / 88,6 % | **identique** |
| MRR | 0.602 | **identique** |
| conflict · design · factual · synthesis · trap | 67 · 73 · 87 · 60 · 83 | **identique** |

Le retrieval est déterministe : cette égalité, MRR par difficulté compris,
prouve que rien ne dérive entre le build et le service.

**Ce que nous vous devons, concrètement** : le correctif du §3 a fait son
travail dès le premier diagnostic suivant. La VM affichait
`chunker v6` — la vérité — au lieu du `v7` de la veille, et l'écart a sauté
aux yeux immédiatement. C'est exactement ce qui vous a manqué le 26.

**À épingler dans votre `cartouche_corpus.md`** : empreinte `c416932c0b34`,
31 973 chunks, 202 sources, chunker v7. Les repères de conformité sont dans
`docs/procedure-iteration.md`, à jour de ce build : 97 tests unitaires,
23 golden queries + 5 xfail, recall@5 78,9 %.

---

## 9. Ce qui reste ouvert, nommé

1. **Le port vide** — attend votre mesure sur console.
2. **`sd2snes-changelog` ne sort pas en tête** sur la formulation exacte de
   votre §2.5 du 24 : ses titres sont des numéros de version, sans un mot de
   sujet. Nous avons tenté un correctif générique — faire emprunter le nom de
   la source aux titres non porteurs — **mesuré perdant** (165 chunks touchés,
   `design` −6 pts) et retiré. À reprendre par source. Le fait, lui, est dans
   l'index et citable.
3. **gq7** (coût DMA par octet) et **gq12** (internals de cproc) restent
   `aspirational`.
4. **`sneslab-wiki` pèse 15 % de l'index** pour un trust `complement` dormant.
   Un plafond par source à l'indexation est à mesurer.
5. **Homebrew 100 % assembleur** : cinq sources recensées le 25 (dont la série
   MIT d'`undisbeliever`, qui est par ailleurs l'un de nos arbitres, et un
   JRPG entier en assembleur), **aucune capturée** — la courbe dit qu'ajouter
   des sources est désormais à somme négative, donc elles attendent un besoin
   exprimé. Dites-nous si le chantier GSU en a l'usage.

---

## 10. Ce que votre rapport nous a appris

Deux corrections de contenu que le corpus devait porter et ne portait pas : le
bug SNESMOD et la liste de chips obsolète. Un défaut d'instrumentation que
nous n'aurions pas vu seuls — la méta qui décrit le code plutôt que les
chunks. Et la confirmation que le prior d'autorité fait ce qu'on en attendait :
`fullsnes` répond sur le GSU là où il se taisait le 24.

Savoir qu'il a servi dans du code livré — votre design d'interruptions, les
huit voix débloquées — vaut mieux qu'un point de recall.
