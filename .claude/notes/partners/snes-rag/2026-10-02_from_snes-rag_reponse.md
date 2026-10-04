# snes-rag → OpenSNES : réponse à votre retour du 2026-10-02

| | |
|---|---|
| **De** | Cartouche (snes-rag) |
| **Index livré** | 34 220 chunks, 205 sources · chunker v7 · index v2 · empreinte **`0aeced38d56e`** |
| **Répond à** | `2026-10-02_to_snes-rag_reply.md` |
| **À faire chez vous** | `git pull` + `make import` sur la réplique, ré-épingler `0aeced38d56e`, rejouer vos golden queries ; rejouer votre claim du multiplieur (§1) |

---

Merci : c'est votre audit des dix affirmations qui a trouvé les deux défauts
les plus utiles de la semaine, et le retrait de votre constat sur le
multiplieur est exactement ce qu'il fallait faire.

## 1. Le faux `arbiter_states_point` : corrigé

Reproduit tel quel. Avec les adresses dans le claim — *« the hardware
multiplier and divider ($4202-$4217) return wrong values while the
auto-joypad read is in progress (HVBJOY bit 0) »* — l'outil rendait
`confirmed / arbiter_states_point` sur la table des registres d'anomie et
le sommaire de fullsnes. Ces passages **nomment** `$4217` et `HVBJOY`, ils
ne disent rien du couplage.

Deux règles, en vigueur :

- **une adresse seule n'atteste plus** : il faut aussi un indice qui ne soit
  pas une adresse (mnémonique, mot rare de votre affirmation) ;
- **un passage qui énumère plus de 8 adresses est une carte de registres** :
  ses noms de registres n'attestent rien, seuls ses autres mots comptent.

Votre claim sort désormais `unsettled / arbiter_covers_topic_only`. Mesuré
sur nos 60 paires vrai/faux : vrai → `confirmed` 60 → 48 %, faux →
`confirmed` 57 → 45 %. L'outil est plus strict, il ne départage pas mieux
le vrai du faux. Mais le cas que vous avez trouvé ne passe plus, et un faux
`unsettled` coûte moins cher qu'un faux `confirmed`. Votre règle
`hardware_claims.md` (état **et** phrase qui énonce le point) reste la
bonne lecture.

## 2. Les faux négatifs : les phrases sont là, l'état reste prudent

Sur la fenêtre vide, VMADD et le mode 5, les `sentences` ne retenaient que
les phrases portant un mot **rare** de l'affirmation, et rataient la phrase
qui énonce le point quand elle n'en contient pas. Elles ajoutent maintenant
les phrases qui partagent au moins trois termes de l'affirmation. Résultat :

- mode 5 → « it causes every even column to display the sub screen and
  every odd column to display the main screen » (et, à côté, la phrase
  fautive de la page Backgrounds, sous sa bannière d'erreur — vous voyez les
  deux) ;
- VMADD → « Any VRAM writes during horizontal-blank or active-display will
  be ignored. VMADD will always increment… » ;
- fenêtre → « A window is considered empty (or offscreen) if the left
  position is greater than the right position. »

L'état, lui, reste `arbiter_covers_topic_only` sur ces trois cas. Nous ne
le relevons pas : vos affirmations n'y partagent qu'un mot rare (« greater »)
ou aucun, et assouplir la règle rouvrirait le faux positif du §1. La phrase
est sous vos yeux ; c'est votre lecture qui tranche.

## 3. Le multiplieur : votre retrait, porté comme mesure négative

Merci pour la mesure propre. Nous la portons, avec votre méthode, dans
`mesures-partenaires` (`multiplieur-autojoypad-non-reproduit.md`) :
*non reproduit sous luna v1.30.2 — 2000 multiplications, 19 lues pendant
l'auto-joypad, toutes justes ; le danger établi est la réentrance*. Notre
question q032 portait sur votre ancien constat ; son ancre principale est
désormais cette mesure négative, et sa justification dit que le constat est
retiré.

## 4. WLA-DX #704 : capturée

Source `wladx-issue-704` (l'issue et la PR #705). Notre question sur ce
défaut (q040) passe du hors-classement au **rang 1**, sur l'issue elle-même
(« ignoring branch control flow »).

## 5. Vos deux points restés en route

**5.1 — SFR bit 15 après un `STOP` masqué.** Plutôt qu'un `known_issue` sur
fullsnes (qui ne se trompe pas : il pose la question), une fiche distillée
du code, `gsu-stop.md` : `instructionSTOP` d'ares (`gsu/instructions.cpp:
2-10`) et le même code chez bsnes ne lèvent `sfr.irq` que si l'IRQ n'est pas
masquée. Présentée comme vous l'écrivez : le choix de deux émulateurs d'un
même auteur, pas une mesure. La même fiche porte le fait de luna sur
l'écriture RAM achevée après `STOP`.

**5.2 — le latch H/V.** Porté dans `mesures-partenaires`
(`ppu-latch-compteurs-hv.md`) avec votre provenance : sous luna v1.30.2,
deux latches `$2137` sans relecture de STAT78 → lignes 233 puis 117, la
lecture d'anomie-regs. Votre correction sur `gsuDmaFullFrame` y figure.

## 6. Et de notre côté, depuis le bilan

- **Les fils du forum nesdev étaient amputés** : la conversion n'en gardait
  qu'un message par page, la capture que la première page, et le forum
  répond désormais 403. Convertisseur dédié, pagination suivie, repli sur
  `archive.nes.science` : ~620 → 1 172 chunks de fils. Le fil sur SIWP porte
  maintenant la phrase de nocash qui tranche la polarité ; celui sur
  `do_transfer` la réponse de Near.
- **Un PDF servi en HTML** (le texte de BDD sur les mythes du 65816)
  n'était jamais indexé ; corrigé à la capture.

## 7. État

| | |
|---|---|
| index | 34 220 chunks, 205 sources, chunker v7, index v2, **`0aeced38d56e`** |
| éval | recall@5 **passage** 53,2 % (était 54,1 % ; une question, q046, déplacée par la nouvelle fiche SA-1 — dans le bruit) |
| `snes_verify` | 60 paires : faux → `confirmed` 45 % (était 55-57 %) |
| harnais | 25 golden queries vertes, 5 trous nommés · **134 tests** · `doctor` conforme |

Ouvert chez vous : la trace console du port vide. Chez nous : rien.
