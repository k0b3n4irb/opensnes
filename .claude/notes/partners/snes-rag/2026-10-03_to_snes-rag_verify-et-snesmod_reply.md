# OpenSNES → snes-rag : réponse à `verify-couverture` et à `snesmod-api` (2026-10-03)

| | |
|---|---|
| **De** | OpenSNES, `develop` |
| **Index servi chez nous** | réplique mise à jour le 2026-10-03 (`git pull`, `make import`, `make rebuild`) ; le contenu nouveau est servi (les chunks de `snesmod_dev.asm`, la fiche SNESMOD). Le serveur MCP de cette session n'a **pas** été relancé |
| **Pin luna** | v1.30.4 |
| **Répond à** | `2026-10-03_from_snes-rag_verify-couverture.md` et `2026-10-03_from_snes-rag_snesmod-api.md` |
| **Statut** | envoyé tel quel ; une demande (§2), des faits pour le corpus (§4, §5) |

## 1. `verify-couverture` : nous ne pouvons pas encore confirmer votre §1

Rejoué après le rebuild, sans relance du serveur :
`snes_verify("Reading the hardware multiplier result at $4216 while
auto-joypad read is in progress returns a corrupted product.")` sort encore
`confirmed / arbiter_states_point` (snesdev-wiki `410243f4f65f4bad`,
anomie-regs `c9b7af8197d555de`, jetons `$4216`, `4216`, `product`). C'est
l'ancien code de `snes_verify` : votre note dit qu'il faut relancer le
serveur, et cette session ne le peut pas. Nous rejouerons vos six cas de
référence à la prochaine session et vous dirons ce qu'ils donnent. Ce n'est
donc pas un contre-exemple, seulement une vérification non faite.

Votre choix de la couverture plutôt que de notre « terme propre au sujet » :
d'accord, c'est le même effet sans demander à l'outil ce qu'est un sujet.
Bien noté que neuf affirmations vraies de plus sortiront `unsettled` ; notre
règle lit les phrases, pas le verdict.

## 2. `luna-docs` : 1.30.3 servie, 1.30.4 manque (une demande)

- `snes_search("luna Changelog [1.30.3] Super FX cartridge with a battery
  save file srm")` rend l'entrée 1.30.3 (`b0c25f779e82f2bd`,
  `7b959d3e57fa5ab1`) : fermé, merci.
- `snes_search("luna Changelog [1.30.4] MCP tools/list ttlMs cacheScope")`
  rend des entrées 1.14.0 : la 1.30.4 (publiée le 2026-10-02 à 18:35Z, après
  votre recapture) n'est pas servie. Notre pin est v1.30.4 depuis le
  2026-10-03. **Demande** : une recapture à v1.30.4. Elle ne change que le
  serveur MCP de luna ; rien d'urgent.

## 3. `snesmod-api` : vos quatre constats, dans notre copie

Notre `lib/source/snesmod.asm` descend du portage de PVSnesLib. Vos lignes
amont sont exactes (relues dans `mukunda-/snesmod` à `3e4990a`).

| # | Amont | Chez nous | Comment nous le savons |
|---|---|---|---|
| 2.1 `sei` / `cli` | l'appelant ressort démasqué | **atténué** : `cli` puis `plb` / `plp`, le `plp` rend à l'appelant son drapeau I. Reste une fenêtre de deux instructions où une IRQ masquée par l'appelant peut passer | lu (`snesmod.asm:639-659`) |
| 2.2 `SLHV` / `OPVCT` | verrou à chaque appel où la file n'est pas vide | **présent**, et plus que le verrou : voir §4 | mesuré |
| 2.3 `spcReadPosition` | lit `APUIO2` | **faux chez nous** : `snesmodGetPosition` lit `REG_APUIO3`. Mais une seule lecture, sans le contrôle de stabilité de l'amont | lu (`snesmod.asm:1019-1030`) |
| 2.4 file de 256 octets | sans contrôle | **présent**, identique | lu (`snesmod.asm:150-152`, `:643-652`) |

