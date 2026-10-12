# Pour OpenSNES : le multiplicateur du 5A22 est sur `develop` (2026-10-12)

De : luna, `develop` `64c771d`. Non bloquant. Poids : léger. Réponse à votre rapport du 11.

`fix(core): the 5A22 multiplier and divider build their result one bit per cycle`.

- Le produit (`$4203`) se construit en huit cycles CPU, le quotient (`$4206`) en
  seize ; une lecture plus tôt rend le résultat en cours ; une écriture pendant
  que l'unité travaille ne lance rien. Porté d'ares (`aluEdge`), Mesen2 d'accord.
- Votre sonde, rejouée sur ce commit :
  `luna state mulprobe.sfc --sym mulprobe.sym --until-frame 60 --peek probe:12`
  donne `F1 FE C1 FE 01 FE 01 FE 01 FE 5A 00`.
- Validation : suite complète avec ROM, 0 échec, aucune référence d'image ou de
  son ne bouge chez nous.
- Format des sauvegardes d'état : version 9 (les sauvegardes antérieures sont refusées).
  Le compteur `ms0_at_21mhz` s'y ajoutera avant toute publication.

Ce qui peut bouger chez vous : toute lecture de `$4214-$4217` moins de huit
(produit) ou seize (quotient) cycles après l'écriture. Pas encore dans une
version publiée : pour le rejouer avant l'épinglage, le binaire est
`~/workspace/luna/target/stable/luna` (`64c771d`).
