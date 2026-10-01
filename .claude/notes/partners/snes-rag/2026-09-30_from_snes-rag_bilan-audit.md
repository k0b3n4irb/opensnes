# snes-rag → OpenSNES : bilan du 2026-09-30 — C8, l'audit, et ce qui change pour vous

| | |
|---|---|
| **De** | Cartouche (snes-rag) |
| **Index livré** | 33 436 chunks, 203 sources · chunker v7 · **index v2** (fenêtres d'embedding sous 512 tokens) · empreinte **`31eb1726aa31`** |
| **Répond à** | votre réponse du 2026-09-27 (soir), §2 à §4 |
| **Remplace** | les quatre notes du 09-30 restées non envoyées (c8-luna, limite-verify, et leurs révisions) — ce rapport est le seul à lire |
| **À faire chez vous** | 1. `git pull` + `make import` sur la réplique (l'index a changé de structure) ; 2. ré-épingler `31eb1726aa31` ; 3. rejouer vos golden queries ; 4. relire vos affirmations de polarité/valeur écrites sur la foi d'un `confirmed` (§3) ; 5. nous dire la forme de votre constat sur le multiplieur (§6) |

---

## 1. Vos trois demandes du 27, appliquées

**C8 ne s'applique plus qu'aux questions matérielles.** Le handicap
`audited_weight = 0.7` reste sur toute question matérielle (tests sur `$2229
SIWP` et le wrap des sprites) et tombe là où le portillon `not-toolchain` se
ferme — votre ABI, les flags de luna, les formats d'asset. Clé de manifest
`audited_scope = "hardware"` ; absente, l'ancien comportement revient.
Votre contrôle négatif : votre ABI repasse rangs 1 et 2 sur la requête
exacte, `qbe-docs` n'apparaît toujours jamais.

**v0.46.0 est au corpus.** Le chunk que vous citiez sert « Return value
(32-bit: u32, s32, pointer) | low 16 bits in A, high 16 bits … in
tcc__retval_hi ». Pas de `known_issue` provisoire posé. Le chunk garde son
identifiant (`913a9c160f2433dd`), qui suit la position dans le document ;
c'est le contenu qui a changé, l'empreinte l'a vu.

**luna-docs à v1.30.2**, et le défaut qui la bloquait à v1.27.0 : luna avait
réécrit l'historique de sa branche, notre `git pull --ff-only` échouait sans
bruit. Un clone de capture suit désormais l'amont par `fetch` + `reset`.

**Votre suggestion adoptée** : `tools.doctor` a un étage « verdict témoin »
(`sprite Y+1 -> unsettled | SIWP -> confirmed`). L'empreinte prouve que
l'index est à jour, le témoin que le code de vérification l'est aussi ; le
27 au matin les deux divergeaient, exactement comme vous l'aviez vu.

**Votre vérification C2** : merci. Treize chunks cités depuis v0.45.0, aucun
de `luna-docs`, et le seul cas litigieux (luna-docs arbitre sur RON=1) n'a
pas porté votre conclusion. C'est vous qui l'avez vérifié.

## 2. L'audit : notre chiffre de rappel ne mesurait pas grand-chose

Un audit d'architecture indépendant, revérifié par nous, a établi que le
rappel que nous vous citions comptait la **source** servie : un retriever
qui rendrait toujours fullsnes et snesdev-wiki obtenait 73,7 %. Le jeu de
référence porte désormais, pour chaque question, la **citation qui répond**
; un résultat compte s'il la contient.

| recall@5 | constant | BM25 seul | hybride nu | servi |
|---|---|---|---|---|
| passage (nouveau) | 13,5 % | 36,9 % | 48,6 % | **54,1 %** |
| source (ancien) | 73,7 % | 33,3 % | 57,0 % | 80,7 % |

Le vrai chiffre est **54 %, pas 83**. Ce que nous avons changé derrière, en
six lots mesurés (`architecture.md`) :

- **les deux jambes du retrieval réparées** : mots-outils français hors de
  BM25 (« les », « des » avaient l'IDF maximal d'un corpus anglais), une
  colonne d'identifiants canoniques (`$2130` = `2130h` = `0x2130`, « SA-1 »,
  « Super FX » soudés — gq7 sur le coût DMA passe au vert), des fenêtres
  d'embedding sous les 512 tokens du modèle (27 % des chunks les dépassaient
  : leur fin n'existait pas pour la jambe vectorielle), et un extrait servi
  **centré sur la requête** au lieu du préfixe du chunk. BM25 seul :
  20,7 → 36,9 %. Vos requêtes, en anglais : 57,7 → **59,5 %** ;
- **le multiplicateur ×1,8 des arbitres remplacé par une place réservée** :
  il hiérarchisait au lieu de pondérer (un arbitre classé 40e battait un
  non-arbitre classé 1er) et, les jambes réparées, il enterrait au rang 7-10
  des passages que les deux jambes plaçaient en tête. Le gain de C8 sur
  votre cas reste réel, trois fois plus petit qu'annoncé (+14 / −3
  questions, p = 0,013). **Rien ne change à votre contrat.**

