# Tests — Guide complet

Ce fichier documente **tous** les tests du projet OpenSNES : quoi, quand, comment.

## TL;DR — Quand lancer quoi

| Tu as modifié... | Lance ces commandes |
|---|---|
| **N'importe quoi** (minimum) | `./tests/unit/run_unit_tests.sh` |
| `compiler/qbe/` ou `compiler/cproc/` | `./tests/compiler/run_tests.sh` + rebuild all |
| `lib/` ou `templates/common/` | `make clean && make` + `OPENSNES_HOME=$(pwd) ./tests/examples/validate_examples.sh --quick` |
| `examples/` | `OPENSNES_HOME=$(pwd) ./tests/examples/validate_examples.sh --quick` |
| **Avant tout commit** | Les 3 commandes ci-dessous |

```bash
# Triptyque obligatoire avant commit
./tests/unit/run_unit_tests.sh
./tests/compiler/run_tests.sh
OPENSNES_HOME=$(pwd) ./tests/examples/validate_examples.sh --quick
```

---

## 1. Tests unitaires (`tests/unit/`)

**Script** : `./tests/unit/run_unit_tests.sh [-v]`

19 modules de test. Chaque module est un ROM .sfc autonome qui s'exécute sur SNES.
Le runner build chaque test + vérifie les memory overlaps via `symmap.py`.

### Qualité par module

#### Tests avec assertions réelles (vérifient des valeurs)

| Module | Assertions | Ce qui est vérifié |
|--------|-----------|-------------------|
| **sprite** | 48 | `oamMemory[]` après oamSet/oamHide/oamClear/oamSetSize, high table bits, struct t_sprites |
| **entity** | 41 | Spawn/destroy, pool count, find by type, position/velocity, collision |
| **text** | 22 | Cursor tracking (textSetPos/GetX/GetY), textPutChar advance, textPrint, textInitEx |
| **math** | 24 | Fixed-point FIX/UNFIX/fixMul, sin/cos, abs, random |
| **types** | 30 + 5 static | Type sizes, overflow, arithmetic, BIT/BYTE/MIN/MAX/CLAMP macros |
| **sram** | 22 | Checksum, save/load, offset save/load, clear |
| **collision** | 21 | Rect/point/tile collision, rectInit, rectGetCenter |
| **video** | 21 + 12 static | setMode, RGB macros, color packing |
| **console** | 18 | brightness get/set, frame counter, region detect, rand/srand |
| **animation** | 18 | animInit, play, loop, oneshot, pause/resume, speed |
| **input** | 20 + 12 static | Button mask constants, padPressed/padHeld/padReleased |
| **audio** | 12 + 14 static | Struct layout AudioSample/AudioVoiceState, constantes API |

#### Smoke tests (vérifient que ça ne crashe pas, sans vérifier les valeurs)

| Module | Fonctions testées | Pourquoi smoke-only |
|--------|-------------------|---------------------|
| **background** | bgInit, bgSetScroll, bgSetMapPtr, bgSetGfxPtr | Registres PPU write-only, pas de read-back |
| **dma** | dmaCopyVram, dmaCopyCGram, dmaClearVRAM | VRAM non lisible depuis C |
| **hdma** | hdmaSetup, hdmaEnable/Disable | Registres HDMA write-only |
| **mode7** | mode7Init, SetScale, SetAngle, Transform | Registres Mode 7 write-only |
| **mosaic** | mosaicInit, SetSize, Enable/Disable | Registre mosaic write-only |
| **window** | windowInit, SetPos, Enable/Disable | Registres window write-only |
| **colormath** | colorMathInit, Enable, SetOp, SetFixedColor | Registres write-only |

Les smoke tests ont quand même des `_Static_assert` compile-time (119 au total).

### Comment ajouter un test unitaire

```
tests/unit/mon_module/
├── Makefile          # Copier un existant, changer TARGET/ROM_NAME/LIB_MODULES
├── hdr.asm           # Copier depuis un test existant, changer NAME
└── test_mon_module.c # Le code de test
```

**Pattern standard** (utilisé par tous les tests) :
```c
#include <snes.h>
#include <snes/console.h>
#include <snes/text.h>

static u8 tests_passed;
static u8 tests_failed;
static u8 test_line;

#define TEST(name, condition) do { \
    if (condition) { tests_passed++; } \
    else { tests_failed++; textPrintAt(1, test_line, "FAIL:"); \
           textPrintAt(7, test_line, name); test_line++; } \
} while(0)

// ... test functions ...

int main(void) {
    consoleInit();
    setMode(BG_MODE0, 0);
    textInit();
    // ... run tests, display results ...
    setScreenOn();
    while (1) { WaitForVBlank(); }
}
```

