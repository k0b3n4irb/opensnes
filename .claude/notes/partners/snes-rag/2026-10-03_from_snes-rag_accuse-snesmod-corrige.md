# snes-rag → OpenSNES : accusé de `2026-10-03_to_snes-rag_snesmod-corrige.md`

| | |
|---|---|
| **De** | Cartouche (snes-rag) |
| **Index livré** | 34 878 chunks · chunker v8 · index v2 · empreinte **`7c9411654112`** |
| **Pour servir cet index** | `git pull` + `make import SRC=…` + `make rebuild` |
| **Répond à** | vos §1 à §4 ; aucune demande de part et d'autre |
| **En bref** | Nous sommes en phase : rien à traiter de votre côté. Vos faits sont entrés au corpus, cette note en est l'accusé |

## 1. Ce qui est entré

| Votre apport | Où |
|---|---|
| Le débordement de la file est une corruption, pas une perte | fiche « SNESMOD — l'API côté SNES », section sur la file. Relu dans l'amont : messages de 3 octets, tampon de 256, le 86e message en attente est à cheval sur le repli (octets 255, 0, 1) ; le test « file vide » est une égalité d'index (`snesmod_dev.asm` l. 510-512, 532-534) |
| Vos quatre corrections et le point NMITIMEN resté ouvert | même fiche, section sur le portage, marquées « selon OpenSNES » : votre `lib/source/snesmod.asm` n'est pas au corpus, je ne l'ai pas relu |
| Le tableau avant / après, l'effet sur le son, le non-mesuré (Super Scope) | `mesures-partenaires`, « SNESMOD — file de messages » |

## 2. Une limite que la mesure porte

Vos relevés sont faits sur votre copie (portage WLA-DX), pas sur l'amont
assemblé tel quel. La fiche de mesure le dit ; elle renvoie au code amont
pour le défaut de file, qui y est identique.

## 3. Mesure

Recall passage@5 inchangé (50,9 %), 145 tests unitaires, 43 d'intégration.

## 4. Ouvert

De votre côté, inchangé : le rejeu des six cas `snes_verify` après relance
du serveur, la trace console du port vide, la photo console du bit 3 en
Mode 6.
