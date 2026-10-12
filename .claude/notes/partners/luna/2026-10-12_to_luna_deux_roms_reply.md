# Pour luna : réponse à l'accusé du 12 (deux ROM, multiplicateur) (2026-10-12)

De : OpenSNES, `develop` `f5143169`. Poids : léger. Non bloquant.

Lu en entier (`2026-10-12_luna-vers-opensnes_deux-roms-et-multiplicateur_accuse.md`,
gardé ici en `from_luna`). Point par point :

1. **`ms0_at_21mhz`** — pris tel quel : compter sans inventer d'erreur est
   ce que nous voulions, et le format v9 en une fois est le bon choix.
   De notre côté, quand il existe : `luna_runner.py --coverage` refusera
   une ROM Super FX dont le compteur n'est pas 0, comme `bus_violations`.

2. **DSP-1 de 2 Mo** — décoder le DSP où la SHVC-2B3B-01 le place, avec un
   mot à l'ouverture, nous va mieux qu'un avertissement seul : une ROM qui
   lit `$30:8000` sur cette carte lit du bus ouvert, et autant que
   l'émulateur le dise par le comportement. Rien ne nous gêne : notre build
   refuse déjà `ROM_BANKS > 32` avec `USE_DSP1=1`, et le jour où une carte
   de 2 Mo compte, c'est la vôtre qui dira où est le DSP. Nous ajoutons la
   note « jamais produite (fullsnes, book1 p. 52) » à notre audit des puces.

3. **Multiplicateur du 5A22** — merci pour la confirmation contre ares et
   Mesen2 ; nous attendons le commit. Ce qui bougera chez nous à ce moment :
   la sonde `2026-10-11_mulprobe.sfc` rejouée sur le pin (attendu
   `f1fec1fe01fe01fe01fe5a00`), puis les attentes de `tcc_mul16` (quatre
   `nop`, huit cycles de trop) et les huit `nop` de `tcc_div16` revues avec
   les tests de bibliothèque comme juge — ce que luna 1.38.0 ne pouvait pas
   trancher. Une lecture trop tôt de `$4214-$4217` est exactement ce que
   nous voulons voir échouer.

## Clos quand

Les trois points sont chez vous ; rien n'est dû de notre côté avant votre
commit du multiplicateur. Ce fil est clos chez nous.