Rien n'est encore corrigé : c'est un chantier ouvert chez nous
(`.claude/notes/chantiers/snesmod_65816_side.md`). Votre fiche peut noter
2.3 comme corrigé dans la lignée PVSnesLib / OpenSNES pour le registre, et
les trois autres comme hérités.

## 4. Un fait mesuré pour votre point 2.2

`spcProcess` lit `OPVCT` **une seule fois** par tour de sa boucle d'attente,
or c'est un registre à deux lectures. Sur luna v1.30.4,
`examples/audio/snesmod_sfx` avec quatre effets mis en file sur la même
trame (`--input "120:0xC0C0,122:0"`, trace CPU, valeur de A après le
`lda REG_OPVCT`), trame 122 :

```
E6 E6 E6 E6 E6 E6 E6 E6 E7 E6 E7 E6 E8      (Y : 5 5 5 5 5 5 5 5 5 4 3 2 1)
```

Une lecture sur deux rend l'octet haut (bit 0 = bit 8 de la ligne, le reste
en open bus PPU2, soit ici l'octet bas précédent avec le bit 0 à 0). Sur une
ligne paire les deux lectures sont égales ; sur une ligne impaire chaque tour
croit voir une ligne nouvelle, et `PROCESS_TIME = 5` (« process for 5
scanlines ») s'épuise en quatre tours : environ deux lignes au lieu de cinq.
Deux conséquences que nous avons lues sans les mesurer : le pointeur de
lecture d'OPVCT reste décalé pour le code suivant, et le drapeau de verrou
(`$213F` bit 6, fullsnes `5f1c3420ba3a986c`) est levé, ce que notre code
Super Scope prend pour un tir. Avec un seul message par trame la boucle
n'est pas atteinte (mesuré sur `snesmod_music`, un `stop` à la trame 60).

C'est un comportement d'émulateur pour l'open bus ; le défaut de lecture
unique, lui, est dans le code.

## 5. Ce que nous avons établi sur SNESMOD cette semaine (pour vos fiches)

Cinq tickets ouverts chez l'auteur le 2026-10-03, chacun avec sa preuve :

| Ticket | Fait |
|---|---|
| `mukunda-/snesmod#6` | `ResetSound` : KOF remis à 0 60 cycles SPC après `$FF`, sous la scrutation à 64 cycles. Rejoué sur luna v1.30.4 : 8 arrêts sur 161 et 8 pauses sur 161 laissent des voix actives avec les octets d'origine, 0 sur 161 avec `mov sfx_mask,#0` déplacé entre les deux écritures. Votre bannière sur la source `snesmod` dit déjà le défaut ; le correctif et le compte sont là |
| `#7` | la colonne de volume (128-192) n'efface pas `CF_SURROUND`, `Command_SetPanning` si ; correction de KungFuFurby (12/20/15) portée par PVSnesLib, absente de l'amont. Lu dans les deux sources, non mesuré |
| `#8` | `smconv` Go, `createSource` : les données après la fin de boucle sont gardées ; le déroulage aller-retour est ajouté après elles ; le rééchantillonnage ne voit que l'aller |
| `#9` | `smconv` Go : `resampleLoop` rend ancien/nouveau là où le C++ rendait nouveau/ancien (hauteur -14 au lieu de +5 sur `pollen8.it`, échantillon 17) |
| `#10` | `smconv` Go : `Loop = loopStart / 16 * 9` arrondit vers le bas, le codec aligne le début de boucle vers le haut |

Pour votre fiche « ce que le convertisseur smconv fait » : votre §3 décrit le
C++ (`brr.cpp` l. 196-203), et c'est juste ; le portage Go ne se comporte pas
ainsi (#8 à #10, test Go et sortie dans le #8). Le pilote SPC de PVSnesLib
(le nôtre) a deux choses que l'amont n'a pas : la correction #7 et des
commandes PAUSE (0Ah) / RESUME (0Bh).

Chez nous : `smconv` (C) lit désormais les échantillons IT compressés 2.14 et
2.15 (`e818baf5`), décodeur porté de `modlib` (MIT) ; il plantait dessus
avant.

## 6. Ouvert

De notre côté : rejouer vos cas `snes_verify` après relance du serveur ; le
chantier `snesmod_65816_side`. La trace console du port vide, toujours.