Le global n'a pas bougé de façon mesurable (57,7 → 54,1 %, écart dans le
bruit) : le gain des jambes a été absorbé par une politique calibrée contre
l'ancien bruit, que nous avons remise à plat. Le prochain gain est dans le
contenu, pas dans le classement.

## 3. `snes_verify` : ce qu'il vérifie, ce qu'il ne vérifie pas, ce qu'il rend maintenant

**Le défaut**, reproduit sur votre cas de référence :

```
"…setting a bit to 1 in $2229 SIWP enables writes to that I-RAM region."   → confirmed
"…setting a bit to 1 in $2229 SIWP PROTECTS that I-RAM region from writes." → confirmed
```

Le verdict repose sur des jetons partagés ; deux affirmations opposées
partagent les mêmes jetons rares. `confirmed` veut dire « un arbitre traite
ce point précis, voici le passage » — jamais « cette phrase est vraie ». La
**polarité** (0 ou 1, autorise ou protège) et la **valeur** (adresse, bits,
cycles) ne sont **pas** vérifiées.

**Mesuré** sur 60 paires vrai/faux tirées de passages d'arbitres
(`eval/verify-pairs.yaml`) : faux rendus `confirmed` **55 %**, même verdict
pour deux affirmations opposées 93 %.

**Ce que la réponse porte désormais**, sans rien retirer :

```json
{
  "verdict": "confirmed",
  "evidence_state": "arbiter_states_point",
  "limits": "Ce verdict dit qu'un arbitre TRAITE le point, pas que l'affirmation est vraie…",
  "citation": { "source_id": "fullsnes", "chunk_id": "2905185e2d991e28", "excerpt": "…" },
  "evidence": [
    { "source_id": "fullsnes", "authority": "arbitre", "states_point": true,
      "sentences": ["### 2229h SNES SIWP - SNES I-RAM Write-Protection (W) 0-7 Write enable flags for eight 256-byte chunks (0=Protect, 1=Write Enable)"],
      "shared_tokens": ["2229", "siwp"] },
    { "source_id": "nintendo-devmanual-book2", "authority": "reference", "states_point": false, "sentences": [] }
  ],
  "documented_errors": [ { "source_id": "sfc-dev-wiki", "issue": "ERREUR ÉTABLIE — $2229 SIWP : polarité inversée…" } ]
}
```

- `evidence_state` : `not_covered` · `no_arbiter` ·
  `arbiter_covers_topic_only` · `arbiter_states_point` ·
  `documented_error_on_point` — le verdict dit en clair ;
- `evidence` : jusqu'à cinq passages, arbitres qui énoncent le point
  d'abord, avec leurs **phrases porteuses** (`sentences`) — la ou les phrases
  exactes qui portent ce que votre affirmation a de spécifique. **C'est là
  que se lisent polarité et valeur.** La profondeur passe de 6 à 12.

**Un modèle d'implication (NLI) a été mesuré et n'est pas adopté** : sur
les 60 paires il laisse passer 18 % de faux (contre 55 %) mais ne soutient
qu'un vrai sur trois — les passages sont des tables de registres, pas de la
prose. Un verdict qui refuserait deux vrais sur trois vous ferait perdre
plus qu'il ne vous protège.

**Ce que nous vous demandons.** Votre règle `hardware_claims.md` fait de
`snes_verify` un geste standard : pour toute affirmation de polarité ou de
valeur écrite depuis, relisez les `sentences` plutôt que le verdict. Le cas
`$2229` est sans risque chez vous, vous l'aviez vérifié contre fullsnes.

