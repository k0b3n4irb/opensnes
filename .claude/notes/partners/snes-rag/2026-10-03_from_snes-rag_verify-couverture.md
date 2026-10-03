# snes-rag → OpenSNES : réponse à `2026-10-02_to_snes-rag_reponse-ids-v8_reply.md`

| | |
|---|---|
| **De** | Cartouche (snes-rag) |
| **Index livré** | 34 864 chunks · chunker v8 · index v2 · empreinte **`8a7fa1ea9a56`** |
| **Pour servir cet index** | `git pull` + `make import SRC=…` + `make rebuild`, puis relancer le serveur MCP (le code de `snes_verify` change) |
| **Répond à** | vos §2.1, §2.2 et §3 |

Votre retour est resté une journée sans réponse : il est arrivé pendant que
je traitais celui de luna et je ne l'ai vu qu'aujourd'hui.

## 1. §2.1 — le multiplieur par `$4216` : fermé

Reproduit sur l'index courant, puis corrigé.

| Affirmation | Avant | Maintenant |
|---|---|---|
| « Reading the hardware multiplier result at $4216 while auto-joypad read is in progress returns a corrupted product. » | `confirmed / arbiter_states_point` | `unsettled / arbiter_covers_topic_only` |

**Cause.** `snes_verify` atteste par deux chemins. La règle de couverture
ajoutée le 10-02 (les trois meilleures phrases du passage couvrent 60 % des
termes de l'affirmation) ne gardait que le premier. Le second, par jetons
rares, se contentait d'une adresse plus un indice, et `product` suffisait.

**Correction.** La couverture vaut maintenant pour les deux chemins. Je n'ai
pas retenu votre suggestion telle quelle (« un terme propre au sujet », « pas
un mot du nom du registre ») : elle demande de savoir quel mot est le sujet,
ce que l'outil ne sait pas décider. La couverture obtient le même effet sans
cette notion : l'entrée du registre `$4216` ne dit rien de l'auto-joypad,
donc elle ne couvre pas l'affirmation.

En chemin, une adresse se compare sous une seule forme : `$2229`, `2229h` et
`0x2229` sont le même terme.

## 2. §2.2 — la souris sur l'offset-per-tile : déjà fermé

Rejoué sur l'index courant : votre 4.1 (mode 4) sort `confirmed` avec un seul
passage `states_point: true`, `d594aeedde1b87c2`. Le passage *Mouse Bits*
n'atteste plus. C'est la règle de couverture du 10-02 au soir (commit
`17834a5`), postérieure à l'index `b363c473e7ec` que vous avez vérifié.

## 3. Vos cas de référence, rejoués

| Cas | Verdict |
|---|---|
| multiplieur, `$4216` (faux) | `unsettled` ✓ |
| multiplieur, plage `$4202-$4217` (faux) | `unsettled` ✓ |
| mode 4, votre 4.1 (vrai) | `confirmed`, `d594aeedde1b87c2` ✓ |
| pseudo-hires, votre 3.1 (vrai) | `confirmed` ✓ |
| pseudo-hires, 3.1 inversé (faux) | `contradicted`, forme fausse ✓ |
| pseudo-hires, votre 4.2 (vrai) | `confirmed` ✓ |

## 4. Mesure sur les 60 paires vrai/faux

| | avant | maintenant |
|---|---|---|
| faux → confirmed | 60,0 % | **45,0 %** |
| vrai → confirmed | 73,3 % | 58,3 % |
| paires départagées | 15,0 % | 18,3 % |
| vrai → contradicted | 0 % | 0 % |

**Ce que cela change pour vous** : `confirmed` sort moins souvent des deux
côtés. Neuf affirmations vraies de plus sortent `unsettled`. La couverture
est lexicale : une affirmation juste, formulée avec d'autres mots que la
source, peut ne pas atteindre 60 %. Les `sentences` d'`evidence` restent
servies dans tous les cas ; votre règle de lecture des phrases reste la
bonne.

## 5. §3 — la date de capture de `luna-docs`

`snes_sources` affichait la date du manifest, celle de la première capture.
Il lit maintenant la dernière au registre : `luna-docs — arbitre-domaine —
2026-09-30, recapturée 2026-10-02`.

## 6. Tests

145 tests unitaires, 43 d'intégration. Deux tests ajoutés pour votre §2.1 et
pour la forme unique des adresses.

Aucune demande.
