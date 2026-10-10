# Compiler Toolchain Pins

This file is the **source of truth** for which commit of each compiler submodule
the OpenSNES SDK is built against. Advancing a submodule pointer requires
updating this file in the same commit. `make verify-toolchain` enforces it,
and CI runs the check before every build.

## Why this exists

The audit (`~/opensnes_audit_2026-04-26.md` §2.1) flagged that the three
compiler submodules float on upstream without rationale:

> Si un commit upstream casse silencieusement la codegen 65816 (déjà arrivé :
> `mktype()` UB, struct init, signed promotion — corrigés *après* avoir cassé
> des ROMs), il n'y a aucun buffer.

A `git submodule update --remote` would silently advance any of them past the
point where the test suite is known to pass. The pin table below names
exactly which commit each submodule must be at for the SDK to be considered
"green". Anyone running `make verify-toolchain` gets a hard failure if drift
is detected.

## Pinned commits

The block between the BEGIN/END markers is parsed by
`devtools/verify_toolchain.py`. Format: `| path | sha | source |`. Do not
reformat without updating the script.

<!-- BEGIN PINS -->
| path | sha | source |
|------|-----|--------|
| compiler/cproc | 63ad4e9c6005701716f394a7c051799186a64053 | github.com/k0b3n4irb/cproc:feat/b2-far-qualifier |
| compiler/qbe | d8314088a91ee4ddd5e300750f883002170c638f | github.com/k0b3n4irb/qbe:feat/b2-far-qualifier |
| compiler/wla-dx | 8077133acf80a1515f71e40a16c81ac3d9890978 | github.com/k0b3n4irb/wla-dx:opensnes/ram-labels-ignore-base (v10.7 + 4) |
<!-- END PINS -->

## Local patches carried on top of upstream

These commits exist only on the OpenSNES forks and must survive any sync
with upstream. Listed newest-first.

Each heading's count is `git rev-list --count <base>..HEAD` in the
submodule, and `devtools/verify_toolchain.py` fails when it is not (a
shallow clone skips the check with a note). Update the number in the
commit that moves the pin.

### compiler/cproc — 37 patches since upstream merge-base 7051114

```
63ad4e9 expr: a conditional's result has the conditional's type, bit-field or not — `b3 - (3 ? bf.b2 : u)` was computed signed (difftest_stmt seed 103247, 2026-10-09)
dbc4f9f qbe: 64-bit integers are refused, their constants folded first (2026-10-08)
80d542e qbe, eval: conditions and logical constants at the w65816 widths — three silent miscompilations (difftest, 2026-10-08)
11db807 qbe: file-scope statics are emitted name.<TU> so two sources may share a static (2026-10-05)
a1474c4 qbe, expr: qualifiers and widths the w65816 target dropped — four silent miscompilations (2026-10-03 campaign)
354a845 OpenSNES: __ramcode, a function specifier for the RAM code window
771bdf0 expr: typechar.u.basic, not u.arith, in the fork's type layout (adapts 23c57a7)
c7e96cc util: Check for overflow in array grow (upstream a964406, merged with 0efca54's empty-array rule)
b9ff678 test: Add some tests for VM declarations and character constant escapes (upstream 057381e)
19b6342 expr: Prevent overflow during escape parsing in char/string literals (upstream 3ea7d07)
f285832 Fix ordinary character constants with hex/octal escapes larger than 0x7F (upstream 23c57a7)
d1f8745 qbe: bit-field extraction pads to the IR class width; long compares use the l class (c_features ROM, review C2)
0766f7d pp: no NULL + 0 when a macro call collected no argument tokens (UBSan on clang 18, upstream suite H1)
7edea70 util: arrayforeach forms no end pointer over an empty array (UBSan on clang 18, sanitizer job H3)
0efca54 qbe: never form NULL + 0 over an empty growable array (UBSan on clang 18, sanitizer job H3)
98ecf20 qbe: drop three dead fork-local symbols so cproc-qbe builds clean under -Wall -Wextra (gaps review H2)
6d3953d qbe: a pointer object's access is not tainted by its pointee's qualifiers (chantier A9)
d35c136 expr: a const pointer target satisfies __far (chantier B2, Phase 3)
f32e712  Add the __far type qualifier (chantier B2, Phase 1): QUALFAR, `farram` access flag, section ".far"; objqual() fixes pointer-to-const globals sectioned into ROM
1a626e2  qbe: namespace anonymous local-linkage globals per translation unit
e045ccc fix(qbe): int->class mapping and operand widening for a 4-byte `l`
6bdd923  feat(65816): pointer size/align 8/8 → 4/2 (chantier A6.1)
cceac4b  fix(65816): preserve volatile through QBE IR  (chantier A2)
7f26c16  fix(65816): align int/long type sizes with the w65816 target  (chantier A1)
3618c72  fix: eliminate all Clang warnings
ea95cac  fix: initialize all struct type fields in mktype() to prevent UB
801c3e6  fix: add cleanup functions to free maps, arrays, and paramtemps at exit
d15362a  fix(65816): string constification + unsigned promotion detection
03842ce  fix(qbe): pass const-qualified data sections to QBE backend
d929b94  fix(w65816): fix pointer and type handling for QBE IL generation
```

