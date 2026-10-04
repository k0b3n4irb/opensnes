# luna → OpenSNES : `params` commité, et `rom.checksum_computed`

| | |
|---|---|
| **De** | luna (`develop`, `39359de`) |
| **Date** | 2026-10-05 |
| **Répond à** | `2026-10-05_opensnes-vers-luna_validation-avant-1.33.0_reply.md` |
| **Binaire** | `~/workspace/luna/target/release/luna` (reconstruit à `39359de`, s'annonce encore `luna 1.32.0`) |
| **En bref** | Sur votre accord, le changement de `params` est commité (`2b473c6`). Votre ligne ouverte du §5 est traitée par un champ nouveau, `rom.checksum_computed` (`39359de`). Aucune version n'est publiée. |

## 1. `dma.channels[n].params`

Commité tel que vous l'avez essayé : `2b473c6`. Merci pour la précision
sur `LUNA_BIN=… make tests` : notre recette du §3 essayait v1.32.0 sans
le dire, nous la corrigeons pour la prochaine fois.

## 2. `rom.checksum_computed`

Nous avons pris la seconde forme que vous proposez : un champ à côté.

| Champ | Sens |
|---|---|
| `checksum_valid` | inchangé : somme ⊕ complément = `$FFFF` |
| `checksum_computed` | nouveau : somme sur 16 bits de tous les octets de l'image chargée ; pour une taille qui n'est pas une puissance de deux, la fin est répétée jusqu'à remplir la moitié qu'elle occupe |

Le test « l'en-tête correspond aux octets » est
`checksum == checksum_computed`.

Pourquoi ne pas changer `checksum_valid` : la détection LoROM / HiROM s'en
sert comme signal, et un nom qui change de sens casserait en silence ceux
qui le lisent déjà.

Votre reproduction, sur le binaire ci-dessus :

| Fichier | `checksum` | `checksum_computed` | `checksum_valid` |
|---|---|---|---|
| `print_string.sfc` | `0xAF40` | `0xAF40` | true |
| copie avec l'octet `$0100` à `$FF` | `0xAF40` | `0xB02F` | true |

Nous trouvons `0xB02F` et non votre `0xB01F` : l'octet d'origine en
`$0100` vaut `$10`, donc la somme monte de `$EF`. À vérifier de votre
côté si votre chiffre venait d'un calcul et non d'une faute de frappe.

Le miroir est vérifié sur des ROMs commerciales de taille non puissance
de deux : 3 Mo (quatre titres, LoROM et HiROM) et 1,25 Mo (un titre),
somme calculée égale à l'en-tête à chaque fois.

Limites :

- La somme porte sur l'image **après retrait de l'en-tête de copieur**
  (512 octets), quand il y en a un.
- Deux images de 6 Mo de notre dossier ne concordent pas ; ce sont une
  traduction et un dump modifié, nous n'avons pas d'image de 6 Mo saine
  pour juger le cas ExHiROM.

Usage :

```
luna state game.sfc --until-frame 0 --out - | jq '.rom | .checksum == .checksum_computed'
```

`luna run` affiche aussi la valeur dans son résumé d'en-tête
(`… / computed $AF40`).

## 3. Ouvert

Rien de notre côté. Les deux changements partiront avec la prochaine
version, qui n'est pas décidée.
