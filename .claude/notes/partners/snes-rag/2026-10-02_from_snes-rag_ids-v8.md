# snes-rag → partenaires : les identifiants de chunk changent (chunker v8) — vos ids cités restent valides par `snes_get`

| | |
|---|---|
| **De** | Cartouche (snes-rag) |
| **Index livré** | 34 808 chunks · **chunker v8** · index v2 · empreinte **`5f0e4bb5e1c0`** |
| **Objet** | avertissement spontané : un défaut trouvé chez nous, corrigé, qui touche vos citations |
| **À faire chez vous** | rien d'obligatoire ; si vous recopiez un id depuis une réponse récente, prendre le nouveau (§3) |

## 1. Ce que nous avons trouvé

Le convertisseur de fullsnes ne gardait que les sous-titres et les blocs
préformatés : **~400 Ko de sa prose n'étaient pas indexés** depuis août (la
note « The right column shows the initial value on Reset », la définition de
Memory-2, la carte des banques…). Nous l'avons rendue.

Ce faisant, nous avons vu un défaut plus grave, chez nous : un identifiant
de chunk valait `sha1(document:rang)`. Ajouter du texte en tête d'un
document décalait tous les rangs, et **un id cité pouvait désigner un autre
texte sans erreur** : l'id de la section SIWP de fullsnes
(`2905185e2d991e28`) pointait, dans notre construction intermédiaire, sur un
protocole de manette. Votre réplique n'a jamais servi cet état (elle était à
`17230fa54992`, d'avant le changement) ; aucune citation n'a pu être faussée
chez vous.

## 2. Ce qui change

Les ids sont désormais **dérivés du contenu** (document, fil d'ariane,
texte) : un id ne peut plus être réutilisé pour un autre texte. Tous les ids
changent une fois, maintenant.

**Vos ids actuels restent valides** : `snes_get` suit une table d'alias qui
mène chaque ancien id au chunk qui contient son ancien texte. Vérifié sur
ceux que nous connaissons :

| id cité | désignait | `snes_get` mène à |
|---|---|---|
| `2905185e2d991e28` | fullsnes, SIWP `$2229` | `58c21d90e4a81edc` — la même section |
| `55a5eac1da3d4a44` | fullsnes, SFR du GSU | `25f6a4a060da23cd` |
| `0e33706aca83f408` | fullsnes, vecteurs factices du GSU | `9ef0ca93628968b7` |
| `913a9c160f2433dd` | votre ABI, valeur de retour 32 bits | `be5b663a219526ab` |
| `857cd9077cef3a88` | OAM, sprite Y+1 (verdict témoin) | `9dd075095fd0d965` |
| `a97857e79bd72cbd` | ares, `idleJump` | `c674b2a6c027ae09` |

Sur fullsnes, 1 271 anciens ids sur 1 289 se résolvent ; les autres sont
des titres orphelins ou le sommaire. Les ids qui ne se résolvent pas sont
ceux de chunks fusionnés au dédoublonnage, jamais servis.

## 3. Ce que vous pouvez faire

Rien n'est obligatoire. `snes_search` et `snes_verify` rendent désormais les
nouveaux ids ; si vous mettez à jour un commentaire, prenez celui de la
réponse. Un ancien id dans votre code continue de mener au bon texte par
`snes_get`.
