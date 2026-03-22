# SuperFX (GSU) — État d'avancement et connaissances

## Résumé

Support SuperFX en cours d'implémentation. Phase 0 (build system) et Phase 1 (boot + registres) fonctionnent.
Phase 2 (rendu bitmap PLOT) bloquée sur un problème de communication SRAM entre le GSU et le SNES CPU.

## Ce qui fonctionne (validé dans Mesen2)

### Phase 0 — Build system ✅ (commit `28b42ff`)
- `wla-superfx` compilé et installé dans `bin/`
- Pipeline 2 étapes : `.sfx` → `wla-superfx` → `wlalink -b` → `.sfx.bin` → `.incbin`
- `USE_SUPERFX=1` dans Makefile, `GSUSRC` pour les sources GSU
- `hdr_superfx.asm` (cart type $14, LoROM, SRAM 32K)
- `memmap_gsu.inc` (GSU address space)
- `lib/Makefile` build variant `superfx`

### Phase 1 — Boot + registres GSU ✅ (commit `4af0f3b`)
- `superfx_hello` : GSU exécute `IWT R0, #$CAFE; STOP`
- SNES CPU lit R0 = $CAFE après STOP → **communication bidirectionnelle via registres $3000-$303F confirmée**
- VCR ($303B) = $04 (version du chip GSU)
- `superfx.h` : définitions complètes des registres
- `crt0.asm` : init SUPERFX (SCMR=$00, BRAMR=$01, lecture VCR)
- `gsuInit()` retourne 1 si GSU détecté

## Ce qui NE fonctionne PAS encore

### Phase 2 — Rendu bitmap ❌ (en cours)

**Problème** : Le GSU écrit dans la SRAM via STW/PLOT, mais les données n'apparaissent pas
correctement quand le SNES CPU DMA depuis $70:0000 vers VRAM.

**Tests réalisés et résultats** :

| Test | Résultat | Conclusion |
|------|----------|------------|
| SNES CPU écrit directement en VRAM ($2118) | ✅ Vert uni | Tilemap + palette + mode corrects |
| SNES CPU écrit en SRAM ($70:0000) puis DMA → VRAM | ✅ Vert uni | SRAM accessible + DMA bank $70 OK |
| GSU écrit via PLOT + DMA → VRAM | ❌ Bruit coloré | PLOT ou config SCMR incorrecte |
| GSU écrit via STW (pas PLOT) + DMA → VRAM | ❌ Bruit coloré | Le GSU n'écrit pas au bon endroit |
| GSU avec RAMB explicite + STW + DMA → VRAM | ❌ Bruit coloré | RAMB ne suffit pas |

**Hypothèses à investiguer** :
1. La SRAM du GSU n'est peut-être pas mappée à $70:0000 côté SNES CPU
2. Le bus arbitration (SCMR RAN/RON) affecte peut-être l'adressage
3. La séquence de lancement depuis WRAM pourrait corrompre l'état du GSU
4. Les registres SCMR/SCBR sont peut-être écrits au mauvais moment

**Prochaine étape OBLIGATOIRE** : chercher des implémentations SuperFX homebrew existantes
et comparer notre séquence d'init/launch avec du code qui fonctionne. Ne plus deviner.

## Architecture technique

### GSU (Graphics Support Unit)
- CPU RISC 16-bit, 10.74 / 21.47 MHz
- 16 registres 16-bit (R0-R15), R15 = PC
- ISA unique (~55 mnémoniques), pas compatible 65816
- Pas de pile hardware (LINK/RET via R11)
- Instruction PLOT : écriture de pixels en format bitplane SNES
- **Pas de compilateur C** — code GSU = assembleur uniquement

### Mémoire
```
┌─────────────────────────────────┐
│         ROM (partagé)           │
│  Bus EXCLUSIF : un seul CPU    │
│  à la fois (SCMR RON bit)      │
└──────────┬──────────────────────┘
           │
    ┌──────┼──────┐
    │      │      │
┌───┴───┐ ┌┴────┐ ┌┴──────┐
│SNES   │ │SRAM │ │GSU    │
│3.58MHz│ │32-  │ │10.74  │
│       │ │128KB│ │MHz    │
│WRAM   │ │$70: │ │       │
│PPU    │ │0000 │ │NO WRAM│
│APU    │ │     │ │NO PPU │
└───────┘ └─────┘ └───────┘
```

### Registres GSU (SNES CPU side, $3000-$303F)