**Ne PAS utiliser** `tests/harness/test_harness.h` — il déclare des labels `_test_*`
qui cassent le linker WLA-DX (cross-object resolution failure avec préfixe `_`).

### Problèmes connus

- **Audio** : RAMSECTION `.audio_ram` ($0400 FORCE, 512B) chevauche `oamMemory` ($0300-$051F). Impossible de linker le module audio avec les sprites. Test limité aux constantes/structs.
- **SNESMOD** : même conflit RAMSECTION + nécessite un soundbank binaire. Pas de test unitaire.

---

## 2. Tests compiler (`tests/compiler/`)

**Script** : `./tests/compiler/run_tests.sh [-v] [--allow-known-bugs]`

54 tests de régression pour le compilateur cc65816 (cproc + QBE w65816).
Chaque test compile un fichier C et vérifie le **contenu de l'assembleur généré** (grep sur le .asm).

### Ce que ça teste

| Catégorie | Tests | Exemples |
|-----------|-------|----------|
| Binaires existent | 2 | cc65816, wla-65816 présents dans bin/ |
| Codegen basique | 5 | Compilation minimale, directives WLA-DX correctes |
| Types & struct | 6 | Struct alignment, union, nested struct, 2D array, global struct init |
| Opérateurs | 8 | Shift, division, multiplication, comparaisons, bitops, variable shift |
| Contrôle de flux | 4 | Loops, switch, function pointers, SSA phi nodes |
| Variables | 6 | Static mutable, const data en ROM, volatile, volatile ptr, globals |
| Promotions | 3 | u32 arithmetic, sign promotion, large local |
| Patterns | 3 | Animation patterns, input button masks, library no unhandled ops |
| Fonctions | 4 | Multiarg call offsets, string init, multiply, return value |
| Optimisations | 13 | Non-leaf frameless, dead store elim, PLX cleanup, XBA shift, commutative swap, inc/dec, A-cache PHA, CMP dead store, inline mul 11-15, DP registers, indirect store A-cache, tail call, lazy rep20 |

### Quand les lancer

**Obligatoire** après toute modification de :
- `compiler/qbe/w65816/` (backend 65816)
- `compiler/cproc/` (frontend C)
- `bin/cc65816` ou `bin/qbe`

