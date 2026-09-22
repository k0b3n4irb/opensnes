# OpenSNES → Cartouche (snes-rag) — validation de la livraison du 2026-09-12

**Référence :** rapport du 2026-09-11 (`opensnes_report_snes-rag_2026-09-11.md`).
**Index vérifié :** 30848 chunks, construit le 2026-09-12T09:12:20Z, chunker v6,
empreinte `e2a738a553ec` — 201 sources capturées sur 223.
**Méthode :** les 8 requêtes de validation + le contrôle négatif, avec
`exclude_sources=["opensnes-docs", "opensnes-notes-tech"]`, `k=3` ; puis des
relances reformulées là où la première passe échouait, pour distinguer un
trou d'ingestion d'une faiblesse de retrieval ; enfin un essai de `snes_verify`.

---

## 1. Livraison — ce qui est là

| demandé | livré | niveau |
|---|---|---|
| QBE `doc/il.txt`, `doc/abi.txt` | ✅ `qbe-docs` | arbitre de domaine |
| cproc README + `doc/` | ✅ `cproc-docs` | arbitre de domaine |
| dépôt luna (README, docs, CHANGELOG) | ✅ `luna-docs` | arbitre de domaine |
| Tiled TMX format | ✅ `tiled-tmx-format` | arbitre de domaine |
| Aseprite file spec | ✅ `aseprite-file-spec` | arbitre de domaine |
| `snes-sdk-hecht` | ✅ capturée | solid |
| Calypsi / WDC816CC / vbcc 65816 | ✗ absentes | — |
| `llvm-mos` | ✗ toujours « watch » | — |

Aussi livré, au-delà de la demande :
- **`snes_verify`** — verdict structuré (`confirmed` / `contradicted` /
  `unsettled` / `not_covered`) avec citation, erreurs documentées et
  `chunk_ids`. C'est exactement le geste de notre règle `hardware_claims.md`.
- **Version d'index exposée** par `snes_sources` (date, chunker, empreinte) —
  la remarque b du rapport précédent. On l'épingle dans notre note de corpus.
- **L'erreur documentée sur `qbe-docs`** (« ABI upstream ; pour w65816 l'arbitre
  est `compiler/ABI.md` ») est en place et remonte bien dans `snes_verify`.

Merci — c'est rapide et propre.

---

## 2. Requêtes de validation — résultats

| # | requête | 1ʳᵉ passe | relance reformulée | diagnostic |
|---|---|---|---|---|
| 1 | QBE IL : agrégats et `call` | ✗ (bsnes licence, wla-dx `.INVOKE`) | ✅ avec « `type :name = { w, l }` / phi » → `qbe-docs` §5, §7 | contenu indexé ; **retrieval sensible au vocabulaire** |
| 2 | QBE ABI, cible sans registres | ✗ (snesdev-abi-v1, DiscoC) | ✗ « call … env … variadic » → wla-dx macros, asar | idem ; collision lexicale « argument / variadic / macro » avec les docs assembleur |
| 3 | cproc : abaissement des structs | ✗ (arbitre affiché : `qbe-docs` abi.txt) | ✅ « extensions et C23 » → `cproc-docs` | **ma requête était mal posée** : les docs cproc ne décrivent pas les internes (`type.c`, `decl.c` sont du source, pas de la doc) |
| 4 | luna `--power-on random` | ✗ (wla-dx `.SEED`) | ✅ « zero ones random seed » → `luna-docs` | contenu indexé ; sensibilité au vocabulaire |
| 5 | schéma des manifests `luna test` | ✅ `luna-docs` | — | OK |
| 6 | `luna diff --tolerance` | ✅ `luna-docs` | — | OK |
| 7 | TMX : bits de flip du gid | ~ (`tiled-tmx-format` en arbitre mais passages `<tileoffset>`, wangtile) | ✗ même avec la phrase exacte de la doc (« The highest three bits of the gid store the flipped states ») → snesdev-wiki tilemaps | **à vérifier : la sous-section « Tile flipping » (sous `<data>`) est-elle capturée ?** |
| 8 | Aseprite : cel et palette | ✅ `aseprite-file-spec` | — | OK |
| 9 | contrôle négatif : ABI cc65816 | ✅ `opensnes-docs` en tête, pas la doc QBE | — | OK |

