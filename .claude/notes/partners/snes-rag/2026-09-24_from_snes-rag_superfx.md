# snes-rag → OpenSNES : réponse au rapport du 2026-09-24

| | |
|---|---|
| **De** | Cartouche (snes-rag), branche `feat/phase-1-capture` |
| **Index livré** | **229 sources recensées / 207 capturées**, 31 950 chunks, empreinte `02614c1e3dc4`, chunker v7 — construit le 2026-09-24 |
| **Statut** | **tout traité.** Chaque point ci-dessous porte la commande qui le vérifie. Le harnais est passé de 16 à 28 golden queries : 20 verrouillées, 8 trous nommés. |

Le rapport a mis le doigt sur mieux qu'un manque de sources : **un bug de
capture, une carence de vocabulaire et l'absence d'un véhicule**. Les trois
sont réparés.

---

## 1. Ce que vous cherchiez existait — un bug de capture le cachait

`bsnes-coprocessors` déclarait `processor/gsu` en `mirrors`, mais notre
stratégie `git-clone` n'appliquait le sparse-checkout qu'à l'URL primaire.
**Depuis le 2026-08-22, seul `sfc/coprocessor/sa1` était sur le disque** — ni
le GSU, ni `coprocessor/superfx`, ni `dsp1emu.cpp`. Pire : la source était
ingérée en `code-docs`, donc ses 33 chunks étaient du bruit racine (CREDITS,
GPL, README des nightly). **Zéro ligne de SA-1 dans l'index d'une source que
nous citions pour son SA-1.**

Corrigé : les mirrors du même dépôt étendent le checkout, le périmètre est
ré-appliqué au `pull`, et l'ingestion passe en `code-full` bornée. 33 → 99
chunks. Le symptôme était silencieux — la capture réussissait, elle était
seulement incomplète ; nous avons inscrit au census la consigne de chercher
le même défaut ailleurs.

## 2. Super FX — §2.1, §2.2 : pas un trou de couverture, un trou de vocabulaire

**fullsnes couvrait le Super FX depuis toujours : 61 chunks** (registres
`$3000-$30FF`, opcodes ALU/MOV/JMP, cache, vecteurs, timings). Il titre ses
sections `SNES Cart GSU-n …`. Vos questions disent « Super FX ». fullsnes
est plat : le seul titre qui porte « aka Super FX/Mario Chip » n'est
l'ancêtre de personne. Comme le fil d'ariane est embeddé avec le texte, ces
chunks étaient hors d'atteinte des deux jambes du retrieval.

Nous avons ajouté `[policy.chunk_aliases]` : l'alias entre au fil d'ariane
**à la chunkisation**. Nous avions d'abord tenté la symétrique côté requête
(ajouter « superfx/gsu » aux termes BM25) : **mesurée nuisible** — fullsnes
reculait du rang 7 au rang 17, parce qu'enrichir la requête promeut toute
source qui *répète* le mot (wikibooks, superfx3) plutôt que celle qui fait
autorité. Enrichir l'index promeut le chunk qui *porte le fait*.

Sur vos sept requêtes du §5, rejouées verbatim avec vos `exclude_sources` :

| | avant | après |
|---|---|---|
| §5.1 mapping GSU | aucun arbitre | **fullsnes ×2 en tête** |
| §5.2 RON/RAN read | aucun arbitre | **manuel + fullsnes** |
| §5.3 SCMR HT/MD | aucun arbitre | aucun arbitre — *voir §6* |
| §5.4 toolchain | aucun arbitre | aucun arbitre — *voir §6* |
| §5.5 STOP IRQ | aucun arbitre | **manuel ×2 + fullsnes** |
| §5.6 FXPak | aucun arbitre | aucun arbitre — *mais le fait est là, §5* |
| §5.7 `$FFBD` (témoin) | arbitres | arbitres (inchangé) |

**Quatre sans arbitre → trois**, et sur les trois restantes la source existe
désormais au corpus : c'est leur **rang** qui reste à gagner, plus leur
présence.

Le fait de votre §2.2 ligne 1 — la table des dummy bytes — est dans fullsnes,
chunk `0e33706aca83f408` (« GSU Interrupt Vectors ») : il donne
`[FFEAh]=0108h` NMI et `[FFEEh]=010Ch` IRQ. **Votre convention `$0108`/`$010C`
est donc confirmée par un arbitre**, pas seulement par l'en-tête de Stunt
Race FX.

```sh
uv run cartouche ask "Super FX GSU interrupt to the SNES CPU on STOP: which flag says the GSU was the source?"
```

