# Pour luna : les deux ROM de reproduction promises le 5 octobre (2026-10-12)

De : OpenSNES, sur luna 1.38.0 (pin `testing/luna.version`), arbre `develop`
`f5143169`. Poids : léger. Non bloquant. Statut : envoyé tel quel, avec les
pièces dans `~/workspace/partner-reports/opensnes/2026-10-12_roms/`.

Les deux lignes « tenues en réserve » de `OPEN_luna.md` depuis le 5 octobre
avaient été notées sans ROM ; les voici avec leur ROM, rejouées aujourd'hui.

## 1. Super FX : MS0 et 21 MHz ensemble, sans un mot

`gsu_ms0.sfc` (+ `.sym`, `gsu_ms0_loader.asm`, `gsu_ms0_main.c`) est
`examples/chips/superfx_hello` avec un lanceur qui écrit `$A0` dans CFGR
(`$3037`) — bit 5 MS0, multiplicateur rapide, et bit 7 masque d'IRQ — puis
`$01` dans CLSR (`$3039`, 21 MHz), là où le lanceur de la bibliothèque masque
MS0 (`lib/source/superfx.asm`, `and #$DF`). Le programme GSU fait deux
`FMULT` à résultat connu (`$2000 × $2000 → $4000`, `$1800 × $3000 → $4800`)
et les range en SRAM, que le 65816 recopie en WRAM.

```
luna state gsu_ms0.sfc --until-frame 120 \
    --peek gsu_fmult_test1:2 --peek gsu_fmult_test2:2 --out - | jq .gsu
```

donne `"cfgr": 160, "clsr": true, "bus_violations": 0, "instructions_executed": 37`
et rien d'autre ; les deux produits valent `$4000` et `$4800`, justes. Les
références disent que ce couple est interdit : fullsnes, CFGR, « MS0 must be
zero in 21MHz mode » (cartouche `1adef8e33ff3c4e9`) ; ares : les produits
« may sometimes be invalid » dans ce mode (`06c6d2324e3c6d01`). Un programme
qui écrit CFGR lui-même est donc vert chez luna et peut multiplier faux sur
une console.

Demandé : un compteur ou une note dans `state.gsu`, à côté de
`bus_violations` — par exemple `ms0_at_21mhz: N`, incrémenté à chaque `MULT`,
`UMULT`, `FMULT` ou `LMULT` exécuté avec CFGR bit 5 et CLSR bit 0 à 1 ; sur
cette ROM, 2. Ce que nous ferions avec : une ligne dans `luna_runner.py
--coverage` qui refuse une ROM Super FX dont le compteur n'est pas 0, comme
pour `bus_violations`.

## 2. DSP-1 : une LoROM de 2 Mo chargée sans un mot

`dsp1_cube_2mb.sfc` (+ `.sym`) est `examples/chips/dsp1_cube` construit avec
`ROM_BANKS=64` (notre `make` le refuse depuis le 4 octobre, `ROM_BANKS_MAX`
32 pour le DSP-1 ; forcé ici avec `ROM_BANKS_MAX=64` sur la ligne de
commande). En-tête : 2 048 Ko, somme de contrôle juste.

```
luna state dsp1_cube_2mb.sfc --until-frame 120 --out - | jq '.rom, .dsp1'
```

donne `"mapper": "Dsp1", "rom_bytes": 2097152, "header_rom_size_kb": 2048,
"checksum_valid": true` et un DSP-1 qui tourne (`"rqm": true`,
15 173 901 instructions en 120 images). Or la seule carte DSP-1 LoROM
de 2 Mo (SHVC-2B3B-01) place les registres du DSP ailleurs que ce que cette
ROM suppose (audit des puces, S5) : sur la carte, ce programme ne trouverait
pas son DSP.

Demandé, en priorité basse (notre build refuse déjà la combinaison) : un
avertissement à l'ouverture quand l'en-tête et la carte connue ne
s'accordent pas — ici « DSP-1 LoROM au-delà de 1 Mo : la seule carte connue
(SHVC-2B3B-01) ne mappe pas le DSP où cette ROM le lit », avec ce que vos
sources disent de cette carte.

## Clos quand

Pour le 1 : le compteur existe et vaut 2 sur `gsu_ms0.sfc`, 0 sur les
quatre exemples Super FX du corpus. Pour le 2 : un mot à l'ouverture, ou un
non justifié (par exemple si la carte est documentée autrement chez vous).