Bonus vérifié : `luna profile` (cycles par symbole, `--from-frame`, JSON) → `luna-docs` ✅.

**Bilan : 6 requêtes sur 9 passent telles quelles, 2 passent après
reformulation, 1 reste ouverte (TMX flip).** Aucune des sources demandées n'est
absente de l'index ; les échecs de première passe sont du retrieval, sauf #7.

---

## 3. Ce que la livraison nous a déjà rapporté

En cherchant `--power-on`, `luna-docs` répond : « `luna test` manifests take
`power_on = "random"` (+ `seed`, default 1) ». Vérifié sur notre binaire
épinglé v1.18.0 : un manifest avec `power_on = "random"` et `seed = 7` passe,
luna affiche `power-on: random (seed=0x…07)`. **La demande R-A de notre
rapport luna du 2026-09-11 (clé `power_on` dans les manifests) était donc déjà
satisfaite** — nous ne l'avions pas vu parce que `luna test --help` ne liste
pas les clés de manifest. Le rapport luna est corrigé en conséquence. C'est le
cas d'usage exact du corpus : une session sans lui aurait déposé un ticket
pour une fonctionnalité livrée.

---

## 4. Demandes de suite (par priorité)

1. **TMX « Tile flipping »** — vérifier que la sous-section de
   `doc.mapeditor.org/…/tmx-map-format/` (sous `<data>`, les constantes
   `FLIPPED_HORIZONTALLY_FLAG = 0x80000000`, etc.) est dans les chunks de
   `tiled-tmx-format`. Si oui, c'est le cas #2 ci-dessous ; si non, recapture.
2. **Retrieval sur le vocabulaire toolchain.** Les termes « argument,
   variadic, macro, call, seed » sont saturés par les docs assembleur
   (wla-dx, asar) et le bruit domine les arbitres de domaine fraîchement
   ingérés. Deux pistes, au choix : (a) un paramètre `include_sources` /
   `domain` sur `snes_search` pour restreindre à des sources nommées ;
   (b) un boost des arbitres de domaine quand la question contient le nom
   de l'outil (« QBE », « cproc », « luna », « TMX »). Nous pouvons vivre
   avec (b) seul.
3. **`snes_verify` : choix de la citation.** Sur le claim « cc65816 pousse
   les arguments de gauche à droite, pointeurs sur 4 octets », le verdict
   `confirmed` s'appuie sur un passage générique de snesdev-wiki (modes
   d'adressage pile du 65c816) alors que `opensnes-docs` (`compiler/ABI.md`,
   présent dans `chunk_ids`) est la source qui affirme réellement le claim.
   Un `confirmed` adossé à une citation qui ne dit pas le claim est un
   faux sentiment de sécurité ; préférer la citation qui recoupe le plus
   le texte du claim, et rétrograder en `unsettled` si aucune ne le fait.
4. **Manuels Calypsi / WDC816CC / vbcc** — si l'absence tient à la licence
   ou au format, dites-le et nous fermons le point ; sinon ils restent
   utiles pour la décision « struct par valeur » (item C1 du backlog SDK).
5. **`llvm-mos`** — peut rester en « watch », aucune urgence.

---

## 5. Côté OpenSNES (pour information)

- La note `.claude/notes/tech/cartouche_corpus.md` (item D6 de la revue)
  est créée avec la liste des sources, l'empreinte d'index du jour, les
  requêtes de validation et leur statut, pour détecter la dérive.
- Le rapport luna du 2026-09-11 porte une correction sur R-A.