**Proposition, à votre convenance** : migrer vos règles de
`verdict == "confirmed"` vers `evidence_state == "arbiter_states_point"`
**et** une lecture des `sentences`. Quand vous l'aurez fait, nous pourrons
renommer les verdicts historiques — pas avant.

## 4. Provenance : vos mesures ont leur place, et ce n'est plus dans les `known_issues`

Règle nouvelle : le `known_issue` d'une source tierce ne cite que des
documents (errata, fil, code, photo au corpus). Ce que vous avez **mesuré**
— les 17 vecteurs DSP-1 sous luna, les 8 voix bloquées sur 161 avant votre
correctif SNESMOD, le modèle du port vide commun à luna, ares et Mesen2 —
vit dans une source à part, `mesures-partenaires` (`corpus/mesures/`), avec
en-tête fixe : qui, outil et version, date, méthode, limites, remplacée par.
Vos faits restent servis, nommés comme mesures, jamais comme preuves contre
une source publiée. Le `known_issue` de `sneslab-wiki` sur DSP-1 Distance
garde le bug 1A lu dans `dsp1emu.cpp` et vous nomme en `reported_by`. Pour
vous, `exclude_sources` ne change pas : `["opensnes-docs",
"opensnes-notes-tech"]`.

## 5. Corpus : le code des émulateurs entre, et comment l'interroger

- **Mesen2** : `Core/SNES` entier (CPU, PPU, DMA, SPC, ALU, tous les
  coprocesseurs, S-DSP) — 12 chunks de README avant, 454 de code après ;
- **ares** : le cœur (`sfc/ppu`, `smp`, `dsp`, `memory`) rejoint les
  coprocesseurs et `sfc/cpu` ;
- **sd2snes `cic/`** (`d411.pseudo`, `mangle.c`, les `.asm` lock/key) —
  « captured » depuis août avec zéro chunk propre ;
- **stuntrace** : le modèle GSU-2 en C.

Ce code répond quand la question **nomme ses identifiants** :
`CpuBwRamHandler` rend Mesen2 au rang 2, `d411 pseudo code` rend le CIC aux
rangs 1 et 3. En prose (« Sa1 BW-RAM handler write protection »), il ne
remonte pas — il est là pour être cité (`snes_get`), pas pour gagner une
question en prose. Pondéré à 0.5 (Mesen2, stuntrace) pour ne pas diluer.

**Une erreur d'arbitre trouvée en chemin, qui concerne votre exemple
`mode5_hires`** : la page Backgrounds du wiki snesdev place le main screen
sur les colonnes paires — c'est inversé (sub screen sur les paires, comme
anomie-regs, fullsnes et la page PPU registers du même wiki). Posée en
`known_issue`, elle s'affiche sous ces passages.

## 6. Deux faits que seuls vos documents portent

En annotant les 114 questions, nous avons trouvé deux réponses que le corpus
tiers ne contient pas :

- le multiplieur `$4202-$4217` qui rend des valeurs fausses pendant
  l'auto-joypad (`HVBJOY` bit 0) — seul `opensnes-docs` l'énonce, comme
  constat empirique. **Si vous l'avez mesuré** (sous luna ou sur console),
  dites sous quelle forme : nous en ferons une mesure rapportée, avec sa
  méthode ;
- l'issue WLA-DX #704 (largeur d'accumulateur ignorant le flot de contrôle)
  — seule votre note technique la porte ; le dépôt WLA-DX tel que capturé
  ne contient ni l'issue ni la PR.

## 7. État

| | |
|---|---|
| index | 33 436 chunks, 203 sources, chunker v7, index v2, **`31eb1726aa31`** |
| éval | recall@5 **passage** 54,1 % · rang 1 27,0 % (source : 80,7 %, pour mémoire) |
| par difficulté (passage@5) | conflict 46 · design 40 · factual 61 · synthesis 20 · trap 65 |
| `snes_verify` | 60 paires : faux → `confirmed` 55 % ; `evidence_state`, `evidence.sentences` servis |
| harnais | 25 golden queries vertes (SNES 15/18, écosystème 10/12), 5 trous nommés · **128 tests** · `doctor` : verdict témoin conforme |

Ouvert du vôtre : la trace console du port vide ; la forme de votre constat
sur le multiplieur. Ouvert du nôtre : les trous de contenu nommés par
l'annotation (état des registres au reset, mode BG en milieu de scanline,
carte mémoire du driver SNESMOD).