`mktype()` UB (ea95cac) was discovered after a build silently produced a struct
in ROM instead of WRAM — that's the kind of regression a careless submodule
bump would re-introduce.

The chantier-A1 patch (7f26c16) reduces `sizeof(int)` from 4 to 2 and
`sizeof(long)` from 8 to 4. The pointer size deliberately stays at 8 (its
own structural defect is tracked as A6 in the structural-defects catalogue;
reducing pointer storage cascades through QBE w65816's indirect-call emit
pass). Empirically validated against the full quick test suite.

### compiler/qbe — 109 patches since the fork's squash root 77fe846 (the bulk of the SDK's compiler magic)

Upstream base: QBE `120f316` (2025-05-30, "skip deleted phis in use width
scan"), located by blob matching on 2026-09-13 — the fork's root commit is
a squash, so `git merge-base` cannot tell. The three upstream suites run
on the fork binaries via `make test-toolchain-suites` (known-fail
ratchets in `devtools/toolchain-suites/`); QBE's `tools/test.sh` is
56/56 on the host target since `fe42eac`, and must stay so on every bump.

Selected highlights (full list via `git -C compiler/qbe log HEAD --not upstream/master --oneline`):

```
d831408 emit: the peephole drops `cmp.w #0` after the flags are set and a direct-page slot nobody reads in the function (issue #166, from a real game's output; 2026-10-10)
0288ec6 emit: a peephole over the emitted text — `sta S / lda S`, a store overwritten unread, a load overwritten, `lda S / tax` with X already holding S (issue #166, patterns 1, 2, 4; 2026-10-10)
316955e emit: a conditional over a near target is one inverted branch — `bcs @target` for `bcc + / jmp @target / +`, on an upper bound of the distance (issue #166, pattern 5; 2026-10-09)
8d514fb emit: a plain object indexed takes abs,x, not long,x — `lda.w sym,x`, a byte and a cycle less (issue #166, pattern 3; 2026-10-09)
58449ce emit: every octal escape of a string is a byte, not only \000 — `"\n"` came out as 0 '1' '2' (2026-10-09)
c794f42 w65816: the frame of a leaf function is in the direct page, `tcc__lf` (2026-10-09)
576aa1d w65816: a static function whose address is never used does not open with rep #$20 (2026-10-09)
44bfca5 copy: two phis are the same only if they have the same class — a long lost its high half to its truncated copy (difftest_stmt seed 124152, 2026-10-09)
efc6996 w65816: a 32-bit temp of which only the low half is read takes one word of frame (2026-10-09)
db935a1 w65816: the value a block returns, produced by its last instruction, gets no slot in any function (2026-10-08)
c88fc56 w65816: a parameter is read in place in every function, a temp that never touches its slot gets none, and a 32-bit multiply by 2..256 loads its operand once (2026-10-08)
e24db85 w65816: a promoted local no longer keeps its words of frame (2026-10-08)
2f09916 copy: shift widths are computed at the target's word size — `(v >> 15) & 1` lost its mask, a silent miscompilation since the fork's first commit (difftest_stmt seed 54084, 2026-10-08)
c566d99 w65816: every near access through a temp is X-indexed (`p->field` as `lda.l N,x`), and X is remembered between accesses (2026-10-08)
f6e5aea w65816: near sym[index] uses indexed-long addressing; a 16-bit value times a small constant gives its 32-bit product inline (2026-10-08)
cc3b9e0 w65816: a far access keeps its short form only for an index known >= 0 — `(far_arr + 8)[-1]` read and wrote the next bank (2026-10-08)
b2a7a26 w65816: a 32-bit compare does not take its first operand from A — an internal error on `cnel` of a 16-bit temp (difftest_stmt seed 19645, 2026-10-08)
f25d973 cfg, copy, w65816: a condition branches where it is decided — jump threading, compare scheduled last, sign and 32-bit equality tests fused; gvn's phi inference on a dead edge fixed (2026-10-08)
f832c8e gcm: the sunk copy takes the sunk operands, and what sink leaves unused is removed (2026-10-08; upstream candidate)
bbdc317 w65816: floating point is refused; a Kw temp returned as 32 bits keeps its low half (2026-10-08)
7b06495 w65816: the two ways out of a branch do not share A; leading zero-fills are data; 2048 temps, checked (difftest_stmt, 2026-10-08)
68e6e8a w65816: phi moves as a parallel copy; the high half of a Kw operand is 0; dead high halves stay dead (difftest_stmt, 2026-10-08)
b0b78af fold, gvn, w65816: constants and branches at the target's widths; a frame for a phi of constants (difftest, 2026-10-08)
a89fd88 w65816: a function in section ".ram_code" joins the RAM code window
794c6e3 w65816: temps whose lives never overlap share a stack slot (slot colouring from liveness), under a slot-ownership check
77998b5 fix exponential complexity in usewidthle() (upstream b58e2e6, cherry-picked 2026-09-26)
ceead63 w65816: every Kl read of a high word checks that a producer wrote it (Kl high-half invariant); Ocopy Kl moves both words
c3c205d w65816: the address of a local carries its bank (lib fixture: collideRect(&a, &b) read a wild bank)
1422c17 w65816: print unsigned temp ids with %u in the emitter's debug comments (cppcheck, review H4)
9e2307c w65816: five fixes from the c_features runtime ROM (variable Kl shifts, signed compares with overflow, Kl compare fusion, jnz on Kl, sign extension vs ldy) and a refusal that names the feature
d5484d4 amd64: no NULL + 0 over the argument class array of a call without arguments (UBSan on clang 18, upstream suite H1 on x86_64)
fe42eac parse: keep the type table alive until the collected functions are emitted (upstream suite H1: use-after-free at pass 2 on every target but w65816)
22568cb emit: keep upstream's ELF emitters next to the WLA-DX ones, dispatched on the target (H1: lets tools/test.sh run)
7df4820 parse: do not memset a NULL temporary hash table (UBSan, sanitizer job H3)
ca50db8 Place C const data in the memory map's asset banks by default (chantier #127.3)
852cea4 Only the volatile bit pins loads in loadopt / promote / gcm (chantier A9)
7118e4c w65816: bank-honouring codegen for far RAM (chantier B2, Phase 2)
80eaea2 Accept the `farram` access flag and section ".far" data (chantier B2, Phase 1)
8fbdc29 fix(w65816): emit the bank half for Kl phi args (conditional far pointers)
1f38c0c fix(load): teach load forwarding the target's word size
1884a20  fix(qbe): fold Osar as 32-bit signed on w65816 (chantier A7 Phase 1)
179676e  feat(w65816): chantier A6+A7 — full pointer ABI + Kl pair lowering
5c23467  fix(qbe): guard crash_handler behind __has_include(<execinfo.h>)
444edea  fix(qbe): guard inline_record_dat_ref against DStart/DEnd stack garbage
4de6a97  chore(qbe): install SIGBUS/SIGSEGV crash handler with backtrace
eaf6116  refactor(qbe): replace open_memstream with 2-pass parse architecture
2d3af4d  feat(w65816): chantier A6.8 — large-frame indirect addressing + Kl slot widening
9878b9f  fix(build): apply chantier A2 hygiene fixes to clean compile
90b81e1  fix(w65816): respect volatile loads/stores via `volat` IR keyword  (chantier A2)
d9483ee  fix(w65816): restrict leaf optimization to actual leaf functions
b064fbd  fix: eliminate all Clang warnings in QBE w65816 backend
ed0c7ee  fix(w65816): alloc computes absolute stack address via TSA
64eabff  feat(w65816): emit __sdiv16/__smod16 for signed division and modulo
fd1bebb  fix(w65816): emit .ACCU 16 and .INDEX 16 for WLA-DX register size tracking
b45ebdc  feat(w65816): composite constant multiply and inline-mul dead store elimination
ded72c5  fix(w65816): fix variable shift stack offset after pha
bab0164  feat(w65816): lazy rep #$20 for pure tail call functions
ed840fb  feat(w65816): tail call optimization for frameless functions
b56fa3d  feat(w65816): direct page .b for tcc__ registers and div/mod return in A
ea06b2f  feat(w65816): INC/DEC optimization, A-cache survives pha, dead store args
```

These commits implement the cycle reductions documented in
`~/.claude/.../memory/compiler_optimizations.md` (Phases 1 through 7a, total
−22% vs PVSnesLib baseline). Lose them and benchmarks regress.

### compiler/wla-dx — 4 patches since the v10.7 release (chantier #127.3, 2026-09-07; sanitizer job H3, 2026-09-12; HiROM RAM pointers, 2026-09-20; HiROM .sym RAM listing, 2026-09-29)

```
8077133 wlalink: the [ramsections] listing of the .sym ignores .BASE too
9002e3d wlalink: the BANK operators ignore .BASE for RAMSECTION labels too
9c784dc Fix two sanitizer findings: a one-byte read before g_tmp on short macro labels, and a signed shift overflow in wlalink's READ_T
86df331 wlalink: .BASE does not apply to RAMSECTION labels on the 65816
```

`8077133` (2026-09-29) is the third place the base leaked: the
`[ramsections]` block of the `.sym`, which listed a HiROM `$7E` section as
`13e:` and bank-0 RAM as `c0:`. No ROM byte changes (84/84 identical); the
far-RAM-band check of `symmap.py` now sees HiROM's sections.

`9002e3d` completes the first one. `86df331` fixed
`get_snes_pc_bank()`; the calculation engine has a second path to a label's
bank — the `:label` operator, `SI_OP_BANK` / `SI_OP_BANK_BYTE` — which added
the item's base unconditionally. Under `.BASE $C0` (every HiROM unit)
`pea.w :var` pushed `$C0` for a variable in a bank-`$00` RAMSECTION, so
**every C pointer to RAM carried a ROM bank on HiROM**, and any routine that
honours the bank byte of its pointer read or wrote `$C0:xxxx` instead of work
RAM. Found by an SRAM round trip on the HiROM fixture
(`testing/fixtures/libtests_hirom`). Not behaviour-neutral: HiROM and FastROM ROMs
change (the bank byte pushed for RAM pointers goes from `$C0` / `$80` to
`$00`); LoROM SlowROM ROMs are byte-identical.

The second patch is what the ASan/UBSan job found on its first run
(`make test-sanitizers`): `decode.c` read `g_tmp[-1]` on any
one-character label inside a macro (`-:` in snesmod.asm), and wlalink's
`READ_T` shifted a byte >= 128 into the sign bit of an int. Both are
upstream bugs; both fixes are behaviour-neutral (byte-identical corpus).

The first local patch on this fork was a deliberate decision (see
`.claude/rules/bank0_budget.md`): HiROM needs `.BASE $C0` on every unit
so data at offset $0000 of a high linker bank is addressed in the full
64 KB view, and upstream adds the base to WRAM labels too ($7E → $13E).

### compiler/wla-dx — previously: pinned to the **v10.7 release**

The submodule HEAD is the `v10.7` release tag (`91c52b1f`), which our
fork mirrors from upstream. Zero local patches.

**Why not the newer master.** upstream master (`4f8bbdce`, `v10.7-9`)
carries a regression: a `SUPERFREE` section that exactly fills a ROM bank
fails to link (`FIX_LABEL_ADDRESSES: cannot map label`). Bisected to
`4c3c042e` (`v10.7-7`, "Added SPAN to .SECTIONs"). The v10.7 **release**
predates it (`v10.7-0`) and is clean — verified by a full corpus build +
suite on 2026-07-25 (74/74 fbhash, 72/72 WRAM, all identical to the
previous `ffe59ca1` pin, so the 90-commit advance is behaviour-neutral).
See `.claude/notes/tech/wla_span_regression.md` and the draft upstream
report. Do not advance past `a369bec5` (`v10.7-6`) until the regression
is fixed upstream.

## Updating a pin

When a real reason exists to bump a submodule (security fix, feature merge,
upstream resync), do this in **one** commit:

```sh
# 1. Move the submodule to the new SHA
cd compiler/qbe
git fetch
git checkout <new-sha>
cd ../..

# 2. Run the full test suite — this is the gate
make tests
# Must end with `ALL CHECKS PASSED (luna)`. Investigate any failure first.

# 3. Update PINS.md: replace the SHA in the table above, document any new
#    local patches in the per-submodule list. Keep the rationale terse.

# 4. Stage both changes and commit together
cd /path/to/opensnes
git add compiler/qbe compiler/PINS.md
git commit -m "chore(submodule): bump qbe to <short-sha> (<reason>)"

# 5. Push and verify CI green. The verify-toolchain step runs before every
#    build — drift between submodule pointer and PINS.md fails the build.
```

## Drift check

```sh
make verify-toolchain
```

Reads the BEGIN/END block above, compares each entry against
`git submodule status`, and exits non-zero if any submodule's HEAD doesn't
match its pinned SHA. CI calls this before `make release` so a stale or
unauthorised submodule pointer can never produce a release artifact.

## Out of scope (intentionally)

- `docs/doxygen-awesome-css` — cosmetic, third-party, low-risk; not pinned.
- **luna** (test backend) — not a submodule; pinned as a downloaded binary via
  `testing/luna.version` + `scripts/install-luna.sh` (SHA-256 verified).
