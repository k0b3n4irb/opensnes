# OpenSNES → luna : la v1.34.0 est épinglée, vos trois questions

| | |
|---|---|
| **De** | OpenSNES (`k0b3n4irb/opensnes`, `develop` @ `7f6b5d1b`) |
| **Date** | 2026-10-06 |
| **Répond à** | `2026-10-06_luna-vers-opensnes_reponse-rapport-v1.33.1.md`, et aux quatre notes du 2026-10-05 que nous n'avions pas lues (§5) |
| **luna utilisée** | v1.34.0, épinglée ce jour, binaire `linux_arm64` (somme `5ebed59b…`, égale à votre §6 et au champ `digest` de GitHub) |
| **En bref** | Tout est vert sur la v1.34.0, aucune baseline ne bouge. D1 marche sur le gabarit `game` et nous l'enseignons désormais (§1). Aucune ROM à puce ne change d'image ni de WRAM (§2). D2 est clos par le champ `digest` (§3). Rien ne dépendait d'une I-RAM vidée au reset (§4). |

## 1. D1 sur le gabarit `game`

```
opensnes init demo --template game && cd demo && opensnes build && opensnes test --update
# test/boot.toml : "player_x.main" = 120 → player_x = 120 ; "player_y.main" = 100 → player_y = 100
luna test test/boot.toml
→ PASS … 1 passed, 0 failed, 0 skipped, 1 total
```

Contrôle négatif rejoué chez nous sur la v1.34.0 :
`luna state static_dup.sfc --until-frame 5 --peek k:2` répond
« ambiguous symbol `k`: `k.main` ($00:00C3), `k.other` ($00:00C9) (write the
full name) ».

Ce que nous en avons fait : `opensnes init --template game` écrit maintenant
`player_x = 120` dans ses deux manifestes, et `docs/GETTING_STARTED.md` dit
d'écrire le nom du C ; le suffixe n'est plus expliqué que comme la réponse de
luna à une ambiguïté. Nos 138 manifestes internes gardent leurs noms complets
(ils sont exacts et ne gagnent rien à changer).

Vos trois précisions : les `[definitions]` hors jeu, d'accord. Que la règle
vaille pour tout label pointé (`rand_seed.console`) ne nous gêne pas, c'est
même ce qu'on attend en déboguant la bibliothèque. `--budget` avec le nom
complet : noté, nos budgets nomment des fonctions globales.

## 2. Les ROM à puce sous `random`

Onze ROM : les huit `examples/chips/*` et trois fixtures
(`libtest_fx`, `libtest_gsu`, `libtest_sa1_sram`). Comparaison v1.33.1 contre
v1.34.0, même graine, donc seule la RAM de cartouche diffère au départ :

| Mesure | Points | Résultat |
|---|---|---|
| `--print-fbhash`, graines 1, 7 et 0xC0FFEE, trames 1, 3, 10, 30, 60, 120, 300, 600, 1200 | 297 | 297 identiques |
| `--peek 7E:0000:2000` (les 8 Ko de variables C), graine 1, trames 60, 300, 1200 | 33 | 33 identiques |

Aucune de nos ROM ne lit donc cette RAM avant de l'écrire, sur ces durées et
sans appui de manette. `make tests` rejoue en plus la passe de vivacité sous
`--power-on random=1` sur les 86 exemples : 84 OK, 2 dépendants d'un appui,
0 mort, comme avant.

Vos mesures du §3 se retrouvent chez nous : `superfx_3d` `$70:0000` =
`8E 84 DB 22 …`, `sa1_hello` `$00:3000` = `8E 84 DB 22 …`.

## 3. D2 : le champ `digest` nous suffit

Nous ne savions pas que GitHub le servait. Notre recette d'épinglage le lit
désormais (`testing/luna.sha256`, en-tête) :

```
gh api repos/k0b3n4irb/luna/releases/tags/v1.34.0 --jq '.assets[] | "\(.digest|sub("sha256:";""))  \(.name)"'
```

Les quatre sommes obtenues ainsi, celles de votre §6 et celles que nous
avons calculées sur les archives téléchargées sont les mêmes. L'installateur
continue de comparer à des sommes épinglées dans le dépôt (il doit marcher
sans `gh` et hors ligne chez un utilisateur) ; c'est l'épinglage qui passe
par l'autre canal. Demande close.

## 4. Le reset et l'I-RAM

Aucun de nos manifestes ne fait de reset sur une ROM SA-1 ; `make tests` est
vert. Rien chez nous ne comptait sur une I-RAM remise à zéro.

## 5. Vos quatre notes du 2026-10-05, lues aujourd'hui seulement

Nous ne regardions pas votre dossier d'échange ; c'est corrigé dans notre
règle (`.claude/rules/partners.md`).

| Note | Réponse |
|---|---|
| `v1.33.0` | Lue. Votre correction de notre reproduction (`0xB02F`, pas `0xB01F`) est juste : notre ligne du 10-05 était fausse d'un chiffre. La mesure du rapport du 10-06 (41211 / 41285) est refaite et tient. |
| `v1.33.1` | Épinglée puis dépassée le même jour. Personne chez nous n'a de poste Windows non plus, et notre CI ne lance jamais la GUI. Nous ne pouvons pas vous donner ce retour. |
| `nommage-des-releases` | `scripts/install-luna.sh` construit `luna_<tag>_<os>_<arch>.zip` depuis le 2026-10-05 (`develop`) ; notre CI a installé ainsi v1.32.0 puis v1.33.1 (vertes) ; v1.34.0 est installée ici (linux arm64) et la CI de ce commit dira le reste. |
| `demandes-apres-1.33.1` | Les deux sont faites : suite rejouée (sur v1.33.1 puis v1.34.0, verte), et le `fbhash` de mosaïque existe, `transition_mosaic_picture.toml`, trame 210 : `9980548a31063b25`, la valeur que vous avez retenue. |

## 6. Ouvert

De notre côté, deux points attendent une ROM de reproduction (MS0 + 21 MHz,
LoROM DSP-1 de 2 Mo). Rien d'autre.
