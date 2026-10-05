# Les 91 exemples : inventaire, doublons, proposition (2026-10-05)

Demandé par le propriétaire avant la migration des exemples sur les fichiers
de réglages (`docs/tools/CONVENTIONS.md`) : « c'est l'occasion de revoir
tous ces exemples ; 30 % sont des répétitions ». Lu sur `develop` à
`2fbceeef`. Rien n'est supprimé ici : c'est la proposition, à trancher par
le propriétaire, et la méthode pour l'appliquer sans casser ce que les
exemples tiennent.

## 1. Ce que l'inventaire mesure

Pour chacun des 91 : la catégorie, le titre et les « SNES Concepts » du
README, les modules liés, les lignes de `main.c`, le nombre de manifestes
luna qui le jouent, le nombre de pages de `docs/` qui le citent, et quatre
contraintes dures :

| Contrainte | Source | Ce qu'elle impose |
|---|---|---|
| **Grille console** | `docs/HARDWARE_VERIFICATION.md` (26 ROM) | un exemple de la grille ne se supprime pas ; il peut absorber un autre |
| **Seul exécutant** d'une fonction publique | `testing/ROM_COVERAGE.md`, table « one ROM only » | supprimer l'exemple découvre la fonction : son code passe dans l'exemple qui l'absorbe ou dans une fixture `libtests*` |
| **Manifestes et baselines** | `testing/manifests/`, `baselines/` | un manifeste suit son ROM (renommé ou fusionné) ou disparaît avec lui ; les baselines WRAM / fbhash / audio se recapturent |
| **Références des docs** | tutoriels (`@ref examples_*`), `examples/README.md` (échelle numérotée et table par catégorie), `KNOWN_LIMITATIONS.md` | le sentinel (ancres 3, 5, 6) refuse un chemin mort et un compte faux ; chaque référence se réécrit vers l'exemple absorbant |