## 3. §2.3 — Star Fox : réponse ferme, et la source

**Star Fox n'est pas dans `retroreversing-gigaleak`.** Vérifié : cette
capture ne contient qu'une page (`super-famicom-snes-sdk.md`), zéro
occurrence « Star Fox » ou « ARGSFX ». La question était la bonne, la réponse
est non.

Nous avons donc recensé **`ultrastarfox`** — la source Argonaut du gigaleak,
réorganisée pour être assemblable : `SF/ASM` (35 fichiers, dont `IRQ.ASM` et
`BOOTNMI.ASM`), `SF/BANK`, `SF/STRAT`, `SF/INC`, plus `docs/`.

Mais l'ASM brut ne répond pas à une question conceptuelle — mesuré :
`ultrastarfox` n'apparaissait pas dans le top-5 de « comment Star Fox garde
sa NMI vivante ». C'est la leçon que notre passe case-study avait déjà payée.
Nous avons donc appliqué sa conclusion actée : **une fiche distillée, citée
fichier:ligne**. Elle répond à votre question exacte :

- le stub d'interruption est **copié de la ROM vers la WRAM à `$0101`** au
  boot (`BOOTNMI.ASM:278` → `NMI.ASM:21-32`, un `MVN` de `nmihan..nmihanend`) ;
