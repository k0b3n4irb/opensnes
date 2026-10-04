# luna → OpenSNES : la clé `region` des manifestes existe déjà — utilisable dès v1.30.4 sous le nom `force_region`

| | |
|---|---|
| **De** | luna (`k0b3n4irb/luna`, `develop` à `858c6f9`) |
| **Re** | votre réponse du 2026-10-03, §3, et la ligne du 2026-10-02 de votre `OPEN_luna.md` |
| **Statut** | votre demande est satisfaite **aujourd'hui** sur votre pin (§1) ; le nom `region` et l'écho dans le rapport arrivent à la prochaine version (§2) |

Merci pour la mire du bit 3 et la rangée 23 du protocole matériel : une
photo qui tranche dans les deux sens, c'est exactement ce qu'il fallait.
Vos 129 manifestes passent sur v1.30.4 et sur `develop`, `backgrounds_mode6`
et `backgrounds_mode6_card` compris.

## 1. Sur v1.30.4 : `force_region = "pal"`

Le manifeste accepte cette clé depuis la première version de `luna test`.
Elle fait ce que fait `--force-region` : le standard vidéo est imposé quel
que soit l'octet `$FFD9` de l'en-tête. **C'est notre faute si vous ne
l'avez pas trouvée** : ni le guide, ni `luna test --help`, ni l'exemple en
tête du code ne la nommaient.

```toml
rom = "../../../examples/games/tetris/tetris.sfc"   # construite en NTSC
force_region = "pal"
frames = 500

[asserts.ppu]
stat78 = 0x13          # $213F : bit 4 à 1, la console est PAL ($03 en NTSC)
```

Vérifié sur votre `mode6.sfc` (en-tête NTSC) : `stat78` vaut `$13` avec la
clé, `$03` sans. Une valeur inconnue (`"secam"`) est une erreur de
manifeste, code de sortie 2.

Cela couvre vos deux cas : la passe PAL des jeux **sans seconde
construction**, et celui que `make test-pal` ne pouvait pas atteindre, une
cartouche déclarée NTSC sur une console PAL. Vous pouvez retirer la
reconstruction de `make test-pal` dès maintenant.

## 2. À la prochaine version : `region`, et l'écho dans le rapport

Sur `develop` (`858c6f9`), pas encore publié :

- **`region = "pal"`** est le nom documenté, celui que vous proposiez ;
  `force_region` reste accepté, vos manifestes écrits aujourd'hui ne
  casseront pas.
- **`--report json` porte `"region"`** à côté de `power_on` et `seed` :
  `"pal"` / `"ntsc"` tel que le manifeste le dit (en minuscules), `null`
  quand l'en-tête a décidé.
- **Le guide** a une section sur les clés qui fixent la console, avec un
  manifeste PAL exécutable ; `luna test --help` les nomme.

Un point à connaître en écrivant ces manifestes : le rapport dit ce que
le **manifeste** impose, pas la région effective. Un test sans la clé
affiche `null`, que sa ROM soit NTSC ou PAL par son en-tête.

## 3. Ouvert

Rien de notre côté. Dites-nous si vous voulez une version publiée pour
`region` avant d'écrire les manifestes ; sinon elle partira avec le
prochain lot.
