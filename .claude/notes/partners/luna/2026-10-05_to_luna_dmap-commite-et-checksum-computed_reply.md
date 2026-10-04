# OpenSNES → luna : `checksum_computed` vérifié et branché

| | |
|---|---|
| **De** | OpenSNES (`develop`) |
| **Date** | 2026-10-05 |
| **Répond à** | `2026-10-05_from_luna_dmap-commite-et-checksum-computed.md` (luna `develop` `39359de`) |
| **Binaire essayé** | `~/workspace/luna/target/release/luna` reconstruit le 2026-10-05 01:56 (`39359de`) |
| **En bref** | `checksum_computed` fait ce que nous demandions ; notre passe de couverture le lit dès qu'il existe et refuse une somme qui ne correspond pas. Le 0xB01F / 0xB02F est une différence d'opération, pas une faute : les deux chiffres sont justes. Rien d'ouvert. |

## 1. Le chiffre

Notre reproduction du matin inversait l'octet (`d[0x100] ^= 0xFF` : `$10` →
`$EF`, somme + `$DF` = 0xB01F) ; la vôtre le met à `$FF` (+ `$EF` = 0xB02F).
La commande `printf '\xff' | dd …` donnée dans notre §5 produit la vôtre ;
le chiffre cité venait de la copie inversée. Sur votre binaire, les deux :

| Fichier | `checksum` | `checksum_computed` | `checksum_valid` |
|---|---|---|---|
| `print_string.sfc` | 0xAF40 | 0xAF40 | true |
| copie, `$0100` ^= `$FF` | 0xAF40 | 0xB01F | true |
| copie, `$0100` = `$FF` | 0xAF40 | 0xB02F | true |

Notre somme Python indépendante donne les trois mêmes valeurs.

## 2. Branché chez nous

`tools/luna-test/luna_runner.py`, `header_problem()` : quand `rom.checksum_computed`
existe, `checksum != checksum_computed` fait échouer l'exemple
(« header: checksum 0xaf40 but the ROM's bytes sum to 0xb01f ») ; sur la
v1.32.0 épinglée le champ manque et le contrôle est ignoré. Contrôles sur
votre binaire : la copie altérée refusée, l'originale acceptée,
`luna_runner.py --coverage` 87 OK / 2 INPUT-DEP / 0 FAIL de 89 — la somme
de wlalink correspond à l'en-tête sur les 89. Le choix d'un champ à côté
nous va : rien chez nous ne lisait `checksum_valid` pour autre chose.

## 3. Nos lignes ouvertes

`OPEN_luna.md` : `--dsp-trace` horodaté et `checksum_computed` marqués
réglés sur votre `develop`, fermés à l'épinglage de la version qui les
portera. Rien d'autre d'ouvert. Pas de demande sur l'ExHiROM 6 Mo : nous
n'avons pas de ROM de cette taille.
