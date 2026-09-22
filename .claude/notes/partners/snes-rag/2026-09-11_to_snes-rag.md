# OpenSNES → Cartouche (snes-rag) — rapport du 2026-09-11

**De :** OpenSNES SDK (`k0b3n4irb/opensnes`, HEAD `0ffd06bf`, develop)
**Corpus sondé :** état `snes_sources` du 2026-09-10 — 195 sources capturées
sur 218 recensées, 7 arbitres généraux, 9 arbitres de domaine.
**Contexte :** la revue des manques du SDK du 2026-09-11
(`.claude/notes/reviews/2026-09-11_gaps_review.md`, branche `wip/gaps-review`)
a sondé le corpus pour savoir ce qu'un agent de développement y trouve, et
ce qu'il n'y trouve pas. Ce rapport ne porte que sur le corpus : sources à
ingérer, requêtes de validation, et deux remarques d'usage.

**Méthode :** huit requêtes `snes_search` avec
`exclude_sources=["opensnes-docs", "opensnes-notes-tech"]` (obligatoire chez
nous pour toute vérification : nos propres docs sont indexées et
transformeraient la question en boucle de confirmation), `k=3` ou `4`. Une
requête est comptée « trou » quand aucun des passages retournés ne porte sur
le sujet, ou quand seul un tiers (pvsneslib, notre tutoriel) y répond par
ricochet.

---

## 1. Verdict d'ensemble

Le côté **matériel** est couvert et arbitré : PPU, timing, APU/DSP, chips,
mapping, périphériques, suites de test-ROMs. Les requêtes « profilage et
budget CPU par frame » et « suites de test-ROMs » retournent anomie-timing,
fullsnes, snesdev-wiki, tasvideos : rien à ajouter.

Les trous sont côté **toolchain et formats d'assets**, c'est-à-dire là où
un agent qui touche au compilateur, au harness ou aux convertisseurs a
besoin d'un arbitre et n'en a pas.

| requête | résultat | source manquante |
|---|---|---|
| QBE IL : data, phi, appels, agrégats | aucun passage pertinent (top hit : DiscoC, sneslab) | docs QBE |
| cproc : tailles de types, decl, émission des structs | aucun passage pertinent | docs cproc |
| luna CLI/MCP : `--until-frame`, mem-trace, profile, manifests | seulement via `opensnes-docs` (notre tutoriel de debug) | dépôt luna |
| Tiled TMX / export JSON Aseprite | seulement via `pvsneslib` (tmx2snes) et un README de demo | docs Tiled, spec Aseprite |
| ABI des compilateurs C 65816 (Calypsi, WDC816CC, vbcc, ORCA/C, cc65) | nommés par snesdev-wiki, aucun contenu | manuels éditeurs |
| profilage / budget CPU | anomie-timing, fullsnes | — |
| test-ROMs pour la non-régression | snesdev-wiki, tasvideos | — |

---

## 2. Sources à ingérer (par ordre d'utilité pour le SDK)

| # | source | pourquoi | niveau suggéré |
|---|---|---|---|
| 1 | **QBE** — `doc/il.txt`, `doc/abi.txt` du dépôt QBE (notre fork est `compiler/qbe`, upstream `c9x.me/compile`) | zéro résultat ; chaque correction de codegen part de la sémantique IL (phi, sélection, agrégats, `Kl`) — c'est le sujet des deux bugs silencieux ouverts du SDK | arbitre de domaine (compilateur) |
| 2 | **cproc** — `README.md`, `doc/extensions.md`, `doc/c23.md`, `doc/software.md` (`compiler/cproc`, upstream sr.ht/~mcf/cproc) | zéro résultat ; les conventions de `type.c` / `decl.c` gouvernent chaque patch du front-end (tailles `int=2`/`long=4`, structs par valeur) | arbitre de domaine (compilateur) |
| 3 | **luna** — README, `docs/`, CHANGELOG du dépôt `k0b3n4irb/luna` | atteignable seulement à travers notre tutoriel ; la sémantique des flags (`--power-on`, tolérance de `diff`, schéma des manifests `luna test`) est réapprise à chaque session depuis `--help` | arbitre de domaine (outillage) |
| 4 | **Tiled** — format TMX (doc.mapeditor.org, « TMX Map Format ») | seule la copie pvsneslib répond ; les bits de flip du gid et les encodages de calques sont la spec de correction de `tmx2snes` | référence |
| 5 | **Aseprite** — `docs/ase-file-specs.md` du dépôt aseprite | aucune source ; seule autorité sur la sémantique cel / palette derrière l'export JSON que lit `aseprite2snes` | référence |
| 6 | **Calypsi** manuel 65816, **WDC816CC** manuel (WDCTools), backend 65816 de **vbcc** | nommés sans contenu ; les décisions d'ABI en cours (struct par valeur, retour 32 bits) ont besoin de ce que font les pairs | secondaire (comparatif) |
| 7 | `snes-sdk-hecht` — déjà recensée « to-capture » | l'autre SDK SNES à base de cc ; le crt0/runtime le plus proche du nôtre | secondaire |
| 8 | `llvm-mos` — déjà recensée « watch » | backend LLVM famille 6502 ; utile seulement à une future discussion « remplacer QBE » | watch |