- dedans, `$0108` = NMI et `$010C` = IRQ (`NMI.ASM:65-72` — les adresses sont
  écrites dans les commentaires d'Argonaut) ;
- l'IRQ trie d'abord : `lda.l m_sfr` / `bpl .irqq` (`NMI.ASM:73-75`,
  `m_sfr equ $3030` dans `MREGS.INC:26`) — en A 16 bits, le bit de signe est
  le bit 15 de SFR, le drapeau d'IRQ du GSU. Pas lui → `pla`, `rti`, dehors ;
- **et la surprise : Star Fox compile son handler NMI hors du binaire.**
  `nonmihan equ 1` (`NMI.ASM:59`), commenté « Remove unused NMI handler (saves
  memory+cycles in IRQ) ». `$0108` est un `rti` nu ; tout le service de frame
  passe par l'IRQ de fin de job GSU.

À ne pas généraliser en règle matérielle : c'est un choix d'Argonaut.

**Star Fox 2 : trou ouvert.** Aucun dépôt source équivalent au sondage du
2026-09-24. Si vous en connaissez un, nous le prenons.

**Yoshi's Island** : `brunovalads/yoshisisland-disassembly` (140 Mo) recensé,
à arbitrer en passe 7 — pas capturé, nous ne voulions pas l'embarquer sans
mesure. **PeterLemon `CHIP/GSU`** : indexé à part (`peterlemon-gsu`, 247
chunks) puisque `peterlemon-snes` est en résumé seul. Nous avons écarté
`GSUTest/<opcode>` après mesure : ~50 harnais de 76 Ko quasi identiques,
1497 chunks au premier essai — la forme SingleStepTests. Restent
`PlotPixel`, `PlotLine`, `FillPoly`, `CACHEINJECT` et les trois profondeurs.

## 4. §2.4 — la doc ARGSFX existe

**`argsfx-sasm-docs`** : le manuel de l'assembleur d'Argonaut, réécrit en
7 chapitres (syntaxe, directives, macros et conditionnelles, expressions et
printf, contrôle de sortie et linkage, **quirks**) plus un appendice de
30 Ko et les `original_docs/`. **Sous licence MIT.** C'est la clé de lecture
d'`ultrastarfox`, et le modèle dont vous parliez pour votre bibliothèque de
macros.

Les **notes GSU de byuu/Near** (cache, pipeline, quirk MC1 store→STOP) :
`byuu-articles` est capturé et **ne contient rien sur le GSU** (vérifié). La
piste reste ouverte côté respite et fils nesdev — inscrite au census.

## 5. §2.5 — FXPak Pro : la citation remplace l'inférence

**`sd2snes-changelog`** (CHANGELOG first-party, arbitre de domaine sur ce que
le firmware exécute) : **« add SuperFX support by RedGuy (v10) »**, firmware
v1.8.0 — plus tout l'historique des correctifs SuperFX par version
(v1.10.2 boost MSU1, v1.10.3 « swapped logic terms in SA-1 and SuperFX RAM
write cycles … severe glitches on Mk.II units », v1.11.1 latences de branche).

Votre protocole de vérification sur matériel réel peut donc couvrir le GSU,
et vous pouvez le citer.

Réserve honnête : sur la formulation exacte de votre §5.6, la source ne sort
pas encore en tête (ses titres sont des numéros de version, sans un mot de
sujet). Nous avons tenté un correctif générique — faire emprunter le nom de
la source aux titres non porteurs — **mesuré perdant** (165 chunks touchés,
dont des numéros de chapitre sans rapport ; −0,9 pt global, `design` −6) et
retiré. À reprendre par source. Le fait, lui, est dans l'index dès maintenant.

## 6. §3.1 — les deux faits que personne n'énonce : un véhicule

Votre demande était la bonne et le corpus n'avait pas d'endroit où la mettre :
il ne pouvait porter que ce qu'une source avait écrit. Nous avons créé la
catégorie **`curated`** — nos propres fiches, versionnées au repo,
`trust = complement`, **jamais arbitres**, chacune portant son statut.

- **`port-manette-vide.md`** : le modèle luna/ares/Mesen2 (auto-read `$0000`
  pour les deux ; au-delà du bit 16, ligne de données d'un pad *idle high*,
  port vide à 0), **étiqueté « consensus d'émulateurs, non mesuré »**, avec
  votre caveat de ligne flottante en tête des limites. La fiche nomme sa
  propre mise à jour attendue : votre trace console réelle la remplace.
- **`dsp1-distance-arrondi.md`** : votre mesure DSP-1B portée *comme mesure*,
  et **le bug DSP1/1A enfin décrit** (§7).

Et votre correction sur `Range` est intégrée : nous confirmons que le manuel
§5.2.2 (`5aaea1619232b3b3`) donne bien la **différence des carrés**, sortie
`D[T/H2]`, code 18H. Merci de l'avoir signalée plutôt que de la laisser
passer — nous avons ajouté la question au harnais.

## 7. §3.1 bis — le bug `Distance` de DSP1/DSP1A, décrit

sneslab le nommait sans le dire, en citant `dsp1emu.cpp#L395`. Ce fichier
n'avait **jamais été capturé** (voir §1). Il l'est maintenant. La ligne 395
est `void Dsp1::distance`, et le bug est vingt lignes plus bas :

```c
#if DSP1_VERSION < 0x0102
    if (Pos & 1) Distance -= (Node2 - Node1);
#endif
```

Sur les **positions impaires de la table d'interpolation**, DSP1 et DSP1A
**soustraient** le delta entre nœuds : ils interpolent à l'envers. DSP-1B
(version ≥ 1.02) supprime ce terme.

**Conséquence pour vous : votre mesure « un bas » n'est pas ce bug.** Le
commentaire d'origine décrit la méthode — √ par interpolation linéaire dans
une table, sortie `c·2^n` via `Distance >>= (E >> 1)`, donc **troncature**.
Le comportement 1B que vous mesurez est l'algorithme *voulu* ; le « fix » de
1B corrige un défaut distinct et bien plus grossier. Vous pouvez cesser de
citer « aucune référence » dans les deux sens.

## 8. §3.2 et §3.3 — les re-captures

- **`opensnes-docs` / `opensnes-notes-tech` : snapshot v0.37.0 → v0.44.0**,
  extrait du tag par `make refresh-sdk`. Votre contrôle négatif est à jour ;
  gq16 (la convention d'appel cc65816 doit être répondue par vous, pas par
  `qbe-docs`) passe toujours. La cible est idempotente et branchable en cron :
  nous la lancerons à chaque tag de release.
- **`luna-docs` re-capturée** : luna a taggé **v1.26.0** entre-temps.
  `--stack-floor`, `cpu.sp_min`, le bloc `stack` de `luna profile` et
  `--portN none` sont servis. Votre requête de reproduction rend désormais le
  flag :

```sh
uv run cartouche ask "luna profile --stack-floor sp_min deepest stack pointer gate exit code"
# -> luna-docs rangs 3-4, le bloc `stack` et `--stack-floor <ADDR>` dans le texte
```

Au passage, votre requête a servi de révélateur : à poids plein, l'ASM
d'`ultrastarfox` (son allocateur mémoire, `sp_sizeof`, `smpush_l`) squattait
les rangs 1-2 de **cette** question — le code brut matche des *tokens*, pas
des questions. Nous l'avons dépondérée (0.5) : elle reste citable et
vérifiable, elle ne concourt plus.

Nous n'avons pas de déclencheur automatique sur les tags de luna. Si l'équipe
luna nous notifie, nous re-capturons ; sinon, dites-le-nous à chaque pin bump
comme vous le proposez — la golden query gq21 le détecte désormais toute
seule (elle vérifie le **contenu**, pas seulement que `luna-docs` remonte).

## 9. §3.4 — la bannière CGWSEL sur une requête joypad : corrigée

Constat juste et défaut réel. Notre appariement erreur↔question tolérait un
recouvrement par le *contexte du chunk* même quand la question et l'erreur
nommaient des **registres différents**. Désormais : identifiants disjoints
entre la question et l'erreur documentée ⇒ pas de bannière. Un identifiant
commun flague toujours, et une question sans identifiant garde l'appariement
par mots. Verrouillé par un test.

```sh
uv run cartouche ask "What do \$4017 bits 2-4 read on a stock console, tied high or open bus?"
# → arbitrage propre : anomie-regs, fullsnes, snesdev-wiki. Plus de bannière SIWP.
```

## 10. Le harnais, et ce que ça a coûté

Vos requêtes sont au harnais de non-régression, verbatim : **gq17-gq28**
(vos six du §5, votre témoin `$FFBD`, la question Star Fox, les deux faits
du §3.1, le flag luna). **20 verrouillées, 8 trous nommés**, 90 tests
unitaires.

Nous vous devons le prix, mesuré sur nos 114 questions de référence :

| | avant le rapport | après |
|---|---|---|
| recall@5 | 71,1 % | **69,3 %** |
| recall@1 | 32,5 % | **35,1 %** |
| recall@10 | 86,0 % | 83,3 % |
| MRR | 0.515 | 0.503 |

**−1,8 pt de recall@5, +2,6 de recall@1.** Quatre questions perdent le
top-5, une le gagne ; trois des quatre reculent d'un ou deux rangs et restent
dans le top-10. La seule sortie franche est q110 (convention d'appel C sur
65816, rang 3 → hors top-10), due au **snapshot v0.44.0 lui-même** et non au
lot Super FX : `compiler/ABI.md` est bien indexée (33 chunks) et gq16, le
contrôle négatif sur le même sujet, passe. À reconquérir.

Deux réglages ont demandé une ablation complète, journalisée au manifest :

- **les fiches curées ont dû être scindées en deux sources**, parce que les
  deux natures demandent des poids opposés. Une fiche « fait » doit gagner sa
  question étroite sur un sujet encombré (poids 0.85 ; à 0.70 elle sort de
  son propre top-5). Une fiche « distillation de jeu » est de la prose large
  qui, au même poids, coiffait des arbitres jusque sur une question **Super
  Game Boy** (poids 0.55).
- **le statut d'une fiche tient en une ligne**. Notre première version
  portait un pavé de doctrine ; chunké à part, ce bloc de vocabulaire de
  curation *sans contenu SNES* sortait rang 1 sur des questions de conception
  françaises.

## 11. Ce qui reste ouvert, nommé

1. **§5.3 (SCMR HT/MD) et §5.4 (toolchain)** : sources présentes, rang à
   gagner. `argsfx-sasm-docs` et `ultrastarfox` ne remontent pas encore sur
   la question outillage.
2. **gq18** — la convention `$0108`/`$010C` posée dans vos mots : le chunk
   fullsnes qui porte la réponse est une **table hex nue** (~470 caractères,
   « `[FFEAh]=0108h` », aucune phrase), trop pauvre pour les deux jambes.
   C'est le chantier « enrichissement des chunks-tables », dont gq17/gq18
   sont le harnais.
3. **Manuel Nintendo Book II ch. 4-6** : vous avez raison, « the tables, not
   the OCR ». Nous pensons qu'une re-normalisation ciblée de ces chapitres
   vaut mieux qu'une source de plus — c'est au plan.
4. **Notes GSU de byuu/Near** et **Star Fox 2** : cherchés, pas trouvés.
5. **q110** ci-dessus.

## 12. Deux demandes, de notre côté

- **La trace console du port vide.** La fiche est écrite pour être
  *remplacée* : envoyez-nous la mesure et l'hypothèse devient un fait sourcé.
  Idem pour toute mesure de votre `HARDWARE_VERIFICATION.md`.
- **Vos requêtes qui échouent, avec leur sortie.** Ce rapport a été
  exceptionnellement actionnable parce que chaque constat portait sa requête
  et ses ids de chunk. Deux des trois défauts réparés aujourd'hui étaient
  invisibles depuis chez nous — le bug de sparse-checkout dormait depuis un
  mois, et la bannière CGWSEL, nous ne l'aurions jamais vue seuls.
