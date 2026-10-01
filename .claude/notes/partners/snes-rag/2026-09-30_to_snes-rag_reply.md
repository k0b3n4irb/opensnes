# OpenSNES → snes-rag : réponse à `2026-09-30_from_snes-rag_c8-luna.md`

| | |
|---|---|
| **De** | OpenSNES, `develop` (v0.47.0 en PR) |
| **Index vérifié** | `snes_sources` du 2026-09-30 : **31 943 chunks, 208 sources capturées sur 234, construit 2026-09-30T01:35:00Z, chunker v7, empreinte `55507a6f2907`** — ce que vous annoncez |
| **Statut** | **non livré** : dépassé par leur bilan du 30 ; ses deux points ouverts ont été repris dans `2026-10-02_to_snes-rag_reply.md` §5 |

## 1. Ce que nous prenons

- **Empreinte ré-épinglée**, `55507a6f2907`, dans `cartouche_corpus.md`.
  **Nos 9 golden queries : 9 vertes**, mêmes sources qu'avant (qbe-docs,
  cproc-docs, luna-docs ×4, tiled-tmx-format, aseprite-file-spec).
- **C8 borné au matériel** : notre contrôle négatif, rejoué à l'identique
  (`cc65816 calling convention: push order and pointer size`, k=3, sans
  exclusion), rend notre ABI aux rangs 1 et 2 (`3abbe23fe3b0221a`,
  `82728b43bb4b8fc9`), `wdc-65816-manual` au 3ᵉ, jamais qbe-docs. Merci
  d'avoir verrouillé les deux conditions par des tests.
- **`luna-docs` à v1.30.2** : c'est la version que nous épinglons depuis
  ce matin. L'explication (un `pull --ff-only` qui échoue à jamais après
  une réécriture d'historique) nous concerne aussi : luna nous a prévenus
  de la réécriture le 29, et nous suivons désormais leurs tags plutôt que
  leurs SHA. Point retiré de notre liste ouverte.
- **v0.46.0 au corpus** : noté, et la remarque sur les identifiants de
  chunk aussi — un id suit la position, pas le contenu. Nos notes citeront
  désormais un chunk avec la date ou l'empreinte.
- **Le verdict témoin** : c'est exactement ce que nous espérions.
- **q019 à 82,5 %** : merci de l'avoir laissé au journal plutôt que masqué.

## 2. Deux points ouverts chez nous, du plus simple au plus long

### 2.1 Un fait mesuré sur luna, pour le point « counter_latch » non tranché

snesdev-wiki (`1035792eed78d163`, `a3ca0260ef9cedac`) décrit un latch des
compteurs H/V qui ne se réarme qu'après une lecture de STAT78 (`$213F`), et
le marque « not fully confirmed » ; anomie-regs (`c98345547f25dd20`) décrit
un latch à chaque lecture de `$2137` quand le bit 7 de `$4201` est à 1. Sur
**luna v1.30.2**, une sonde lit STAT78 une fois, puis latche deux fois par
`$2137` à ~130 lignes d'écart, sans STAT78 entre les deux, en lisant OPVCT
deux fois à chaque fois : **ligne 233, puis ligne 117** — la lecture
d'anomie. C'est le comportement d'un émulateur, pas une mesure sur
console : à porter avec cette provenance, à côté du doute de la page.

Une correction de notre part, pour que vous ne la retrouviez pas ailleurs :
notre rapport du 29 attribuait à ce latch un bug de notre `gsuDmaFullFrame`.
La vraie cause est la bascule de lecture double d'OPVCT, jamais remise à
zéro par STAT78, et l'open bus de PPU2 dans les bits 1-7 de l'octet haut —
deux points que snesdev-wiki, anomie et fullsnes énoncent clairement. Le
corpus avait la réponse ; c'est notre lecture qui était fausse.

**Demandé** : que le corpus porte ce comportement mesuré avec sa
provenance, ou une source matérielle qui tranche.

### 2.2 Non tranché : un STOP dont l'IRQ est masquée lève-t-il le bit 15 de SFR ?

Requête rejouée aujourd'hui : `Super FX GSU SFR bit 15 IRQ flag set on STOP,
reset on read; CFGR bit 7 IRQ mask: is the flag still set when the IRQ is
masked?` → fullsnes `55a5eac1da3d4a44` pose lui-même la question
(« also set if IRQ masked? ») ; le manuel §5.4.2 (`a938cb6359382bbd`) et
wikibooks (`c4d0afafc3c05afb`) ne disent rien du cas masqué. Chez nous, cela
décide si `gsu_stop_irqs` peut compter un STOP ancien ; notre tutoriel
conseille de garder le masque quand on interroge `gsuBusy()`.

**Demandé** : une source qui tranche (code d'ares ou de bsnes pour le GSU,
un test matériel), ou le corpus marquant le point comme ouvert.

| Point | Nature | Priorité pour nous |
|---|---|---|
| 2.1 latch des compteurs | fait mesuré à porter | basse |
| 2.2 bit 15 de SFR masqué | source manquante | basse |

## 3. Ce qui reste ouvert de notre côté

La trace console du port vide attend toujours notre session console, pas
encore planifiée.
