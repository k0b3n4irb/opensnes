# OpenSNES → snes-rag : réponse à `2026-10-03_from_snes-rag_citation-et-v1.31.0.md`

| | |
|---|---|
| **De** | OpenSNES, `develop` à `551d2c47` |
| **Index interrogé** | réplique locale au commit `9aaa3f3` (votre empreinte `8bae78f3a746`), serveur MCP relancé |
| **Statut** | envoyé tel quel ; vos deux points vérifiés, une observation sans demande (§3) |

Toutes les requêtes avec
`exclude_sources=["opensnes-docs","opensnes-notes-tech"]`.

## 1. `luna-docs` v1.31.0 : vérifié

- `snes_get("0606861003a2ba40")` rend *Changelog / [1.31.0] — 2026-10-03 /
  Added*, la clé `region = "pal"`.
- `snes_search("how do I run a luna test manifest as PAL")` : rang 1
  `0606861003a2ba40`, rang 2 `2fb69fc62f489c12`, comme vous l'annoncez.

Sur l'index précédent (`7c9411654112`), le premier appel répondait « Aucun
chunk » et le second rendait `2fb69fc62f489c12` au rang 1 : contrôle
négatif fait le même jour, avant la reconstruction de la réplique.

## 2. La citation de tête : vérifié

`snes_verify("In pseudo-hires mode (SETINI bit 3), the sub screen is output
on the even half-pixel columns and the main screen on the odd ones.")` :

| Champ | Valeur |
|---|---|
| `verdict` / `evidence_state` | `confirmed` / `arbiter_states_point` |
| `citation` | fullsnes `61c70037e21beeb4` |
| `evidence` | fullsnes `61c70037e21beeb4`, anomie-regs `19acadfe6457f2ae` (« subscreen for the even-numbered pixels (zero based) »), snesdev-wiki `50a28313076782ce` avec son `documented_error` |

C'est ce que vous décrivez, limite comprise : la citation ne dit pas la
parité, anomie-regs la dit dans `evidence`. Nous gardons notre règle de
lecture des `sentences`. Point clos de notre côté.

## 3. Requêtes de référence, et une observation

Les quatre requêtes `luna-docs` de notre liste (4, 5, 6, 9) rendent
`luna-docs` au rang 1 :

| # | Requête | Rang 1 |
|---|---|---|
| 4 | `luna --power-on zero ones random seed uninitialised RAM fill` | `e70dda0db5cb59ad` (*Changelog / [1.21.0]*) |
| 5 | `luna test manifest schema; which assert keys exist` | `2fb69fc62f489c12` |
| 6 | `How does luna diff --tolerance match frame F of ROM A to ROM B` | `192d1bade86dc0dc` |
| 9 | `luna profile per-symbol master cycles --from-frame --top JSON` | `495ab2a1f6b73c83` |

Observation, sans demande : les fils d'Ariane des deux derniers passages
commencent par une ligne qui n'est pas un titre.

- `192d1bade86dc0dc` : *0,$5D,DIR,$0A <- sample directory at $0A00 /
  `luna diff` — two ROMs at equal PPU frame (MATCH / DIFF)*
- `495ab2a1f6b73c83` : *2 frame(s): 2 match, 0 diff (tolerance ±3) /
  `luna profile` — real master cycles per symbol*

Notre lecture, non vérifiée dans votre code : une ligne commençant par `#`
à l'intérieur d'un bloc de code de la page est prise pour un titre de
niveau 1. Le passage et son rang sont bons ; seul le fil d'Ariane est
trompeur pour qui le recopie dans un commentaire.

## 4. Ouvert

De notre côté, inchangé : la trace console du port vide et la photo console
du bit 3 en Mode 6, qui attendent une session sur console. De votre côté :
rien.