**Attention** : après un changement compiler, il faut aussi `make clean && make` pour
régénérer tous les .asm (les vieux .asm avec l'ancien codegen restent sinon).

### État actuel

53/54 passent. 1 known bug : `comparisons: test_s16_shift_right` (signed right shift pattern manquant).

---

## 3. Validation des exemples (`tests/examples/`)

**Script** : `OPENSNES_HOME=$(pwd) ./tests/examples/validate_examples.sh [--quick] [--verbose]`

Valide **tous** les ROMs dans `examples/` ET `pvsneslib_examples/`.

### Checks effectués

| Check | Description | Outil |
|-------|-------------|-------|
| **Build** | `make` réussit (skip si `--quick`) | make |
| **Memory overlap** | Pas de collision WRAM Bank $00/$7E | `symmap.py --check-overlap` |
| **ROM size** | .sfc existe, 32KB-4MB, puissance de 2 | wc -c |
| **VRAM layout** | Pas de chevauchement tilemap/tileset | `vramcheck.py` (si présent) |

### Mode quick vs full

- `--quick` : skip le rebuild, valide uniquement les .sfc existants. Skip silencieusement les exemples non-buildés.
- Sans flag : clean + rebuild chaque exemple avant validation.

### Couverture actuelle

- `examples/` : 25 ROMs (tous les exemples OpenSNES)
- Total : **25 ROMs** validés

---

## 4. Tests manuels (7 exemples de référence)

Ces exemples doivent être testés **dans Mesen2** après tout changement library/compiler.
Le script automatique ne peut pas vérifier le rendu visuel ni l'interactivité.

| # | Exemple | Quoi vérifier |
|---|---------|---------------|
| 1 | `examples/games/breakout/` | Balle rebondit, raquette bouge, blocs cassent |
| 2 | `examples/games/likemario/` | Mario marche/saute, caméra scrolle |
| 3 | `examples/graphics/effects/hdma_wave/` | Distorsion ondulée visible |
| 4 | `examples/graphics/sprites/dynamic_sprite/` | Sprite animé au centre |
| 5 | `examples/graphics/backgrounds/continuous_scroll/` | BG scrolle avec D-PAD |
| 6 | `examples/audio/snesmod_music/` | Musique joue |
| 7 | `examples/memory/save_game/` | Sauvegarde/chargement fonctionne |

---

## 5. Arbre des fichiers

```
tests/
├── CLAUDE.md                       ← Ce fichier
├── README.md                       ← Doc principale
├── Makefile                        ← Coordonne le build parallèle
├── black_screen_test.lua           ← Smoke test Mesen2 (event-driven)
├── run_black_screen_tests.sh       ← Runner: teste 25 ROMs pour écrans noirs
├── test_snes_compliance.sh         ← 8 vérifications statiques (grep headers/asm)
│
├── compiler/                       ← Tests de régression compilateur
│   ├── run_tests.sh                   54 tests (C → ASM → grep patterns)
│   └── build/                         Fichiers temporaires de test
│
├── examples/                       ← Validation automatique des ROMs
│   ├── validate_examples.sh           Build + overlap + ROM size check
│   ├── compare_screenshots.sh         Comparaison visuelle PVSnesLib vs OpenSNES
│   ├── compare_screenshots.lua        Script Mesen2 pour captures
│   ├── breakout_transition_test.lua   Test Mesen2 du breakout
│   └── hello_world/
│       └── test_hello_world.lua       Test Mesen2 de hello_world
│
├── harness/                        ← Framework de test (NON UTILISÉ activement)
│   ├── test_harness.h                 API avec macros TEST_ASSERT_*
│   ├── test_harness.c                 Implémentation (labels _test_* → WLA-DX bug)
│   └── test_harness.lua               Runner Mesen2
│
└── unit/                           ← Tests unitaires library (19 modules)
    ├── run_unit_tests.sh              Build + overlap check pour chaque module
    ├── animation/
    ├── audio/                         Constants + struct (pas de link audio)
    ├── background/                    Smoke test
    ├── collision/
    ├── colormath/                     Smoke test + _Static_assert
    ├── console/
    ├── dma/                           Smoke test
    ├── entity/
    ├── hdma/                          Smoke test + _Static_assert
    ├── input/
    ├── math/
    ├── mode7/                         Smoke test + _Static_assert
    ├── mosaic/                        Smoke test + _Static_assert
    ├── sprite/                        48 assertions sur oamMemory
    ├── sram/
    ├── text/                          22 assertions cursor tracking
    ├── types/
    ├── video/
    └── window/                        Smoke test + _Static_assert
```

---

## 6. Tests Mesen2 Lua & snesdbg

### Scripts Mesen2 dans tests/

| Script | Pattern | Fonctionne ? |
|--------|---------|-------------|
| `black_screen_test.lua` | Event-driven (endFrame callback) | Oui |
| `run_black_screen_tests.sh` | Shell runner, lance Mesen2 --testrunner | Oui |
| `test_snes_compliance.sh` | Grep statique (pas de Mesen2) | Oui |
| `examples/hello_world/test_hello_world.lua` | Event-driven | Oui |
| `examples/breakout_transition_test.lua` | Event-driven | Oui |
| `examples/compare_screenshots.sh` + `.lua` | Shell + Mesen2 --testrunner | Oui |

### snesdbg (`tools/snesdbg/`)

Bibliothèque de debug Lua pour Mesen2 :
- **API event-driven** : `afterFrames(n, callback)`, `onSymbolReached(name, callback)`
- Lecture/écriture mémoire par nom de symbole
- Watches, breakpoints, assertions, OAM inspection
- Test framework BDD (`describe`/`it` avec callbacks `done()`)

Voir `tools/snesdbg/README.md` pour l'API complète.

---

## 6. Chiffres clés

| Métrique | Valeur |
|----------|--------|
| Tests unitaires (modules) | 19 |
| Assertions réelles (runtime) | ~385 |
| Assertions smoke (runtime) | ~170 |
| `_Static_assert` (compile-time) | 119 |
| Tests compilateur | 54 |
| ROMs validés (examples) | 25 |
| Exemples de référence (manuel) | 7 |

---

## 7. Règles pour les contributions

1. **Tout changement doit passer les 3 scripts** avant commit
2. **Tout nouveau module library** doit avoir un test dans `tests/unit/`
3. **Tout fix compilateur** doit avoir un test dans `tests/compiler/run_tests.sh`
4. **Les tests unitaires n'utilisent PAS test_harness.h** (bug WLA-DX labels `_`)
5. **Les hdr.asm sont gitignorés** — utiliser `git add -f` pour les nouveaux tests
6. **Les smoke tests sont marqués comme tels** — ne pas les confondre avec des vrais tests
