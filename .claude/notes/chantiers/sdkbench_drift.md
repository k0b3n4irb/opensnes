# Lot 2 : la dérive des sept charges, bisectée (2026-11-01)

Fait : voir §8 de `.claude/notes/reviews/2026-11-01_benchmark_pvsneslib_4_6_0.md`.
Un artefact de mesure corrigé (`d66126ec`, NmiEntry), trois compromis gardés
(mul16, mul32, boucle par objet), porte CI à +2 % (`4414374c`).

Reste ouvert, non pris : la boucle par objet perd un `tax` (A inconnu après
`bcc` au pied de la boucle, `ldx.b` à la place) : +1,6 % sur `physics`, c'est
un chantier de classe A, à ne lancer que si une autre charge le réclame.

Piège : un worktree sans `testing/bin/luna` retombe sur le luna du PATH, dont
l'attribution des cycles diffère ; copier le luna épinglé avant de mesurer.

Lot 3 fait : dessin dynamique de métasprites, voir §9 du rapport (55 048 -> 46 360).
Écart restant avec PVSnesLib à remesurer (`make bench-twins`, ROMs jumelles à construire).
Piège : `testing/manifests/movement_platform_objects_enemies.toml` échantillonnait
`insectx` en plein rappel (monde ou écran selon la phase), assoupli en `ge`.
Lot suivant : rapport §6 ligne 4 (le moteur dynamique perd des transferts en rafale) ou 5 (NMI).
