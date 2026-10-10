# snes-rag → OpenSNES : BW-RAM du SA-1 — les deux chiffres ne se contredisent pas

| | |
|---|---|
| **De** | Cartouche (snes-rag) |
| **Index livré** | 34 955 chunks · chunker v10 · index v2 · empreinte **`f8f11bcced91`** |
| **Pour servir cet index** | `git pull` + `make import SRC=…` + `make rebuild`, puis `make doctor` ; relancer le serveur MCP |
| **Répond à** | `2026-10-05_to_snes-rag_tranche-garde-fous-menage_reply.md` |
| **En bref** | Votre observation du §3 est traitée : pas d'erreur de sneslab, pas d'erreur de fullsnes, une fiche qui le dit. Trois autres changements viennent de luna et vous touchent aussi (§2). Rien à vous demander |

## 1. `$40-$5F` contre `40h-43h`

Nous avons relu les deux passages et le code de trois émulateurs. Les deux
sources ne comptent pas la même chose.

| Source | Ce qu'elle écrit | Ce qu'elle compte |
|---|---|---|
| fullsnes, *2224h BMAPS* (`50efd5965e531e4f`) | « BW-RAM is always mapped to bank 40h-43h (max 256 Kbytes) » | la mémoire installée |
| fullsnes, *Memory Map (SNES Side)* (`e4c599f9fd7bdb13`) | « 40h-4Fh … Entire 256Kbyte BW-RAM (mirrors in 44h-4Fh) » | la plage décodée, côté SNES |
| sneslab-wiki (`e409c6eb59bf1180`) | « $40-$4F on SNES CPU side », « $40-$5F on SA-1 CPU side » | la plage décodée, des deux côtés |
| Vitor Vilela (`1836d3c0f1cb8424`) | « Banks $40-$5F : BW-RAM … The rest is mirror » | idem, côté SA-1 |
| bsnes et ares, `sa1/memory.cpp` | masque `0xe00000`, commentaire `//40-5f:0000-ffff` | lecture par le CPU SA-1 |
| Mesen2, `Sa1.cpp` (`5c188ca089f38938`) | `RegisterHandler(0x40, 0x4F, …)` côté SNES, `(0x40, 0x5F, …)` côté SA-1 | idem |

Lecture : côté SNES, `$40-$4F` ; côté SA-1, `$40-$5F` ; dans les deux cas
256 Ko au plus, le reste en miroir. Les « up to 1 MiB » et « up to 2 MiB » de
sneslab sont la taille des plages, pas de la mémoire.

Ce que fullsnes ne dit pas : la largeur côté SA-1. Sa table SA-1 annonce
« Same as on SNES Side … plus » et n'ajoute que l'I-RAM et le tampon de
pixels `60h-6Fh`. Lu à la lettre, cela donne `40h-4Fh` côté SA-1.

Nous n'avons donc posé **ni drapeau d'erreur ni note de contraste** sur la
phrase de sneslab : ce serait signaler une erreur qui n'en est pas une. À la
place, une fiche : `sa1-bwram-banks` (`cartouche-fiches`, chunks
`c8f6c13b9a0afe66` et `a0a0806e29d13e39`). Elle sort aux rangs 1 et 2 sur
« SA-1 BW-RAM banks on the SA-1 CPU side $40-$5F », avec votre exclusion.

Deux réserves, écrites dans la fiche :

- personne dans le corpus ne rapporte une lecture console de `$50-$5F`
  depuis du code SA-1 ; le chiffre repose sur des notes matérielles et trois
  émulateurs, dont deux de même lignée ;
- Vitor Vilela donne la mémoire virtuelle en `$60-$7F`, les autres en
  `$60-$6F`. Non tranché.

Le défaut de `snes_verify` sur ce cas (banques sur deux chiffres, `sentences`
vide) reste ouvert.

## 2. Trois changements venus de luna

| Quoi | Effet pour vous |
|---|---|
| `authority_min` s'applique avant la coupe | il filtrait les `k` passages déjà choisis : un seul rendu sur cinq demandés. Il cherche maintenant jusqu'au rang 20 et rend les `k` premiers qui passent le seuil |
| La bannière « erreur documentée » ne se lève plus sur une adresse web | une note du manifest qui cite un lien partageait « http » et le nom du site avec tous les chunks de la même source. Sur nos 228 questions, 80 portaient une bannière ; 67 après correction |
| `pandocs-sgb` n'est servi que sur le Super Game Boy | hors de ce périmètre il parle du Game Boy ; il occupait 9 places sur nos 228 questions, aucune utile |

Le classement est mesuré neutre : 0 question gagnée, 0 perdue à 5 passages.
`snes_verify` rend les mêmes verdicts sur nos 60 paires.

## 3. Votre §2

Rien à faire de notre côté : les quatre lignes sont du code serveur, elles
suivront votre relance. luna les a vérifiées sur les fonctions du serveur.

## Dû

Par vous : la trace console du port vide, la photo du bit 3 en Mode 6.
Par nous : rien.
