# opensnes-rom — is the ROM good? {#tools_opensnes_rom}

The tool that speaks last in a build (@ref tools_conventions). Today it
runs the **post-link checks** every OpenSNES project gets after `wlalink`;
the header, checksum and region work (`finalize`, `inspect`) joins it in a
later release. It is what lets a game developer build without Python: the
five scripts these checks lived in stayed with the contributors.

## Check

```sh
opensnes-rom check game.sfc                                  # the .sym and the .c.asm beside it
opensnes-rom check game.sfc --bank0-fail 1024 --ram-fail 512 # what make/common.mk passes
opensnes-rom check game.sfc --json
```

Six checks, in this order; the build fails on the first four kinds of
failure, the others are information:

| Check | What it refuses | Why it is silent on hardware |
|---|---|---|
| **bank $00 ROM** | fewer than `--bank0-fail` bytes free in the code bank (the ratchet, `.claude/rules/bank0_budget.md`); warns under `--bank0-warn` and names asset payload that sits in bank $00 | the next section that does not fit is placed elsewhere and read as garbage |
| **C RAM band** | a RAM section that crosses or sits past `$2000`; fewer than `--ram-fail` bytes free; warns under `--ram-warn` with the three largest sections | plain C RAM addressing is bank-$00-implicit; above `$1FFF` it is wrong-banked or hits the registers (`FAR` is the way above) |
| **data-init sentinel** | `DataInitEnd` not at the end of `.data_init` | an object linked after `data_init_end.o` has globals that boot uninitialised |
| **bank-blind reads** | a symbol read with 16-bit addressing (`lda.w sym`, `lda.w #sym` without `#:sym`) that the linker placed in bank $01+ | the read returns garbage; pass the data as a far pointer or keep it in bank $00 |
| **NMI / WRAM port** | a write to `$2180-$2183` in a function reachable from `NmiHandler` or from a callback given to `nmiSet` | a mid-sequence interrupt corrupts the port's address and the main thread writes to the wrong place |
| **assets** | nothing: one line with the VRAM and CGRAM weight of the converted graphics on disk | an inventory, not a budget; `opensnes budget` measures the running scene |

The figures print at every link (`OK: bank $00 ROM (code): 19177 bytes
free`, `OK: C RAM band $0000-$1FFF: 4832 bytes free …`): they are the
instrument that turns "will we hit the ceiling?" into a number.

## Output and exit codes

0 every check passed; 1 one failed (the message names the symbol, the
section or the function, and what to do); 3 no `.sym` beside the ROM.
`--json` gives one object: the free bytes of each band, each verdict, the
asset weight. `--no-bank-reads`, `--no-nmi-race`, `--no-assets` skip a
check; `make/common.mk` maps `SKIP_BANKREAD_CHECK=1` and the others onto
them.

## Where the checks came from

`symmap.py --check-bank0-overflow / --check-ram-budget / --check-data-init`,
`check_bank_reads.py`, `check_nmi_wram_race.py` and
`asset_budget.py --oneline` in `devtools/`. The port was compared against
them on every built ROM of the repository (99: same verdicts, same free
byte counts, same far-band figures). The Python stays for the contributor
gates that use it (`symmap.py --check-overlap` on the release ROMs, the
unit tests).
