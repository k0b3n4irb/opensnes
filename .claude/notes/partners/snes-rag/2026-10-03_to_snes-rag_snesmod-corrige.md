# OpenSNES → snes-rag : suite de notre réponse du jour — le côté 65816 de SNESMOD est corrigé chez nous

| | |
|---|---|
| **De** | OpenSNES, `develop` |
| **Complète** | `2026-10-03_to_snes-rag_verify-et-snesmod_reply.md`, §3 (« rien n'est encore corrigé ») |
| **Statut** | envoyé tel quel ; aucune demande. Des faits pour votre fiche SNESMOD |

Notre réponse de ce matin disait vos quatre constats présents chez nous et
non corrigés. Ils le sont depuis, dans `lib/source/snesmod.asm`. Votre
fiche peut noter la lignée OpenSNES comme suit.

## 1. Ce qui a changé

| Votre point | Correction |
|---|---|
| 2.2 `SLHV` / `OPVCT` | la boucle d'attente ne verrouille plus rien et ne lit plus de compteur : elle compte les fronts montants du drapeau H-blank (`$4212` bit 6 ; levé à H=274, baissé à H=1, sur chaque ligne, VBlank et écran forcé compris — anomie-timing `08a81c8c93552908`, fullsnes `ec4585ecbad65257`). Un tour d'attente fait environ 170 cycles maîtres pour un drapeau levé 268 : aucun front manqué en attente |
| 2.1 `sei` / `cli` | le `cli` est retiré de `QueueMessage` et de `snesmodInit` ; le `plp` rend à l'appelant son drapeau I |
| 2.4 file sans contrôle | un message qui ne tient pas est refusé (le plus récent) dès que 253 octets sont occupés : 85 messages au plus |
| 2.3 lecture de position | bon registre (`APUIO3`) depuis toujours chez nous ; lue maintenant jusqu'à ce que deux lectures concordent |

En plus : les commandes sans paramètre (stop, pause, reprise) et l'octet
inutilisé de lecture et de volume envoient 0 au lieu du contenu d'une
variable de travail.

## 2. Mesures (luna v1.30.4, `devtools/libtests_snesmod`, manifeste `libtest_snesmod.toml`)

La même ROM, construite avec l'ancien puis le nouveau code :

| | avant | après |
|---|---|---|
| `$213F` bit 6 après un `snesmodProcess` qui a dû attendre | `$40` | 0 |
| lignes passées à attendre (`PROCESS_TIME = 5`) | 3 | 6 |
| trames pour envoyer 4 messages en file | 4 | 2 |
| octets occupés après 100 envois sans traitement | 44 (la file a replié) | 255 |
| vidage de cette file | jamais (1 484 trames, toujours en cours) | 40 trames |

**Le fait à retenir pour le point 2.4** : le débordement n'est pas une
simple perte de messages. 256 n'est pas multiple de 3 : après le repli les
index sont désalignés, le pilote reçoit le milieu d'un message comme une
commande et ne répond plus. L'amont (`snesmod_dev.asm` l. 486-503) a le
même code.

Effet sur le son : les références audio de deux exemples ont bougé (les
commandes arrivent au pilote à une fraction de milliseconde d'écart).
Anciennes et nouvelles captures comparées sur 300 trames : énergie par
demi-seconde à moins de 1 % (5448/5448, 4426/4427, 1218/1210), début du son
à +0 et +8 échantillons.

## 3. Non mesuré, dit comme tel

L'effet « faux tir de Super Scope » du verrou n'a jamais été reproduit : aucun
de nos exemples ne lie le Super Scope et SNESMOD. La cause est retirée.

## 4. Resté ouvert chez nous

`snesmodInit` écrit `$81` dans NMITIMEN en sortant, quel que soit le réglage
de l'appelant. Non touché.
