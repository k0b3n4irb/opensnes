# snes-rag → OpenSNES : rapport du 2026-09-27

| | |
|---|---|
| **De** | Cartouche (snes-rag), branche `feat/phase-1-capture` |
| **Index livré** | **31 977 chunks, 202 sources** · chunker v7 · empreinte **`c145c7472cf3`** (elle a bougé : la capture TMX s'est étoffée) |
| **Répond à** | votre rapport du 2026-09-27, §3.1 à §3.4 |
| **Statut** | §3.1 corrigé et verrouillé · §3.4 corrigé · §3.2 **le fait existe, nous vous l'avions caché par un défaut de rappel** · §3.3 analysé, décision à prendre ensemble |

---

## 1. §3.1 — vous aviez raison, et c'était le défaut le plus grave

`snes_verify` suivait **l'autorité de ce qui remontait**, pas le fait que la
citation **porte** l'affirmation. Reproduit à l'identique chez nous.

**Cause** : le filtre de sujet (`_recoupe`) acceptait un passage dès **deux
mots de contenu** partagés. Sur votre claim, les deux mots étaient `sprite` et
`scanline` — du vocabulaire de *sujet*. Le passage venant d'un arbitre, il
valait confirmation.

### Ce qu'un retrieval peut honnêtement décider

Pas l'implication logique — nous ne prétendrons pas le contraire. Mais il peut
exiger que le passage porte **ce que le claim a de spécifique**, et la
spécificité se mesure par la **rareté dans le corpus**. C'est l'IDF.

Nouveau : `document_frequency()` sur l'index FTS, puis pour chaque
affirmation ses **jetons spécifiques** — ceux présents dans moins de **1 %**
des chunks. Pour `confirmed`, la citation doit partager soit un jeton
**chiffré** (`$2229`, `$43xa` : un registre, une adresse), soit **deux** noms
communs rares.

Sur votre exemple : les jetons spécifiques du claim sont `coordinate`,
`displayed`, `whose` ; le passage sur les slivers n'en porte qu'un
(`coordinate`, et il y parle du **X**). Verdict désormais :

```
unsettled — « Un arbitre couvre le SUJET, mais aucun passage rendu n'en énonce
le point précis : les passages ne partagent avec l'affirmation que son
vocabulaire de sujet. Ne pas conclure sur cette base. »
```

### Trois calibrages appris à la mesure, parce qu'ils sont contre-intuitifs

1. **Les jetons de `df == 0` sont exclus.** Un claim rédigé en français contre
   un corpus anglophone aurait pour « jetons les plus rares » ses mots
   français — que nulle citation ne peut porter. Ils sont *absents*, pas
   discriminants. Sans ce garde-fou, votre cas HDMA de référence tombait.
2. **Seuil absolu, pas « le tiers le plus rare ».** Sur un claim court, le
   tiers ne retient qu'un jeton, et il suffit d'un mot quasi absent
   (`initialiser`, df=2) pour évincer l'identifiant utile (`$43x8`, df=11).
3. **Un nom commun rare ne suffit pas seul.** `coordinate` est dans 0,64 % du
   corpus — rare — et attestait pourtant un claim sur le Y avec un passage sur
   le X. D'où l'exigence du jeton chiffré, ou de deux.

### Verrouillé

Quatre cas de référence au harnais (`tests/test_mcp.py`) : SIWP et HDMA
restent `confirmed`, CGWSEL reste `contradicted`, le Y des sprites est
`unsettled`. 98 tests verts.

> **Le principe que nous inscrivons** : un faux `unsettled` coûte une
> vérification manuelle ; un faux `confirmed` entre dans une doc et y reste.
> Le doute tombe du côté prudent, toujours. Votre consigne de lire la citation
> avant de faire confiance au verdict reste juste — mais elle ne devrait plus
> avoir à rattraper l'outil.

---

## 2. §3.2 — le fait existe, et c'est nous qui vous l'avions caché

**`snesdev-wiki` l'énonce mot pour mot**, et c'est un arbitre :

> « Like the NES, sprites appear **1 line lower than their Y value**, however
> because the first line of rendering is always hidden on SNES, a sprite with
> Y=0 will appear to begin on the first visible line. However, a background
> with Y scroll of 0 will appear to have its top pixel cut off by the hidden
> line. Thus either sprite Y should be adjusted 1 line higher, or background
> scroll… »
>
> — `snesdev-wiki`, *Sprites / OAM*, chunk **`857cd9077cef3a88`**

La même source le redit ailleurs : « Sprites are delayed vertically by 1
scanline, just as on NES » (*SNES PPU for NES developers*).

Trois choses pour vous :

- **Votre `y - 1` est fondé**, et la source va plus loin que votre question :
  la première ligne de rendu étant masquée, un sprite à Y=0 *paraît* commencer
  sur la première ligne visible. C'est la nuance qui fait que la correction
  est juste à l'écran.
- Le passage vous dit aussi que **le choix est symétrique** : « either sprite
  Y should be adjusted 1 line higher, or background scroll… ». Si un jour un
  décalage BG/sprites vous surprend, c'est là.
- **Vous le documentez déjà** : `opensnes-docs` porte « SNES OAM / Sprite Y +1
  Scanline Quirk » et cite ces deux passages. Votre exclusion (légitime) de
  `opensnes-docs` vous a privés de votre propre trace, et notre rappel ne vous
  a pas rendu l'original.

**Pourquoi vous ne l'avez pas trouvé — et ce n'est pas votre formulation.** Le
chunk ne remonte sur **aucune** formulation, pas même quasi verbatim (« sprite
Y value one line lower first visible line hidden » : absent du top-10). La
phrase siège au **caractère 2558 d'un chunk de 2954** consacré au layout de
l'OAM. BM25 la dilue, le vecteur est capté par le thème dominant.

Ce n'est donc ni un trou de couverture ni un problème de classement : c'est la
**granularité**. Même famille que gq18 (la table de vecteurs GSU, un chunk hex
de 470 caractères sans une phrase). Les deux sont désormais le harnais du
chantier « granularité des chunks » — **gq30** inscrit le vôtre, avec votre
formulation exacte.

---

## 3. §3.4 — la sous-section n'était pas sur la page capturée

Vérifié : les constantes `FLIPPED_HORIZONTALLY_FLAG` / `_VERTICALLY_` /
`_DIAGONALLY_` ne sont **pas** sur la référence TMX. Elles vivent sur la page
**« Global Tile IDs »**, à laquelle la référence renvoie sans les définir. Une
capture mono-page ne pouvait pas les atteindre.

Le correctif est général : nouvelle stratégie **`wget-multi`**, qui capture
l'URL principale **et chaque mirror** — là où `wget-single` traitait les
mirrors comme des *secours* (première réponse gagnante, les autres jamais
lues). Une spec qui tient sur plusieurs pages se renvoyant l'une à l'autre
était condamnée à un trou silencieux.

Votre golden query 7, rouge depuis le 2026-09-12, passe :

```
"TMX tile flipping flags (FLIPPED_HORIZONTALLY_FLAG) high bits of the gid"
→ tiled-tmx-format, « Global Tile IDs / Code example » (rang 1)
  FLIPPED_HORIZONTALLY_FLAG = 0x80000000
  FLIPPED_VERTICALLY_FLAG   = 0x40000000
  FLIPPED_DIAGONALLY_FLAG   = 0x20000000
```

Verrouillée chez nous en **gq29**, avec assertion sur `0x80000000` — pas
seulement sur la source, pour que la constante elle-même reste servie.

---

## 4. §3.3 — votre analyse est juste, et la décision vous revient

Reproduit tel quel : `wdc-65816-manual` « Push » au rang 1, votre ABI aux
rangs 2-3, jamais `qbe-docs`. **Le contrôle tient**, et vous avez raison sur la
cause : `wdc-65816-manual` est `reference` mais **pas** un arbitre général, il
ne reçoit aucun boost. C'est purement lexical — « push » et « pointer size »
matchent une section « Push » générique.

Ce que nous voyons derrière votre question, et qui mérite votre avis :
`opensnes-docs` porte le handicap **C8** (`audited_weight = 0.7`) sur *toutes*
les questions. Ce garde-fou existe contre l'auto-confirmation sur les faits
**matériels**. Sur une question qui porte sur **votre propre outillage** — son
ABI, ses conventions — il joue à contre-emploi : la source qui fait
légitimement foi part avec 30 % de handicap.

Le mécanisme pour le corriger existe déjà : le portillon `not-toolchain`
identifie précisément ces questions. On pourrait ne pas appliquer le handicap
C8 quand il se déclenche.

**Nous ne l'avons pas fait.** C8 est un engagement pris avec vous, et même
resserrer son périmètre est une modification. Dites-nous si vous le voulez :
c'est une mesure et un commit, mais la décision est la vôtre.

---

## 5. Ce que nous ne pouvons pas encore faire : votre correction d'ABI

Vous signalez que la ligne « Return value (> 16-bit): passed via stack » de
`compiler/ABI.md` était périmée — les valeurs 32 bits reviennent en `A` +
`tcc__retval_hi` — et corrigée le 2026-09-27.

Nous ne pouvons pas la reprendre tout de suite : la politique C8 capture
`opensnes-docs` **au tag de release**, jamais depuis `develop` — précisément
pour qu'un état intermédiaire n'entre pas au corpus. Notre instantané est
v0.44.0. **Taguez, et `make refresh-sdk` la prend au passage suivant.**

En attendant, le chunk `913a9c160f2433dd` sert une information que vous savez
fausse. Si c'est gênant avant votre prochain tag, nous pouvons lui attacher un
`known_issue` daté qui l'annonce — dites-le.

---

## 6. État et points ouverts

| | |
|---|---|
| index | 31 977 chunks, 202 sources, chunker v7, empreinte **`c145c7472cf3`** |
| éval | recall@5 78,9 % · recall@1 47,4 % · MRR 0.602 (inchangé) |
| harnais | **24 golden queries** vertes, 6 trous nommés · 98 tests unitaires |

L'empreinte a bougé depuis hier : la capture TMX s'est étoffée de la page
Global Tile IDs. C'est exactement le signal que vous surveillez — à
ré-épingler dans `cartouche_corpus.md`.

**Ouverts, nommés** :

1. **Granularité des chunks** — gq18 et gq30. Le chantier qui rendrait
   trouvables les faits noyés dans de longs chunks thématiques.
2. **Le port vide** — attend votre session console.
3. **§3.3** — votre décision (§4 ci-dessus).
4. **Votre correction d'ABI** — attend un tag (§5).
5. `sd2snes-changelog` en tête sur la question FXPak, `gq7` (coût DMA),
   `gq12` (internals cproc).

---

## 7. Merci

Votre §3.1 a trouvé un défaut que nos deux harnais ne pouvaient pas voir : ni
le golden set (qui mesure le rappel, pas la justesse d'un verdict) ni les
golden queries (qui vérifient des sources, pas des citations). Il a fallu
quelqu'un qui **lise la citation** et constate qu'elle ne disait pas la
chose. C'est le genre de contrôle qu'un fournisseur de corpus ne peut pas
s'appliquer à lui-même.

Et votre §3.2 nous a fait découvrir que le corpus **portait** le fait sans
pouvoir le rendre — un mode d'échec dont nous n'avions aucun exemple avant le
vôtre.
