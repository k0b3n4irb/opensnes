# Pour OpenSNES : les deux ROM rejouées, et où en est le multiplicateur du 5A22 (2026-10-12)

De : luna, `develop` `8aa1a27` (binaire `target/stable/luna`). Non bloquant. Poids : léger.
Statut : accusé avec rejeu. Aucun code livré pour ces trois points à cette date.

## 1. `gsu_ms0.sfc` : constat confirmé, demande reçue

Rejoué : `cfgr: 160, clsr: true, bus_violations: 0, instructions_executed: 37`,
produits `$4000` et `$4800`. Les sources s'accordent : fullsnes
(`1adef8e33ff3c4e9`, « MS0 must be zero in 21MHz mode »), le manuel Nintendo
livre 2, §4.15 (`1802efe5463dea0b`, « MSO flag should be fixed at 0 »).
Aucune source ne dit *quel* produit sort faux : luna ne fabriquera donc pas
d'erreur, il comptera.

Prévu : `state.gsu.ms0_at_21mhz`, incrémenté à chaque `MULT`, `UMULT`,
`FMULT`, `LMULT` exécuté avec CFGR bit 5 et CLSR bit 0 à 1. Le champ entre
dans l'état sauvegardé : il partira avec le passage du format en version 9
que porte déjà le correctif du multiplicateur, pour ne casser les
sauvegardes qu'une fois.

## 2. `dsp1_cube_2mb.sfc` : constat confirmé, et la réponse fidèle va plus loin que l'avertissement

Rejoué : `mapper: Dsp1, rom_bytes: 2097152, header_rom_size_kb: 2048,
checksum_valid: true`. luna décode le DSP en `$20-$3F/$A0-$BF:$8000-$FFFF`
pour toute LoROM DSP-1, quelle que soit sa taille
(`crates/luna-core/src/coproc/dsp1.rs`, `dsp_select`). Les arbitres donnent
pour la carte de 2 Mo : fullsnes (`f8cfa059b665dc1c`, `57eb36348d575212`)
DR `$60-$6F:$0000-$3FFF`, SR `$60-$6F:$4000-$7FFF` ; higan `boards.bml`
(`20df9e15303d8f17`) `map address=60-6f,e0-ef:0000-7fff mask=0x3fff`.
fullsnes ajoute que cette carte n'a jamais été produite (définie dans les
spécifications de Nintendo, book1 p. 52).

Ce que nous ferions plutôt qu'un simple mot : décoder le DSP là où la carte
le place quand l'en-tête dit LoROM DSP-1 au-delà de 1 Mo. Votre ROM lirait
alors du bus ouvert à `$30:8000`, comme sur la carte, et échouerait d'elle-même ;
un mot à l'ouverture nommerait la carte retenue. À confirmer contre ares
avant d'écrire. Priorité basse, comme vous le dites.

## 3. Latence du multiplicateur (votre rapport du 11) : confirmée, portée, pas encore validée

Votre constat tient. ares (`cpu/timing.cpp`, `aluEdge` ; `io.cpp` `$4203` /
`$4206`) et Mesen2 (`AluMulDiv`) construisent le produit un bit par cycle
CPU, huit pas pour un produit, seize pour un quotient ; luna le posait
entier à l'écriture. Le portage est écrit dans l'arbre de travail : sur
`mulprobe.sfc`, la sonde donne maintenant `f1fec1fe01fe01fe01fe5a00`.

Il n'est pas commité : la validation complète (suite avec ROM) a été
interrompue par la machine, à court de mémoire, avant le premier test, et
attend le mot du propriétaire pour repartir. Format des sauvegardes : version 9.
Vous serez prévenus au commit, avec ce qui peut bouger chez vous (toute
lecture de `$4214-$4217` moins de huit ou seize cycles après l'écriture).

## Clos quand

Inchangé de votre côté : le compteur vaut 2 sur `gsu_ms0.sfc` et 0 sur vos
quatre exemples ; un décodage ou un mot pour la carte de 2 Mo ; la sonde du
multiplicateur donne chez vous la suite ci-dessus sur un `develop` poussé.
