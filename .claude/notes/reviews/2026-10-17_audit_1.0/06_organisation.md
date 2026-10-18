# 06 — Organisation et structure du projet

Expert : organisation et structure de projet · date : 2026-10-17 · HEAD audité : `develop` @ `73a3fdfe` (v0.50.0 du 2026-10-12 + `[Unreleased]`, luna v1.38.0)

## Périmètre couvert

1. Lus : `CLAUDE.md`, les 19 fichiers de `.claude/rules/`, `.claude/settings.json` et ses trois hooks, `.claude/agents/`, `.claude/skills/`, `.claude/notes/README.md`, les six notes de `status/`, l'en-tête et l'entrée A5 de `STRUCTURAL_DEFECTS.md`, `ROADMAP.md`, `docs/STABILITY.md`, `CONTRIBUTING.md`, `MAINTAINING.md`, `.github/` (CODEOWNERS, 9 workflows, `lint.yml`, `release.yml`), `devtools/lint_commits.py`, `compiler/PINS.md`.
2. Audits précédents : `H_governance.md` et `2026-10-04_reste_a_traiter.md` (lignes H), la fiche D1-D5 et `silent_defects_log.md` (critère 7).
3. Mesuré dans l'arbre principal, en lecture seule : `git log` / `shortlog` / `numstat` sur 30 jours (614 commits sur `develop`), tailles des sections du CHANGELOG, octets chargés à chaque session, l'état des dossiers d'échange partenaires (`~/workspace/partner-reports/opensnes/`, dossier snes-rag).
4. Lancé dans le worktree `opensnes-audit` : `make lint-docs` (vert, 85 exemples), `devtools/lint_commits.py --message-file` sous une identité de contributeur fictive (refus, exit 1).
5. Pas de mesure réseau (`gh issue list` interdit par le mandat) : l'état des issues et PR GitHub n'est pas mesuré dans ce rapport.

## Points forts

- **L'historique a un seul auteur, et la règle est tenue depuis qu'elle existe.** Depuis le 2026-10-06, les 233 commits de toutes les branches ont `k0b3n4irb` pour auteur et pour committer (`git log --all --since=2026-10-06 --format=%cn | sort | uniq -c`). Plus aucun commit `GitHub <noreply@github.com>` depuis cette date, alors qu'il y en avait 110 avant.
- **Les frontières entre l'utilisateur et le contributeur sont tenues mécaniquement.** `make/common.mk` n'appelle plus aucun interpréteur (ancre 17 de `check_doc_drift.py`, `make lint-docs` vert). Les scripts de `devtools/` et `testing/` ont tous un appelant : sur tous les `.py`/`.sh` hors tests et fixtures, la seule entrée sans référence dans `Makefile`, les workflows ou les README est `devtools/vram_layout/vram_spec.py`, qui est un module importé. `devtools/README.md` dit quelles dépendances il faut (Python 3.10+ en stdlib, ortools en option).
- **Le cycle avec les partenaires fonctionne, et on peut le vérifier date par date.**
  - Côté luna : 80 fichiers dans `partners/luna/`. Les trois notes `luna-vers-opensnes` du 2026-10-12 ont chacune reçu leur réponse le jour même (`2026-10-12_to_luna_*_reply.md`). Le pin est passé à v1.38.0 le jour de sa publication (10-10, `2026-10-10_from_luna_v1.38.0-publiee.md`).
  - Côté snes-rag : depuis la consigne du 10-02, chaque fichier `to_` est livré dans son dossier.
  - Côté amont : SNESMOD est vérifié chaque semaine (prochain contrôle le 10-24) et QBE chaque mois, avec une politique écrite (`partners/qbe-upstream/README.md`, décision du 10-13).
- **Le processus de release est outillé et suivi.** Six versions mineures entre le 09-26 et le 10-12. `release.yml:27` refuse une étiquette hors de `main`. La fusion de release se fait en local (`fe7620f6 release: v0.50.0`, committer k0b3n4irb). La CI de `develop` n'annule plus ses propres runs (`lint.yml:43`, `cancel-in-progress: ${{ github.ref != 'refs/heads/develop' }}`).
- **Plusieurs constats H du 03/10 sont fermés pour de bon** (le détail est dans la dernière section) : le « 3-pillar » de `CLAUDE.md`, l'agent `snes-engine-reviewer.md:29`, « Twenty engineers », la règle « une session par arbre » (`release.md` point 7) et le paragraphe d'effort minimal du critère 7.
- **Ce qu'il faut pour reprendre le projet existe** : `MAINTAINING.md` (dépôts, pins, CI, ce qui n'est pas dans le dépôt), `CODEOWNERS`, `SECURITY.md`, quatre formulaires d'issue et un modèle de PR.

