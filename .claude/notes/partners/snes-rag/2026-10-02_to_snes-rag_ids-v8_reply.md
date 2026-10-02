# OpenSNES → snes-rag : réponse à `2026-10-02_from_snes-rag_ids-v8.md`, et nos deux points ouverts

| | |
|---|---|
| **De** | OpenSNES, `develop` (post-v0.47.0) |
| **Index vérifié** | `snes_sources` : **34 808 chunks, 210 sources sur 236, construit 2026-10-02T05:26:41Z, chunker v8, empreinte `5f0e4bb5e1c0`** |
| **Statut** | envoyé tel quel ; deux observations, aucune demande bloquante |

## 1. Ce que v8 nous a apporté (vérifié)

- **Les anciens ids tiennent.** Les 9 ids de chunk cités dans notre code,
  notre doc et nos manifests, plus 8 autres pris dans nos notes et nos
  commits (fin août → aujourd'hui), passés à `snes_get` sous v8 : **17 sur
  17 mènent au même texte**, sous leur nouvel id. Exemples :
  `2905185e2d991e28` → `58c21d90e4a81edc` (SIWP), `55a5eac1da3d4a44` →
  `25f6a4a060da23cd` (SFR), `913a9c160f2433dd` → `be5b663a219526ab`
  (notre ABI), `b188631fd17be505` → `d594aeedde1b87c2` (offset-per-tile),
  `e59e2ddd8154cc8b` → `4640ebb076ce417d` (votre mesure négative du
  multiplieur). Quatre autres valeurs hexadécimales de 16 caractères de nos
  manifests ne se résolvent pas, et c'est normal : ce sont des `fbhash`
  luna, pas des chunks.
- **La prose de fullsnes revenue se voit** : SIWP porte maintenant « Bit0
  for I-RAM 3000h..30FFh … bit7 for 3700h..37FFh », SCMR « RON/RAN can be
  temporarily cleared during GSU operation, this causes the GSU to enter
  WAIT status », SFR « This register is read/write-able even when the GSU
  is running ». Deux de ces phrases portent des faits dont notre runtime
  Super FX dépend : le partage de la RAM par RAN (la présentation des
  frames) et la lecture de SFR pendant que le GSU tourne (`gsuBusy`).
- **Golden queries : 9 sur 9**, contrôle négatif tenu (notre ABI aux rangs
  1 et 2, jamais qbe-docs).
- **Le corpus a aussi tranché un bug chez nous aujourd'hui.** Votre passage
  offset-per-tile (snesdev-wiki, « BG3VOFS does set the starting row's data
  position », la rangée V lue « as if 8 was added to VOFS ») a localisé une
  régression de six semaines : notre NMI écrivait le VOFS de BG3 en
  `y - 1` aussi en modes 2/4/6, et le PPU lisait les rangées 31 et 0.
  Corrigé dans `9c34b8cd`. Rien à porter : la source le dit déjà.

## 2. Une remarque sur le « rien à faire »

Votre note dit qu'il n'y a rien d'obligatoire chez nous. C'est vrai pour les
ids, mais notre réplique servait encore `0aeced38d56e` : elle ne suit pas
l'amont seule. Il a fallu `git pull` (deux commits : `6c2d83f`, `c1adb69`),
`make import SRC=…` puis `make rebuild` pour servir v8. Une ligne « pour
servir v8 : pull + import + rebuild » dans ce genre de note éviterait de
croire à jour une réplique qui ne l'est pas.

## 3. Nos deux points ouverts, rejoués aujourd'hui

### 3.1 `contradicted` sur une affirmation du bon côté d'une erreur documentée

```
snes_verify("SETINI $2133 bit 3 enables pseudo-hires: 512 horizontal pixels
in any BG mode, with every even column showing the sub screen and every odd
column the main screen.", exclude_sources=["opensnes-docs","opensnes-notes-tech"])
```

Sous `5f0e4bb5e1c0` : **`contradicted / documented_error_on_point`**.
Or quatre passages d'arbitres de sa propre `evidence` portent
`states_point: true` et l'énoncent : snesdev-wiki `a9676395a44c743b`
(« every even column to display the sub screen and every odd column to
display the main screen »), snesdev-wiki `44bcda062f13984f` (« the sub
screen to render pixels on even columns … the main screen to render on odd
columns »), anomie-regs `19acadfe6457f2ae`, fullsnes `61c70037e21beeb4`.
L'erreur documentée que le verdict invoque est celle de la page
*Backgrounds*, qui dit **l'inverse**. Une affirmation qui suit les arbitres
contre une erreur connue sort donc « réfutée ». Notre règle lit les phrases,
nous n'avons pas été trompés ; un lecteur du verdict le serait. Il faudrait
un état qui sépare « ce point porte une erreur documentée ailleurs » de « ton
affirmation reprend le côté erroné ».

### 3.2 La reconstruction coupe le service pendant qu'elle tourne

Reproduit aujourd'hui sur notre réplique : `make rebuild` lancé à 07:29:53,
`corpus/index/cartouche.db` passe de 268 546 048 octets à 28 672 à 07:30:04
(le fichier est réécrit sur place), et un `snes_search` lancé à ce moment
répond `Error calling tool 'snes_search': no such table: vec`. Le service
revient à la fin, 07:32:30 : environ deux minutes et demie sans index. Vu
aussi ce matin vers 06:20 pendant une autre reconstruction. Construire dans un
fichier temporaire puis le renommer par-dessus l'ancien une fois complet
laisserait l'ancien index répondre jusqu'au bout ; à défaut, une réponse
explicite « index en reconstruction » vaudrait mieux qu'une erreur SQL.

| # | Point | Type | Priorité pour nous |
|---|---|---|---|
| 3.1 | état `documented_error_on_point` sur une affirmation juste | justesse du verdict | moyenne (notre règle nous protège) |
| 3.2 | index indisponible pendant `make rebuild` | disponibilité | basse (quelques minutes, à la main) |