| Registre | Adresse | Rôle |
|----------|---------|------|
| R0-R15 | $3000-$301F | Registres généraux 16-bit. **Écrire R15H ($301F) démarre le GSU** |
| SFR | $3030-$3031 | Status/Flags. Bit 5 = GO (1=running). Bit 15 = IRQ |
| BRAMR | $3033 | Backup RAM register (bit 0: 1=SRAM writable par SNES CPU) |
| PBR | $3034 | Program Bank Register (banque ROM du code GSU) |
| SCBR | $3038 | Screen Base Register (base du framebuffer PLOT en SRAM) |
| CLSR | $3039 | Clock Select (0=10.74MHz, 1=21.47MHz) |
| SCMR | $303A | Screen Mode: couleur + hauteur + **bus ownership (RAN/RON)** |
| VCR | $303B | Version Code Register (read-only, $04 sur notre hardware) |
| RAMBR | $303C | RAM Bank Register (read-only côté SNES) |

### SCMR ($303A) — registre critique

```
Bit 7-6: inutilisés
Bit 5:   HT1 (hauteur bit 1)
Bit 4:   RON (1 = GSU owns ROM, SNES CPU NE PEUT PLUS lire ROM!)
Bit 3:   RAN (1 = GSU owns RAM, SNES CPU NE PEUT PLUS lire SRAM!)
Bit 2:   HT0 (hauteur bit 0)
Bit 1-0: MD (00=2bpp, 01=4bpp, 11=8bpp)

Hauteur : HT1:HT0 = 00→128px, 01→160px, 10→192px, 11→OBJ
```

### Bus exclusif — CONTRAINTE MAJEURE

Quand SCMR a RON=1, le SNES CPU ne peut PAS lire la ROM — même son propre code !
Le polling SFR doit s'exécuter depuis la **WRAM** (copie du stub dans $00:0xxx).

Séquence actuelle (dans gsu_loader.asm) :
1. SNES CPU écrit R0 (paramètre) dans $3000
2. Copie le stub de lancement en WRAM
3. JSL vers WRAM
4. [WRAM] Configure SCBR, SCMR (RAN+RON), PBR, R15 → GSU démarre
5. [WRAM] Poll SFR bit 5 (GO) jusqu'à 0
6. [WRAM] SCMR = $00 (bus revient au SNES CPU)
7. RTL vers ROM

### Pipeline d'affichage (validé)

```
GSU PLOT/STW → SRAM $70:0000 → DMA ch0 → VRAM $0000 → Tilemap identité → Écran
                                  ↑                        ↑
                          Source bank $70           VRAM $4000 (32×16 tiles)
                          Mode $01 (2-reg)         Identity: tile[i] = i
                          Dest $18 (VMDATAL)
```

## Fichiers créés

| Fichier | Statut |
|---------|--------|
| `compiler/Makefile` | ✅ Ajout `wla-superfx` |
| `templates/hdr_superfx.asm` | ✅ Header ROM (cart $14) |
| `templates/memmap_gsu.inc` | ✅ Memory map GSU |
| `templates/gsu_boot.sfx` | ✅ Boot stub par défaut |
| `make/common.mk` | ✅ USE_SUPERFX, GSUSRC, pipeline |
| `lib/Makefile` | ✅ Build variant superfx |
| `lib/include/snes/superfx.h` | ✅ Registres + API |
| `lib/source/superfx.c` | ✅ gsuInit() |
| `templates/crt0.asm` | ✅ .ifdef SUPERFX init |
| `examples/memory/superfx_hello/` | ✅ Boot diagnostic |
| `examples/graphics/effects/superfx_bitmap/` | ❌ En cours (SRAM issue) |

## Ressources à consulter

- [ ] SuperFX homebrew existants (GitHub, SMW Central)
- [ ] Code source Mesen2 pour le SuperFX (comportement exact des registres)
- [ ] SnesLab wiki SuperFX : https://sneslab.net/wiki/Super_FX
- [ ] jsgroth blog SuperFX : https://jsgroth.dev/blog/posts/snes-coprocessors-part-7/
- [ ] Super FX Development Kit (SMW Central) : https://www.smwcentral.net/?p=viewthread&t=81692
- [ ] sfxasm (assembleur alternatif) : https://github.com/MrGlockenspiel/sfxasm — peut contenir des exemples
- [ ] DiscoC (compilateur C expérimental pour GSU) : https://github.com/DiscoManOfficial/DiscoC

## Leçon apprise

**Ne pas deviner.** Chercher du code qui fonctionne, comparer pas à pas, valider chaque étape.
L'approche SA-1 (même ISA, vérification Mesen2 debugger) ne s'applique pas au SuperFX
car c'est un CPU RISC avec un bus exclusif et un modèle mémoire différent.