## Points faibles

**06-1 🔴 Il ne peut y avoir qu'un seul contributeur, par construction, et `CONTRIBUTING.md` dit le contraire.**
- **Preuves.**
  - Facteur bus : 1924 commits, une seule personne sous quatre graphies, aucun contributeur externe dans tout l'historique (`git log --all --format='%an <%ae>' | sort | uniq -c`).
  - Ce facteur bus est désormais imposé par l'outil. `devtools/lint_commits.py:106-107` (`AUTHOR_NAME = "k0b3n4irb"`) rejette tout autre auteur et tout autre committer. Le lint tourne sur `pull_request` (`lint.yml:24-29`) et dans le hook `commit-msg` que `CONTRIBUTING.md` demande d'installer (« Run `make hooks` once after cloning »).
  - Rejoué dans le worktree :
    `GIT_AUTHOR_NAME="Jane Doe" GIT_AUTHOR_EMAIL=jane@example.org GIT_COMMITTER_NAME="Jane Doe" GIT_COMMITTER_EMAIL=jane@example.org python3 devtools/lint_commits.py --message-file msg.txt`
    → « author is 'Jane Doe <jane@example.org>': not the maintainer's identity », exit 1.
- **Conséquence.** Un contributeur qui suit `CONTRIBUTING.md` ne peut pas faire un seul commit avec les hooks installés. Sa PR est rouge par construction. Pour être acceptée, sa contribution doit être recommise sous le nom du mainteneur, ce qui efface son attribution dans un projet MIT. Ni `CONTRIBUTING.md` ni `commits.md` ne disent ce qu'on fait d'une PR externe.
- Le critère de gel 2 attend toujours le mot du propriétaire sur le facteur bus (fiche D1-D5, l.165-172).
- **Effort** : S pour décider et écrire la règle (une liste d'auteurs autorisés, ou « la PR externe est recommise avec un trailer `Co-authored-by` humain »), M pour l'outiller.

**06-2 🔴 Le tableau de bord du critère 7 (deux semaines sans défaut silencieux) est faux.**
- **Le journal n'a pas reçu le défaut du 10-15.** `status/silent_defects_log.md` s'arrête au 2026-10-11 (dernière ligne, `255eaa84`). Or `fb429433` (10-15) corrige une mauvaise compilation silencieuse : « peephole_loop_x … stored a stale A — the variable read 0 after `for (i = 0; i < 3; i++)` ».
  - Cette règle est entrée le 10-12, et `v0.50.0:compiler/PINS.md:50` montre qu'elle est livrée dans **v0.50.0**.
  - Le correctif n'est que dans `[Unreleased]` (`CHANGELOG.md:169`).
- **La deuxième semaine de recherche n'est pas consignée.** Le tableau « Weekly effort » n'a qu'une ligne, la semaine 1 du 10-07. La règle du même fichier dit pourtant : « a week without a line restarts the count ».
- **Conséquence.** La note affiche une fermeture de la fenêtre « no earlier than 2026-10-22 », alors que, d'après ses propres règles, elle ne peut fermer qu'au 2026-10-29 au plus tôt.
- Un gel décidé sur la foi de cette note se ferait sur un chiffre faux, et la seule version publiée compile mal un motif de boucle courant.
- **Effort** : S (ajouter la ligne, consigner la semaine ou redémarrer le compteur, et décider d'une 0.50.1 → 06-8).

**06-3 🔴 La porte « jeu vitrine » n'a aucun candidat publiable, et aucune des trois portes du propriétaire n'a de date (H PF14 toujours ouvert).**
- **Jeu vitrine.** `ROADMAP.md:52` dit « Showcase game (not a port) — In progress outside this repository ». Les deux jeux en cours ne remplissent pas cette ligne :
  - Le RPG, sorti de l'arbre le 09-26 (`06930df8`), a son dernier commit le 2026-09-26 (`~/workspace/opensnes-rpg`, `ea3a0a8`) et aucun `remote` configuré. Il ne progresse pas.
  - Le seul autre jeu actif est un projet tiers sous licence, privé. Il ne peut donc pas servir de preuve publique.
- **Session console.** `docs/HARDWARE_VERIFICATION.md:170` dit toujours « No session recorded yet ».
- **Assets.** Ils sont à refaire « before a 1.0 release candidate », sans date ni personne désignée (`ROADMAP.md:53`).
- **Conséquence.** La 1.0 n'a pas de chemin daté. La ligne « In progress » de la porte vitrine est inexacte.
- **Effort** : S pour décider (réécrire la porte : un jeu original publiable, ou un exemple « jeu complet » sous licence du dépôt), L pour la livrer.

**06-4 🟠 Le pilotage de la 1.0 est éclaté entre quatre documents, et celui qui fait référence a 13 jours de retard.**
- **Où vivent les critères.** Les sept critères de gel ne figurent que dans deux notes internes en français : `status/2026-09-26_fiche_decision_D1-D5.md:150-160` (dernière mise à jour le 10-05) et `status/2026-10-03_suivi_actions.md:70-80` (10-04).
- **Ce que dit encore la seconde.** « Release v0.48.0 on the owner's go » (l.86) et « CI: develop green at `c7d58b8a` on 2026-10-04 » (l.102), alors que 0.48, 0.49 et 0.50 sont publiées.
- **Ce que dit la porte publique.** La « v1.0 gate » de `ROADMAP.md:46-57` liste six must-have, pas les sept critères, et ne renvoie à aucun d'eux.
- **Conséquence.** Personne ne peut dire, sans reconstituer l'histoire, ce qui manque à la 1.0 aujourd'hui.
- **Effort** : S (une seule table, vivante, avec un lien depuis `ROADMAP.md`).

**06-5 🟠 Des règles auto-chargées se contredisent, et la plus visible est fausse depuis cinq mois.**
- **Le rappel après compaction est faux.** `.claude/settings.json` (non modifié depuis le 2026-04-26, `43cd06cd`) injecte après chaque compaction :
  - « volatile crashes QBE — use globals instead », alors que `CLAUDE.md:110` dit « `volatile` is honoured by QBE (since chantier A2, 2026-05-09) » ;
  - « data_init_end.o MUST be linked last », alors que `CLAUDE.md` décrit désormais le tri de wlalink et le contrôle `--check-data-init`.
  - C'est le seul rappel que reçoit une longue session, et il contredit la règle.
- **`luna_tooling.md:26` contredit `exchanges.md`.** La première dit « **FILE the luna issue** » ; la seconde, « No GitHub issue between the three any more ».
- **`bank0_budget.md` se contredit lui-même.** l.1-40 : seuil 0 par défaut. l.49 : tableau « default 16 bytes ». l.68 : « Default threshold: 1024 bytes ». `make/common.mk:71` vaut 0.
- **`release.md`, checklist point 1** tolère les avertissements « only above `BANK0_FAIL_THRESHOLD` », soit au-dessus de 0 : une condition vide.
- **Conséquence.** Une session obéit à la consigne qu'elle a lue en dernier. Le sentinel (ancre 11) ne lit ni `settings.json` ni ces contradictions internes.
- **Effort** : S.

**06-6 🟠 Chaque session paie environ 28 000 jetons de règles avant de commencer, dont une large part est de l'historique.**
- **Ce qui est chargé à chaque session.** 14 règles sans `paths:` (98 102 octets) plus `CLAUDE.md` (12 517 octets), soit 110 619 octets, environ 28 k jetons à 4 caractères par jeton. Toucher `compiler/` ajoute 22 326 octets (`compiler.md`).
- **Une large part est de l'historique.** `bank0_budget.md` annonce lui-même « Everything below is the history » à la l.42 : ce sont 13 195 des 15 426 octets de la règle. Les 19 règles contiennent 130 dates (`grep -oE '20[0-9]{2}-[0-9]{2}-[0-9]{2}' .claude/rules/*.md | wc -l`), ce qui en fait des chroniques plutôt que des règles.
- **La liste de `CLAUDE.md` est incomplète.** Elle annonce 18 règles et omet `two_audiences.md` (`grep -c two_audiences CLAUDE.md` → 0).
- **Conséquence.** Le contexte est entamé et les contradictions de 06-5 augmentent. La masse elle-même est mesurée : `.claude/` suivi fait 2,61 Mo, contre 1,09 Mo pour `lib/`, sur 303 fichiers.
- **Effort** : M (séparer la règle de son histoire, et déplacer l'histoire dans `notes/archive/`).

**06-7 🟠 `STRUCTURAL_DEFECTS.md` n'est plus un catalogue utilisable.**
- **Taille.** 2730 lignes et 23 entrées, dont deux seulement ouvertes (A5, D3) ; le reste est de l'histoire.
- **A5 donne quatre comptes de patchs qui se contredisent :** « 21 cproc / 60 QBE / 2 wla-dx » (l.94, l.2562), « 86 QBE / 32 cproc / 4 wla-dx » (l.677) et « Half … of the 34 current patches » (l.737). Le réel est 46 / 134 / 5 (`compiler/PINS.md:47,101,204`).
- **La décision du propriétaire n'y figure pas.** Celle du 10-13 (« on ne contribue pas » à QBE, `partners/qbe-upstream/README.md`) rend caduc le critère d'acceptation d'A5, mais n'apparaît pas dans le catalogue.
- **Aucune mise à jour depuis le 10-05** (`git log --since=2026-10-05 -- .claude/STRUCTURAL_DEFECTS.md` → 0), alors que `CLAUDE.md` en exige une à chaque livraison ou report de chantier.
- **Effort** : S pour A5, M pour archiver les entrées closes.

**06-8 🟠 Il n'existe aucune voie pour publier un correctif 1.0.x.**
- **Le garde-fou de release l'empêche.** `release.yml:27` refuse tout tag qui n'est pas ancêtre d'`origin/main`. Aucune règle ne décrit de branche `maint/*` : `grep maint/ .claude/rules/release.md CONTRIBUTING.md MAINTAINING.md` → rien, alors que les workflows build et lint acceptent `maint/**`.
- **Le cas se présente déjà.** v0.50.0 contient une mauvaise compilation silencieuse (06-2). Il n'y a pas de 0.50.1, et le correctif part avec 59 commits de fonctionnalités (`git rev-list --count v0.50.0..develop`).
- **Conséquence.** `docs/STABILITY.md` promet des correctifs sans changer l'API, mais après la 1.0 un correctif ne pourra sortir qu'avec tout ce qui est sur `develop`.
- **Effort** : M.

**06-9 🟠 Des notes publiées décrivaient un projet tiers sous licence (corrigé le 2026-10-17 : retirées, historique réécrit).**
- **Ce qui était publié.** Des notes suivies de `.claude/` décrivaient ce projet tiers et ses mesures. `origin/main` publie 300 fichiers de `.claude/`.
- **La règle se contredisait.** `exchanges.md` interdisait qu'un détail du projet entre dans le dépôt, mais demandait aussi d'y écrire les décisions qui le concernent.
- **Conséquence.** Risque juridique et d'image pour le SDK, exposé par sa propre documentation interne.
- **Effort** : S (retirer ces notes et réécrire l'historique ; fait le 2026-10-17).

**06-10 🟠 Le rythme ne tient que par des sessions de nuit et de week-end, et produit des releases illisibles pour un utilisateur (H PF20 toujours ouvert).**
- **Le rythme.** 614 commits sur `develop` en 30 jours (108 le seul 10-05).
  - 70 % hors de 9 h-18 h (432/614), 45 % le samedi ou le dimanche (276/614), 102 entre minuit et 6 h.
  - 24 % des commits ne touchent que `.claude/` (150/614), et `docs(claude)` est le premier scope (97).
- **La taille des commits.** Médiane 69 lignes, p90 747 lignes ; 66 commits sur 305 dépassent les 400 lignes que `CONTRIBUTING.md` déclare « Too large ».
- **La taille des releases.** Les sections 0.48.0 et 0.49.0 du CHANGELOG font 825 et 949 lignes (111 et 90 entrées, en moyenne 720 caractères par entrée). Le fichier fait 6211 lignes et 358 Ko.
- **Conséquence.** Un relecteur humain ne peut pas suivre ce flux. Un utilisateur qui passe de 0.47 à 0.50 doit lire environ 2200 lignes : `UPGRADING.md` aide, mais ne remplace pas une note de version courte.
- **Effort** : M (un résumé de release en 10 lignes maximum, le détail en annexe).

**06-11 🟠 Les deux dettes héritées que la 1.0 gèlerait restent sans décision écrite (H PF16, PF17).**
- **SNESMOD.** `lib/source/sm_spc.asm` est toujours un blob de 5522 octets (173 lignes `.byte`/`.db` sur 206), corrigé par des octets édités à la main (l.20-27).
- **La décision manquante.** Aucune décision écrite ne dit que la 1.0 gèle l'API `snesmod*` au-dessus de ce blob. `docs/STABILITY.md` et `ROADMAP.md` n'en disent rien.
- **Ce qui atténue.** Le moteur `audio` a désormais sa source (`audio_driver.spc700.asm`).
- **Pour A5**, voir 06-7.
- **Effort** : S (une phrase dans `STABILITY.md` : gelé tel quel, ou hors promesse).

**06-12 🟡 Le dossier `chantiers/` (« Active multi-day chantier handoffs ») contient surtout des chantiers fermés.**
- 12 des 18 fichiers s'annoncent SHIPPED, CLOSED, DONE ou COMPLETE : `127_rodata`, `132_const_struct`, `audio_v2`, `b2_far_ram`, `c1_asm_audit`, `ecosystem`, `examples_reorg`, `function_inlining`, `hardware_docs_audit`, `qbe_access_flag`, `snesmod_65816_side`, `superfx_runtime`.
- `notes/README.md` dit pourtant « Drop, don't bury ».
- **Effort** : S (`git mv` vers `archive/`).

**06-13 🟡 Les documents de reprise ont dérivé.**
- `MAINTAINING.md` dit « Eight workflows » alors qu'il y en a 9 (`compiler-hunt.yml` en plus). Il donne encore l'audit du 09-26 comme « latest state-of-the-project audit ».
- Les huit agents `audit-*.md` portent tous le « Contexte du jour (2026-09-26) … luna épinglé v1.24.0 » (l.36-38), que le mandat de cet audit a dû remplacer.
- **Effort** : S.

**06-14 🟡 `CONTRIBUTING.md` ne prépare pas un contributeur.**
- **Il manque les prérequis du contributeur** : Python 3, clang, cppcheck, luna (`grep -i python CONTRIBUTING.md` → rien), alors que `two_audiences.md` les liste.
- **La case « Binary assets are from PVSnesLib or are original work »** contredit la porte 1.0, qui veut refaire les assets PVSnesLib.
- **« All PRs require review before merge »** n'a qu'un seul relecteur possible.
- **Les règles sur l'IA sont asymétriques.** Un contributeur externe doit déclarer son usage de l'IA dans sa PR (point 2), alors que l'historique du projet interdit toute mention d'IA (`commits.md`, « no tool identity »).
- **Effort** : S.

**06-15 🟡 La porte « tests du jour » des hooks Claude se contourne facilement.**
- Elle repose sur un marqueur `/tmp/opensnes_tests_passed_<date>` (`pre-commit-check.sh`). Le hook dit lui-même qu'il suffit de le créer avec `touch`.
- Le marqueur est commun à tous les worktrees : un `make tests` vert dans `opensnes-audit` débloque les commits de l'arbre principal le même jour.
- **Effort** : S.

**06-16 🟡 La page d'accueil du dépôt donne un mauvais nombre d'exemples, et le sentinel ne le voit pas.**
- `README.md:156` dit « 88 examples », alors que `find examples -name main.c | wc -l` donne 85 (trois exemples sont devenus des fixtures dans `deec4de4`). `make lint-docs` reste vert parce qu'aucun motif de `COUNT_PATTERNS` (`check_doc_drift.py:190-203`) ne reconnaît « N examples — ».
- → expert documentation.

## Risques

- **Le jeu pilote l'API dans les derniers jours avant le gel.** `[Unreleased]` ajoute quatre fonctions publiques demandées par un jeu bâti sur le SDK (`vramQueueCost`, `audioReleaseEcho`, `audioLoadSampleBegin/Step`) cinq jours après « the SDK carries the public API meant for 1.0 » (`ROADMAP.md:14`). Une API gelée avant d'avoir servi à un second projet risque de garder la forme d'un seul jeu, que le dépôt ne peut même pas montrer (06-9).
- **Les règles dépendent d'une seule machine.** `partners.md` et `exchanges.md` (auto-chargés) imposent des chemins locaux (`/media/psf/Home/...`, `~/workspace/partner-reports/`) et des sessions jointes par `SendMessage`. snes-rag n'est pas public (H PF18). Un second mainteneur ne pourrait suivre ni le circuit partenaires ni la règle `hardware_claims.md` sans recréer cet environnement.
- **Des sessions concurrentes travaillent sur l'arbre que le jeu consomme.** `exchanges.md` reconnaît que le jeu voit les modifications non commitées de cet arbre, et il y avait déjà eu une casse `-Werror` le 10-10. Plusieurs sessions (le jeu, les auditeurs, la session principale) partagent `bin/`, et le marqueur de tests (06-15) est commun à toutes.
- **Le rythme dépend de l'outillage IA.** Avec 108 commits un jour donné et 45 % des commits le week-end, une interruption de l'outil ou de la disponibilité du mainteneur arrête le projet. Rien n'est prévu pour ce cas.
- **Une 1.0 avec 185 patchs de compilateur maintenus seul.** cproc 46, QBE 134, wla-dx 5 (`compiler/PINS.md`), pour une promesse de stabilité « until 2.0 ».

## Améliorations recommandées

| # | Action | Sévérité traitée | Effort | Premier pas concret |
|---|---|---|---|---|
| 1 | Écrire la politique des contributions externes et l'outiller : liste d'auteurs autorisés dans `lint_commits.py`, ou recommit avec un trailer humain `Co-authored-by` explicitement permis ; puis faire statuer le propriétaire sur le critère 2 | 🔴 06-1 | S→M | Une section « External pull requests » dans `commits.md` et `CONTRIBUTING.md`, et `AUTHOR_EMAILS` lu depuis un fichier `.github/AUTHORS` |
| 2 | Remettre le critère 7 à jour : la ligne du 10-15 (`fb429433`), la semaine 2 (ou un redémarrage déclaré), et la nouvelle date de fermeture | 🔴 06-2 | S | Ajouter la ligne au tableau de `silent_defects_log.md`, puis recalculer « no earlier than » |
| 3 | Réécrire la porte vitrine pour qu'elle soit atteignable et publiable, et dater les trois portes du propriétaire | 🔴 06-3 | S (décision) | Une question d'une ligne au propriétaire : « vitrine = jeu original publiable, ou exemple complet du dépôt ? Date de la session console ? » |
| 4 | Une seule table vivante « 1.0 readiness » (sept critères + portes), liée depuis `ROADMAP.md`, et archivage du suivi du 10-03 | 🟠 06-4 | S | Remplacer la « v1.0 gate » de `ROADMAP.md` par un lien vers cette table |
| 5 | Corriger le rappel `SessionStart` de `settings.json`, l'étape 3 de `luna_tooling.md`, les titres et tableaux de `bank0_budget.md` et le point 1 de la checklist de `release.md` ; étendre l'ancre 11 à `settings.json` | 🟠 06-5 | S | Supprimer « volatile crashes QBE » et « MUST be linked last » du `SessionStart` |
| 6 | Séparer chaque règle de son histoire : la règle reste auto-chargée, l'histoire part dans `notes/archive/rules_history/` ; viser moins de 60 Ko chargés à chaque session | 🟠 06-6 | M | Commencer par `bank0_budget.md`, l.42-295 |
| 7 | Mettre A5 à jour (46/134/5, décision du 10-13, nouveau critère) et archiver les 21 entrées closes du catalogue | 🟠 06-7 | S/M | Corriger les l.94, 677, 737 et 2562 |
| 8 | Écrire la voie de correctif (`maint/X.Y` + `release.yml` qui accepte un tag sur `maint/*`), et l'essayer avec une 0.50.1 qui ne porte que `fb429433` | 🟠 06-8 | M | Une section « Patch releases » dans `release.md` |
| 9 | Retirer du dépôt ce qui décrit le projet tiers (fait le 2026-10-17, historique réécrit) | 🟠 06-9 | S | — |
| 10 | Un résumé de release de 10 lignes maximum en tête de chaque section du CHANGELOG ; espacer les versions mineures (une par semaine au plus) | 🟠 06-10 | S | Gabarit dans `release.md` |
| 11 | Une phrase dans `STABILITY.md` sur SNESMOD (gelé tel quel, ou hors promesse) | 🟠 06-11 | S | Décision du propriétaire |
| 12 | `git mv` des 12 chantiers fermés vers `archive/` ; mettre à jour `MAINTAINING.md` (9 workflows, audit du 10-17) ; ajouter `two_audiences.md` à la liste de `CLAUDE.md` ; retirer le « Contexte du jour » figé des agents d'audit | 🟡 06-12, 06-13 | S | Un commit `docs(claude)` |
| 13 | Ajouter à `CONTRIBUTING.md` les prérequis du contributeur et la règle sur les assets alignée sur `ROADMAP.md` | 🟡 06-14 | S | Une sous-section « Prerequisites » |
| 14 | Lier le marqueur de tests au worktree et au HEAD (`/tmp/opensnes_tests_passed_<toplevel-hash>_<HEAD>`) | 🟡 06-15 | S | Modifier `mark-tests-passed.sh` et `pre-commit-check.sh` |

## Verdict

Les automatismes du processus tiennent : identité unique, frontières entre l'utilisateur et le contributeur, sentinel, releases et circuit des partenaires. En revanche, les documents qui pilotent la 1.0 sont éclatés, en retard (critère 7 faux, suivi au 10-04, catalogue au 10-05) et contredits par des règles chargées à chaque session. Sur ce volet, le projet n'est pas prêt pour une 1.0 : trois portes dépendent du propriétaire et n'ont ni date ni candidat publiable, un contributeur externe est refusé par l'outillage lui-même, et rien ne permet de publier une version de correctif.

## Suivi de l'audit du 03/10

| Constat H | État le 2026-10-17 | Preuve d'aujourd'hui |
|---|---|---|
| PF3 (table de suivi vieillie) | **ouvert** (récidive) | Le critère 2 a été corrigé le 10-04 (« not held »), mais `status/2026-10-03_suivi_actions.md` s'arrête au 10-04 (`git log -1` → `0075b9ab`) et dit toujours « Release v0.48.0 on the owner's go » (l.86) → 06-4 |
| PF4 (3-pillar, `audio.asm` dans l'agent) | **corrigé** | `CLAUDE.md` dit « 2-pillar » ; `snes-engine-reviewer.md:29` : « no file is skip-marked today » |
| PF5 (« Twenty engineers ») | **corrigé** | `grep -i twenty .claude/rules/partners.md` → rien |
| PF6 (notes Mesen2/A7 hors archive) | **corrigé, mais le problème revient ailleurs** | `mesen2_*`, `a7_phase0/1`, `a6_a7_*` sont dans `archive/` ; 12 autres chantiers fermés restent dans `chantiers/` → 06-12 |
| PF7 (jetons expirés) | **non vérifiable** | Le mandat interdit de lire `.env` et d'utiliser le réseau |
| PF12 (écart `main`/`develop`) | **corrigé de fait** | Six versions en 17 jours ; 59 commits d'écart depuis v0.50.0 (5 jours). Le revers est 06-10 |
| PF14 (portes 1.0 sans date) | **ouvert**, aggravé | Pas de date dans `ROADMAP.md` ; RPG au point mort depuis le 09-26, sans remote ; l'autre jeu actif non publiable → 06-3 |
| PF15 (critères de gel absents de ROADMAP) | **partiel** | `ROADMAP.md:19` renvoie à `STABILITY.md`, mais les sept critères n'y sont toujours pas → 06-4 |
| PF16 (A5, patchs non remontés) | **partiel** | Le propriétaire a décidé le 10-13 de ne rien remonter, mais `STRUCTURAL_DEFECTS.md:737` dit toujours « 34 current patches » (réel : 185) et la branche de pin reste `feat/b2-far-qualifier` → 06-7 |
| PF17 (blob SNESMOD) | **ouvert** | `sm_spc.asm` : 173 lignes de données sur 206 ; aucune décision dans `STABILITY.md` → 06-11 |
| PF18 (snes-rag non public) | **ouvert** | `MAINTAINING.md` : « run outside this repository » → Risques |
| PF19 (facteur bus = 1, critère 2) | **ouvert**, durci | Aucun contributeur externe, et le lint d'identité refuse désormais tout autre auteur → 06-1 |
| PF20 (charge de soirées et de week-ends, docs internes) | **ouvert** | 70 % des commits hors de 9 h-18 h, 45 % le week-end, 24 % de commits `.claude/` seuls → 06-10 |
| rec 5 (effort minimal pendant la fenêtre) | **partiel** | Le paragraphe existe (`silent_defects_log.md`, « Effort during the window ») mais n'est pas appliqué : semaine 2 absente → 06-2 |
| rec 16 (une session par arbre) | **corrigé** | `.claude/rules/release.md`, point 7 de la politique wip |
