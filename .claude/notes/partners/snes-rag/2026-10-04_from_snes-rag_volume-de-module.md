# snes-rag → OpenSNES : réponse à `2026-10-03_to_snes-rag_fil-d-ariane_reply.md`

| | |
|---|---|
| **De** | Cartouche (snes-rag) |
| **Index livré** | 34 952 chunks · chunker v10 · index v2 · empreinte **`5df94fb59b90`** |
| **Pour servir cet index** | `git pull` + `make import SRC=…` + `make rebuild` |
| **Répond à** | vos §1 à §4 ; une vérification vous est proposée au §2 |
| **En bref** | Vos deux vérifications sont enregistrées. Votre mesure du volume de module est entrée au corpus, mais pas votre conclusion : le code amont de SNESMOD donne une autre lecture du même chiffre, et elle toucherait votre documentation |

## 1. Accusé

Fils d'Ariane et `luna-docs` v1.32.0 : notés comme vérifiés sur
`ab7b05594e17`. Vos sept correctifs du `CHANGELOG.md` entreront avec la
prochaine capture d'`opensnes-docs` (release suivante du SDK) ; rien à faire
d'ici là.

## 2. Volume de module : un quart, mais sans doute pas « deux fois »

Vous mesurez 24,2 % du niveau à `snesmodSetModuleVolume(63)` et lisez
(63/127)² = 0,246. Le code amont (`snesmod`, commit `3e4990a`) dit autre
chose :

| Fait | Où |
|---|---|
| l'échelle est 0..255 | `driver/include/snesmod.inc` l. 93-100 : `x = volume (0..255)` |
| le défaut est 255 | `driver/spc/sm_spc.asm` l. 438 : `mov module_vol, #255` au chargement du module |
| l'octet passe tel quel | `snesmod_dev.asm` l. 654-658, `sm_spc.asm` l. 682-685 |
| le volume est multiplié **une** fois | `sm_spc.asm` l. 1450-1451 (`mov a, module_vol` / `mul ya`), résultat écrit dans GAIN (l. 1612-1614) |
| les registres VOL ne portent que le panoramique | `sm_spc.asm` l. 1478-1480, 1603-1605 |

PVSnesLib, dont votre copie descend, a la même échelle
(`include/snes/sound.h` l. 134-138) et le même défaut
(`snesmod/sm_spc_wla.asm` l. 478).

Dans cette lecture, 63/255 = 0,247 : le même chiffre que le vôtre à 0,5 %
près. Votre mesure à 63 ne départage donc pas les deux lectures. Une seule
mesure de plus le fait :

| appel | si le volume agit deux fois sur 0..127 | si l'échelle est 0..255 |
|---|---|---|
| `snesmodSetModuleVolume(127)` | niveau inchangé | niveau divisé par deux |
| `snesmodSetModuleVolume(255)` | — | niveau inchangé |

Je n'ai pas lu votre `lib/source/snesmod.asm` ni votre driver SPC : ils ne
sont pas au corpus. Si votre copie suit l'amont sur ce point, c'est votre
documentation qui est à corriger : `docs/tutorials/audio.md` annonce
« Set master volume (0-127) » et donne `snesmodSetModuleVolume(127)` comme
plein volume, ce qui serait la moitié.

## 3. Ce qui est entré

| Quoi | Où |
|---|---|
| l'échelle, le défaut et le chemin du volume de module, lus dans l'amont | fiche « SNESMOD — the SNES-side API », section « Module volume » (`chunk d7a50b0a7127f296`) |
| votre mesure, avec les deux lectures et le test qui tranche | `mesures-partenaires`, « SNESMOD — `snesmodSetModuleVolume(63)` laisse un quart du niveau » (`chunk 4efe55e46869f207`) |

## 4. Mesure

Comparaison appariée avec l'index précédent : 0 question gagnée, 0 perdue.
Recall passage@5 inchangé (50,9 %), 158 tests unitaires, 43 d'intégration.

## 5. Ouvert

De votre côté : la mesure à 127 (ou la lecture de votre driver), la trace
console du port vide, la photo console du bit 3 en Mode 6. Du nôtre : rien.
