# cproc MSYS2 segfault — investigation 2026-07-04

## Verdict

**Très probablement corrigé depuis cproc `ea95cac` (2026-03-07)** — les trois
couches de retries protègent vraisemblablement contre un fantôme depuis
quatre mois. Statut : sous surveillance (télémétrie + cron), pas encore clos.

## Timeline reconstituée

| Date | Événement |
|---|---|
| 2026-02-07 | Premiers crashs (`f91a84f0`) : unions, puis « different test files on each run » (`9d26abf6`) |
| 2026-03-05 | cproc → fichier temp au lieu d'un pipe (`845975b1`) ; retry ×3 dans cc65816 sur exit 139 (`5c46c3f6`) |
| 2026-03-06 | Retry ×3 au niveau make + `make clean` (`04aa228e`) |
| **2026-03-07** | **Fix `ea95cac` : « initialize all struct type fields in mktype() to prevent UB »** |

Le fix et les mitigations se sont croisés à un jour d'écart ; personne n'a
réévalué les crashs après le fix. Le profil du bug corrigé correspond
exactement à la signature : champs de type struct non initialisés → lectures
de garbage dépendantes du layout du tas → non-déterminisme ; les 5 fichiers
incriminés (`test_union`, `test_nested_struct`, `test_struct_ptr_init`,
`test_global_struct_init`, `test_string_init`) sont tous lourds en création
de types struct/union ; le tas UCRT place le garbage autrement que glibc,
d'où le footprint Windows-only.

## Preuves expérimentales (2026-07-04, Linux aarch64, cproc pinné 7b3200b)

- ASan + UBSan sur les 156 TU du dépôt : 0 détection.
- MSan (`-fsanitize=memory -fsanitize-memory-track-origins`, détecte les
  lectures non initialisées invisibles pour ASan) : 0 détection.
- Pile bridée à 1 Mo (taille Windows par défaut) : 0 crash.
- Stress 200× de chacun des 5 fichiers incriminés sous ASan (ASLR différent
  à chaque exec) : 0 détection.
- Logs CI Windows réels (4 runs complets, 25 juin → 4 juillet, plusieurs
  centaines d'invocations cproc chacun) : 0 retry make-level ET 0 retry
  cc65816-level (`cproc segfault (attempt`) — ce dernier resterait visible
  dans le log même quand le retry réussit.

Limites : rétention des logs GitHub ≈ fin juin (mars→mai invérifiable
rétroactivement — aucune télémétrie n'existait) ; MSYS2 non reproductible
localement ; la preuve Linux est indirecte.

## Découverte annexe

Le workflow diagnostic référençait `tests/compiler/` (mort avec le submodule
opensnes-emu) — même lancé à la main il ne testait plus rien. Réparé le
2026-07-04 : chemins `devtools/compiler-tests/cases/`, cron mensuel, stress
100×, échec explicite sur segfault. Piste jamais essayée si le bug resurgit :
clang-cl + `/fsanitize=address` sur windows-latest natif (ASan Windows
existe hors MSYS2, contrairement au commentaire historique du workflow).

## Plan de clôture

1. ✅ (2026-07-04) Télémétrie : `CC65816_RETRY_LOG` dans le wrapper + rapport
   `GITHUB_STEP_SUMMARY` par build Windows (opensnes_build.yml, release.yml).
2. ✅ (2026-07-04) Workflow diagnostic réparé + cron mensuel gating.
3. ✅ (2026-07-04, décision mainteneur) Retry make-level démonté sans
   attendre — il pouvait masquer de vrais échecs de build ; le retry
   cc65816 (x3 sur exit 139) reste la seule assurance, avec compteur.
4. ⬜ Après 2-3 mois de compteur à zéro : retirer aussi le retry cc65816,
   fermer l'entrée KNOWN_LIMITATIONS, archiver cette note avec le verdict
   final.

## Références

- `KNOWN_LIMITATIONS.md` (note MSYS2) — version publique.
- `compiler/PINS.md` — patch `ea95cac` dans l'inventaire cproc.
- Rapport de session : investigation P3 du 2026-07-04.
