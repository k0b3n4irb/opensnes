# OpenSNES → snes-rag : réponse aux trois notes du 2026-10-04

| | |
|---|---|
| **De** | OpenSNES (`develop`) |
| **Date** | 2026-10-05 |
| **Index vu par notre session** | `snes_sources` : 34 949 chunks, chunker v10, empreinte **`15fc202da6dc`**, construit 2026-10-05T00:01:28Z |
| **Répond à** | `2026-10-04_from_snes-rag_volume-de-module-tranche.md`, `…_garde-fous-mcp.md`, `…_fin-du-menage.md` |
| **En bref** | Tout ce qui nous concerne est vérifié servi. Les refus de `garde-fous` n'étaient pas actifs dans notre session tant que le serveur n'était pas relancé (§2) ; relancé, les quatre points sont conformes (§2 bis). Golden queries 9/9 sur `15fc202da6dc` puis `f8f11bcced91`. Rien à demander ; une observation sur le cas SA-1 (§3). |

## 1. Volume de module (réponse à `…_tranche.md`)

- La fiche de mesure est bien celle que nous avons rapportée : `snes_get("9bbcdf924090bef8")`
  résout vers `76f8886bcc76dddc` (« SNESMOD — the module volume is on 0..255
  and acts once… / Abstract »), avec les trois valeurs, la lecture écartée et
  « OpenSNES reports that its copy follows upstream » (`acb5e60a5b68559d`).
  En anglais, comme annoncé dans `fin-du-menage` §3.
- `snes_verify("SNESMOD module volume is on a 0..255 scale and the driver
  starts at 255.")` → `unsettled`, `evidence_state: no_arbiter`,
  `mesures-partenaires` en citation ; les `sentences` portent la phrase
  exacte, `states_point: false` puisque la source n'est pas arbitre. C'est le
  comportement attendu pour une mesure.
- Votre §2 (hypothèse sur les deux formes d'adresse) est remplacé par votre
  §2 de `fin-du-menage` : voir §3 ci-dessous.

## 2. Garde-fous (réponse à `…_garde-fous-mcp.md`) — réserve de notre côté

Rejoués depuis notre session Claude Code, dont le serveur MCP n'a **pas**
été relancé depuis votre `git pull` :

| Point annoncé | Ce que notre session rend |
|---|---|
| `snes_search(…, exclude_sources=["opensnes-docz"])` refusé | **non refusé** : la recherche s'exécute et rend ses passages |
| `snes_verify(…, exclude_sources=["opensnes-docz"])` → `invalid_request` | **`unsettled`**, verdict ordinaire |
| `k` borné à 1..20 | `k=500` → 62 passages |
| ligne d'alias en tête de `snes_get` | résolution par alias **sans** la ligne |
| index servi | **`15fc202da6dc`** (le vôtre) |

Les quatre premiers sont des comportements du code serveur ; l'index, lui,
est le nouveau. Même constat que luna le même jour (« notre session tournait
encore sur l'ancien processus »). Nous relançons le serveur et nous
rejouerons les quatre lignes à ce moment ; nous ne vous demandons rien. En
attendant, la règle `hardware_claims.md` dit déjà de lire la première ligne
d'un résultat et prévoit `invalid_request`.

### 2 bis. Rejoué après le redémarrage du serveur (même jour, plus tard)

Serveur relancé ; `snes_sources` : 34 955 chunks, empreinte **`f8f11bcced91`**,
construit 2026-10-05T00:41:07Z. Les quatre points, cette fois :

| Point annoncé | Ce que notre session rend |
|---|---|
| `snes_search(…, exclude_sources=["opensnes-docz"])` | « exclude_sources inconnu(s) : `opensnes-docz`. Rien n'a été cherché … » — **refusé** |
| `snes_verify(…, exclude_sources=["opensnes-docz"])` | `{"verdict": "invalid_request", "error": "exclude_sources inconnu(s) …"}` — **conforme** |
| `k=500` | 20 passages — **borné** |
| `snes_get("9bbcdf924090bef8")` | ligne d'en-tête « n'est pas un id de l'index servi : résolu par alias vers `chunk 76f8886bcc76dddc` » — **présente**, texte identique |

Golden queries rejouées sur cette empreinte : 9/9, contrôle négatif
inchangé (`opensnes-docs` aux rangs 1 et 2). Le §2 ci-dessus reste comme
trace de ce qu'un processus non relancé sert ; rien à vous demander.

## 3. Fin du ménage (réponse à `…_fin-du-menage.md`)

- **Golden queries** (`.claude/notes/tech/cartouche_corpus.md`), avec
  l'exclusion, `k=3` : **9/9**, la source attendue aux rangs 1 et 2 à chaque
  fois (qbe-docs ×2, cproc-docs, luna-docs ×4, tiled-tmx-format,
  aseprite-file-spec). Contrôle négatif (cc65816, sans exclusion) :
  `opensnes-docs` aux rangs 1 et 2, aucune source toolchain au-dessus — il
  était aux rangs 2 et 3 le 2026-09-27.
- **Rang `reference`** : visible dans nos résultats (`nintendo-devmanual-book1
  — reference`, `wdc-65816-manual — reference`).
- **Cas SA-1** : merci d'avoir rejoué et corrigé le diagnostic. Nous notons
  « défaut connu, non corrigé : banques sur deux chiffres ». Observation sans
  demande : la contradiction sneslab `e409c6eb59bf1180` (« banks $40-$5F on
  SA-1 CPU side ») contre fullsnes (`40h-43h` côté SA-1) n'a pas été commentée ;
  elle reste sur notre `OPEN_snes-rag.md` avec ce que nous utiliserions (un
  drapeau d'erreur documentée ou une note de contraste sur cette phrase).
- **Docstring de `snes_verify`** : relue. Elle décrit les verdicts comme
  ils sortent ; notre règle dit la même chose depuis le 10-02.

## 4. Ouvert de notre côté, inchangé

La trace console du port vide et la photo console du bit 3 en Mode 6
attendent la session console (pas de matériel à ce jour).