Treize README n'ont pas de leçon (ils commencent par `cd $OPENSNES_HOME &&
make`) : `snesmod_music_large`, `snesmod_sfx`, `collision_demo`,
`sa1_hello`, `sa1_starfield`, `superfx_3d`, `superfx_hello`,
`transparency`, `text_glyphs`, `breakout`, `likemario`,
`continuous_scroll`, `dynamic_metasprite`. Qu'ils restent ou non, ceux qui
restent reçoivent un README qui dit ce qu'on apprend.

## 2. Les doublons, catégorie par catégorie

Le critère : **un exemple survit s'il enseigne quelque chose qu'aucun autre
n'enseigne, ou s'il est le seul à exécuter une fonction de la lib, ou s'il
est dans la grille console**. Deux exemples qui enseignent la même chose à
deux tailles fusionnent dans le plus complet, avec un bouton pour passer
d'une variante à l'autre quand les deux valent d'être vues.

### Premier niveau : les fusions sûres (19 exemples, 91 → 72)

| Catégorie | Disparaît | Absorbé par | Pourquoi | À déplacer |
|---|---|---|---|---|
| audio | `snesmod_music_large` | `snesmod_music` | même leçon avec un module plus gros ; le franchissement de banque devient un second module du même exemple | ses 2 manifestes |
| audio | `speech_synth`, `play_noise`, `pitch_mod`, `echo` | un seul `audio/dsp_effects` (quatre tours du S-DSP sous quatre boutons) | quatre portages krom d'une astuce DSP chacun, aucun cité par les docs ; un studio veut les voir côte à côte | `audioDisableEcho` (seul exécutant : `echo`), les 9 manifestes, 4 baselines audio |
| backgrounds | `mode1_bg3_priority` | `mode1` | un drapeau (`BG3_MODE1_PRIORITY_HIGH`) : un bouton dans `mode1` | — |
| backgrounds | `mode5` | `mode5_hires` | deux démos du Mode 5 ; `mode5_hires` est dans la grille et va plus loin (entrelacé 448) | — |
| backgrounds | `mode4` | `mode2` | la trilogie offset-per-tile (2/4/6) en deux : `mode2` montre 2 et 4 par bouton, `mode6` (grille) reste | — |
| basics | `timer` | `text/print_string` enseigne déjà le rythme VBlank ; `game_skeleton` le compteur | la plus petite leçon du corpus, déjà donnée deux fois | — |
| basics | `random` | `game_skeleton` | le RNG s'apprend dans le squelette de jeu, où il sert | — |
| basics | `fix32_orbit` | `aim_target` | deux démos de maths ; la cible qui orbite devient le mobile d'`aim_target` | — |
| chips | `sa1_hello` | `sa1_starfield` | « ça démarre » est la première minute de la vitrine (grille) | son manifeste |
| color | `hicolor_blend` | `hicolor_1792` | deux portages krom « plus de couleurs » ; `hicolor_1792` est dans la grille | — |
| color | `gradient_9bit` | `hdma/gradient_colors` | deux dégradés de fond par HDMA | — |
| games | `mapandobjects` | `games/likemario` et `maps/slope_collision` couvrent carte + moteur d'objets | 147 lignes, pas un jeu | ses tables `objfct*` servent de classe de dérive à l'oracle WRAM : vérifier qu'un autre ROM les porte |
| input | `move_sprite` | le `starter/` | c'est exactement le programme du starter | — |
| maps | `map_scroll` | `maps/tiled` | deux cartes Tiled qui défilent ; Mario rejoint le niveau de `tiled` | ses 2 manifestes |
| mode7 | `perspective` | `rotate_scale` (un bouton passe de la rotation plate au sol en perspective) | `perspective_rotate` (grille) garde la matrice complète | — |
| scrolling | `mixed_scroll` | `parallax_scroll` | deux couches à des vitesses différentes : la parallaxe le fait avec trois | — |
| sprites | `dynamic_metasprite` | `dynamic_sprite` (variante métasprite sous un bouton) | la combinaison de deux leçons déjà données ; README nu | ses 2 manifestes |

### Second niveau : à trancher (6 de plus, 72 → 66, soit 27 % en tout)

| Disparaît | Absorbé par | Ce qui plaide pour, ce qui retient |
|---|---|---|
| `sprites/sprite_sizes` | `simple_sprite` (un bouton fait défiler les six tailles OBJSEL) | pour : un menu de 252 lignes pour six valeurs ; contre : la référence visuelle des six combinaisons est pratique telle quelle |
| `transitions/mosaic` | `fading` → un seul `transitions` | pour : les deux effets se combinent déjà dans `mosaic` ; contre : `mosaic` est cité par six pages |
| `chips/superfx_hello` | `superfx_3d` | pour : « ça démarre » ; contre : ses deux manifestes épinglent le chemin de job GSU minimal (`libtests_gsu` en couvre une partie) |
| `hdma/hdma_indirect_gradient` | `gradient_colors` (le mode indirect sous un bouton) | pour : troisième dégradé ; contre : seul exécutant de `hdmaSetupIndirect`, portage krom documenté |
| `text/scroll_message` | `print_string` | pour : « déplacer le texte » tient en une ligne de `bgSetScroll` ; contre : c'est la marche 2 de l'échelle, lue par les débutants |
| `maps/dynamic_map` | — | pour : un moteur maison de « sprites en tilemap » (474 lignes) qui n'est pas celui de la lib ; contre : six pages le citent, et il montre un vrai usage de BG1 comme grille |

Ce que je garde sans hésiter, parce que rien d'autre ne l'enseigne : les
sept jeux restants, la grille console entière, les seuls exécutants
(`shadow_tint`, `direct_color`, `dsp1_cube`, `dsp1_ground`, `superfx_3d`,
`hdma_helpers`, `parallax_scroll`, `apu_switch`), `soundboard` et
`sfx_from_wav` (le moteur audio v2 complet et sa première marche),
`collision_demo` (seule démo du module, README à écrire), `text_glyphs`
(le niveau « sous le capot »), `pseudo_hires` (la question du Mode 6 pour
luna et snes-rag, rangée 23), `aseprite_pipeline` (la vitrine de la
famille d'outils).

## 3. Ce que ça donne

| | Aujourd'hui | Premier niveau | Les deux niveaux |
|---|---|---|---|
| Exemples | 91 | 72 | 66 |
| README sans leçon | 13 | 6 (à écrire) | 6 |
| Portages krom isolés | 10 | 5 | 4 |
| Fonctions découvertes | 0 | 0 (déplacées) | 0 (déplacées) |

Les catégories restent les mêmes ; `games/` passe à 7, `audio/` à 6,
`basics/` à 5.

## 4. La méthode, prudente

Un commit par catégorie, dans cet ordre, et chaque commit vérifié par la
même grille :

1. **Fusionner d'abord, migrer ensuite.** La fusion (deux `main.c` en un,
   un bouton) se fait sur le `data.asm` existant ; la migration vers les
   fichiers de réglages vient quand la catégorie est stable. Deux
   changements dans un commit, c'est une régression qu'on ne sait plus
   attribuer.
2. **Avant de supprimer** : la table du §1 pour l'exemple — grille (non),
   seul exécutant (déplacé où ?), manifestes (renommés ou supprimés),
   références (réécrites) ; `rom_coverage.py` doit rester à 325/325 sans
   ligne nouvelle dans `never_executed.txt`.
3. **Pour chaque catégorie** : `make clean && make` du dossier, `luna_runner
   --compare --only <cat>` et `wram_regress --only <cat>` sur les survivants,
   `make test-manifests`, `make lint-docs` (ancres 3, 5, 6, 9 : chemins,
   comptes, modules des README), `make docs-strict`. Les baselines d'un
   exemple fusionné sont recapturées **une fois**, avec la raison dans le
   commit, comme `testing.md` le demande.
4. **La migration elle-même**, par catégorie : un `<asset>.toml` par source,
   le `data.asm` supprimé, les `extern` du `main.c` alignés sur les noms que
   l'outil génère (`<nom>_til`, `<nom>_tilend`, `<nom>_pal`). Un ROM migré
   doit donner **la même image et le même flux WRAM** qu'avant (la section
   change de nom, pas le contenu) ; une différence de fbhash est à expliquer
   avant toute recapture.
5. **Les treize README nus** sont écrits au passage, avec les trois
   sections que `new_example.md` impose (ce qu'on apprend, les concepts,
   les modules).

Ordre proposé : `text`, `input`, `basics`, `sprites`, `backgrounds`,
`scrolling`, `hdma`, `color`, `windows`, `transitions`, `mode7`, `maps`,
`memory`, `audio`, `chips`, `games` — du plus simple au plus lourd, les
jeux en dernier parce que leurs assets sont les plus nombreux.

## 5. Ce qui n'est pas dans cette proposition

- Les **assets hérités de PVSnesLib** (décision reportée le 2026-09-26) :
  la migration ne change pas leur provenance ; `ATTRIBUTION.md` reste à
  jour fichier par fichier.
- Le **compte « 91 exemples »** apparaît dans `CHANGELOG.md` (gelé) et dans
  les docs actives : le sentinel (ancre 3) fait suivre les secondes.
- `examples/README.md` a deux structures (la table par catégorie et
  l'échelle numérotée avec ses marches `15c…15n`, `22b`, `42c…42g`) :
  l'échelle est à renuméroter d'un bloc une fois les fusions faites, pas
  marche par marche.
