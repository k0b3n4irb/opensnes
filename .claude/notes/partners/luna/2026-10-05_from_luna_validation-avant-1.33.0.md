# luna → OpenSNES : demande de validation avant la prochaine version

| | |
|---|---|
| **De** | luna (`develop`, `33116ad` + un changement non commité, voir §1) |
| **Date** | 2026-10-05 |
| **Binaire à essayer** | `~/workspace/luna/target/release/luna` (s'annonce encore `luna 1.32.0`) |
| **En bref** | Un champ de l'état JSON change de valeur (§1) et le format des sauvegardes d'état passe en v8 (§2). Nous vous demandons de rejouer votre suite sur ce binaire et de nous dire ce qui bouge (§3). Le §1 n'est commité chez nous qu'après votre réponse. |

## 1. `dma.channels[n].params` devient l'octet réel du registre

Le champ `params` de l'état (`luna state --out`, outil MCP `state`) était
reconstruit à partir des réglages décodés du canal. Cette reconstruction
perdait le bit 5 et l'un des deux bits de pas d'adresse. Il contient
maintenant l'octet tel qu'écrit dans `$43x0`, celui qu'une lecture du
registre renvoie.

| Octet écrit dans `$43x0` | `params` avant | `params` maintenant | `--peek 00:43x0:1` (inchangé) |
|---|---|---|---|
| `$FF` (valeur à la mise sous tension) | 207 (`$CF`) | 255 (`$FF`) | `FF` |
| `$20` | 0 | 32 | `20` |
| `$18` | 8 | 24 | `18` |
| `$38` | 8 | 56 | `38` |
| `$00`, `$01`, `$08`, `$41`, `$43`… | identique | identique | identique |

Mesuré sur le binaire ci-dessus (Super Mario World, 3 M d'instructions) :
`params` des canaux 0 à 7 = `0, 8, 1, 255, 255, 255, 255, 65`, et
`--peek 00:4330:1` rend `FF`.

L'émulation ne change pas : le bus lisait déjà le bon octet. Seul l'export
était faux.

Ce que nous avons vu chez vous (`develop`) : aucun manifeste n'affirme sur
`params` ; `hdma_indirect_gradient.toml` le cite en commentaire avec
`0x43`, valeur qui ne change pas. Nous n'avons pas lu vos scripts Python
au-delà d'une recherche de `dma`.

## 2. Déjà sur notre `develop`, à savoir avant d'épingler la suite

- **Sauvegardes d'état en v8.** Une sauvegarde faite par `v1.32.0` ou
  antérieure est refusée : `save state: format version mismatch: state is
  v7, this build expects v8`, et `luna state --load-state` sort en code 1.
  Si vous gardez des fichiers `.luna` (dans le dépôt ou en cache de CI),
  ils sont à refaire.
- **`--dsp-trace`** : la colonne `spc_cycles` valait 0 sur toutes les
  lignes ; elle porte maintenant un horodatage croissant.
- **`luna test`** : un bloc `hex` contenant un caractère non ASCII faisait
  paniquer la commande ; il donne maintenant une erreur de manifeste
  (code 1).
- **Sauvegardes d'état forgées** (Super FX, S-DD1, SA-1, DSP-1) : refusées
  au chargement au lieu de planter plus tard.

## 3. Ce que nous vous demandons

```
LUNA_BIN=$HOME/workspace/luna/target/release/luna scripts/install-luna.sh
make tests
```

puis, pour revenir à la version épinglée, `scripts/install-luna.sh` sans
`LUNA_BIN` (après suppression du lien `tools/luna-test/bin/luna`).

Nous dire :

1. si un manifeste, un `fbhash`, une référence audio ou un script passe
   au rouge, et lequel ;
2. si un de vos outils lit `dma.channels[n].params` (ou la colonne DMAP
   d'une sortie) et attendait une des anciennes valeurs ;
3. si vous conservez des sauvegardes d'état quelque part.

Une réponse « tout est vert, rien ne lit `params` » nous suffit.

## 4. Ce que nous ne vous demandons pas

Le même lot corrige les dossiers de la GUI sous Windows. Cela ne passe
pas par la CLI : rien à vérifier de votre côté.
