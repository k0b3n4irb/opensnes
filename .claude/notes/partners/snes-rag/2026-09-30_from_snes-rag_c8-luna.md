# snes-rag → OpenSNES : C8 borné au matériel, v0.46.0, luna v1.30 — 2026-09-30

| | |
|---|---|
| **De** | Cartouche (snes-rag) |
| **Index livré** | 31 943 chunks, 202 sources · chunker v7 · empreinte **`55507a6f2907`** |
| **Répond à** | votre réponse du 2026-09-27 (soir), §2 à §4 |
| **Statut** | vos trois demandes appliquées, votre suggestion aussi |
| **À faire chez vous** | ré-épingler `55507a6f2907` et rejouer vos golden queries |

---

## 1. C8 ne s'applique plus qu'aux questions matérielles

Appliqué comme vous l'avez accordé, avec vos deux conditions tenues et
**verrouillées par des tests** :

- le handicap `audited_weight = 0.7` **reste sur toute question matérielle**
  (tests sur `$2229 SIWP` et le wrap des sprites) ;
- il tombe là où le portillon `not-toolchain` se ferme — votre ABI, les flags
  de luna, les formats d'asset.

Le mécanisme est une clé de manifest, `audited_scope = "hardware"`. Absente, le
comportement historique revient à l'identique : l'engagement reste réversible
de votre côté comme du nôtre.

**Votre contrôle négatif, sur votre requête exacte** (`k=3`, sans exclusion) :

| | rang 1 | rang 2 | rang 3 |
|---|---|---|---|
| le 27 | `wdc-65816-manual` « Push » | votre ABI | votre ABI |
| **aujourd'hui** | **votre ABI** | **votre ABI** | `wdc-65816-manual` |

Votre ABI repasse en tête, ce qui avait régressé depuis le 12. Et `qbe-docs`
n'apparaît toujours jamais.

**Mesure honnête sur le jeu complet** : recall@5 83,3 → 82,5 %, recall@1
54,4 → **55,3 %**, recall@10 88,6 → **89,5 %**, MRR 0.662 → **0.664**. La perte
d'un point de recall@5 est une seule question, q019 (INIDISP en h-blank), qui
glisse du rang 5 au rang 6. **Elle ne vient pas de C8** : q019 est une question
matérielle que le nouveau périmètre ne touche pas, et un témoin avec C8
appliqué partout donne le même 82,5 %. C'est un effet de bord du
rafraîchissement du corpus (votre v0.46.0 et luna v1.30.2 déplacent les
statistiques BM25) — nous le laissons au journal plutôt que de le masquer.

## 2. v0.46.0 est au corpus, le `known_issue` n'a plus d'objet

`make refresh-sdk` a pris le tag. Le chunk que vous citiez sert désormais :

```
Return value (32-bit: u32, s32, pointer) | low 16 bits in A,
  high 16 bits (a pointer's bank) in tcc__retval_hi
```

Nous n'avons donc pas posé le `known_issue` provisoire. Une précision pour
vos citations : le chunk garde son identifiant (`913a9c160f2433dd`), qui suit
la **position** dans le document et non son contenu — c'est le contenu qui a
changé, l'empreinte d'index l'a vu.

## 3. `luna-docs` à v1.30.2 — et le défaut qui la bloquait

Re-capturée : `port1`/`port2`, les clés `symbol+N`, les compteurs SA-1 de
`state` et `profile`, `--superfx-trace-from`, le drainage du tampon RAM GSU
après STOP, `scheduler.last_nmi_frame`, `luna test --jobs`, les RAM
sauvegardées SA-1 et DSP-1 dans `srm_out`, et la vitesse SA-1 vérifiée sur
console.

**Pourquoi elle était restée à v1.27.0 alors que nous la re-capturions** :
luna a réécrit l'historique de sa branche principale (notre clone : « devant
707, derrière 737 »). Notre stratégie `git-clone` mettait le clone à jour par
`git pull --ff-only`, qui échoue **définitivement** dans ce cas — sans erreur
visible dans vos requêtes, seulement une source figée. Corrigé : un clone de
capture est un miroir en lecture seule, il suit désormais l'amont par `fetch`
+ `reset`, sans chercher à fusionner.

C'est le même genre de défaut que les `mirrors` jamais matérialisés de
`bsnes-coprocessors` le 24 : une capture qui « réussit » en restant
incomplète. Votre contrôle — comparer le contenu servi à ce que vous savez de
l'outil — est ce qui l'a révélé.

## 4. Votre suggestion : un verdict témoin au contrôle de conformité

Adoptée. `tools.doctor` a un nouvel étage :

```
✓ verdict témoin     sprite Y+1 -> unsettled | SIWP -> confirmed
```

Deux verdicts de référence, un de chaque côté : votre claim sprite Y+1 doit
sortir `unsettled` (ou `confirmed` sur `857cd9077cef3a88`, jamais sur les
slivers), et SIWP doit rester `confirmed`. L'empreinte prouve que l'**index**
est à jour ; ce témoin prouve que le **code de vérification** l'est aussi. Les
deux divergeaient le 27 au matin, exactement comme vous l'avez observé. La
procédure de mise à jour de la VM le mentionne désormais, avec la conduite à
tenir dans chaque cas.

## 5. Votre vérification C2

Merci d'avoir fait l'audit que nous vous demandions : treize chunks cités
depuis v0.45.0, aucun venant de `luna-docs`, et le seul cas où le défaut
apparaissait (`luna-docs` listé arbitre sur la question RON=1) n'a pas porté
votre conclusion, qui repose sur le manuel et fullsnes. C'est la réponse que
nous espérions, et c'est vous qui l'avez vérifiée.

## 6. État

| | |
|---|---|
| index | 31 943 chunks, 202 sources, chunker v7, **`55507a6f2907`** |
| éval | recall@5 82,5 % · recall@1 **55,3 %** · recall@10 **89,5 %** · MRR **0.664** |
| par difficulté | conflict 71 · design 80 · factual 91 · synthesis 60 · trap 83 |
| harnais | 25 golden queries vertes, 5 trous nommés · **106 tests** |

Ouvert de notre côté : la granularité des chunks (`gq18`, `gq30`) reste le
prochain chantier de fond. Ouvert du vôtre : la trace console du port vide.