Point d'attention pour 1 et 2 : les docs QBE/cproc *upstream* décrivent
l'ABI x86/arm/riscv ; notre backend w65816 s'en écarte (push gauche→droite,
pointeurs 4 octets, `int` 16 bits). Le SDK a `compiler/ABI.md` pour ça, déjà
indexé via `opensnes-docs`. Il serait bon que la fiche source de QBE porte
une **erreur documentée** du type « l'ABI décrite est celle des cibles
upstream ; pour w65816 l'arbitre est `compiler/ABI.md` d'OpenSNES », sinon
une requête ABI risque de retourner la convention x86 avec l'étiquette
arbitre.

---

## 3. Requêtes de validation après ingestion

À rejouer avec `exclude_sources=["opensnes-docs", "opensnes-notes-tech"]` ;
chacune doit retourner la nouvelle source en tête.

1. En QBE IL, comment déclare-t-on un type agrégat et comment le passe-t-on à un `call` ?
2. Quelles règles d'ABI QBE s'appliquent à une cible sans argument passé en registre ?
3. Comment cproc abaisse-t-il une affectation de struct ; où est calculée la disposition (layout) d'un struct ?
4. Que remplit `luna --power-on random=<seed>` et comment la graine est-elle rapportée ?
5. Quel est le schéma des manifests `luna test` ; quelles clés d'assertion existent ?
6. Comment `luna diff --tolerance` apparie-t-il la frame F du ROM A à celle du ROM B ?
7. Dans un TMX, quels bits du gid encodent le flip horizontal, vertical, diagonal ?
8. Comment Calypsi retourne-t-il une valeur 32 bits et passe-t-il un struct par valeur ?

Une neuvième, de contrôle négatif : « convention d'appel de cc65816
(OpenSNES) : ordre de push et taille des pointeurs » doit continuer à
retourner `opensnes-docs` / `compiler/ABI.md` en premier, pas la doc QBE
upstream (cf. point d'attention ci-dessus).

---

## 4. Deux remarques d'usage

**a. Bruit d'arbitrage sur les requêtes toolchain.** Sur « QBE IL », le
bandeau annonce « ⚠️ erreur(s) documentée(s) sur ce sujet : byuu-articles,
discoc-gsu-toolchain » et le top hit est un README DiscoC sans rapport ;
sur « cproc internals », `mame-upd7725` sort en arbitre de domaine (par
collision lexicale sur `uml.h`). Ce n'est pas faux, mais un agent lit
« source faisant autorité : mame-upd7725 » sur une question de compilateur
C. Une fois les sources 1–3 ingérées, ces requêtes auront un vrai arbitre
et le bruit disparaîtra de lui-même ; d'ici là, une réponse « aucune source
du corpus ne couvre ce sujet » serait plus utile qu'un classement de
passages hors sujet.

**b. La liste des sources n'est pas dans notre dépôt.** Notre règle
`hardware_claims.md` impose `snes_search` pour tout claim matériel, mais
rien chez nous ne dit quelles sources existent ni quelles « golden queries »
l'audit de 2026-09 avait retenues (elles vivent dans un artefact externe).
Le SDK va se doter d'une note `.claude/notes/tech/cartouche_corpus.md`
(item D6 de la revue) reprenant la sortie de `snes_sources` et les requêtes
ci-dessus. Si le corpus expose un identifiant de version ou une date
d'index, on aimerait l'y épingler pour détecter la dérive.
