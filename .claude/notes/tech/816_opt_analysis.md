# 816-opt Analysis — Applicable Optimizations for OpenSNES

## Context
PVSnesLib's 816-opt is a post-compilation peephole optimizer (38 patterns, multi-pass).
We integrate optimizations directly into QBE emit.c instead.

## Already Implemented in Our Backend
- Redundant load after store (sta+lda same slot) → A-cache
- Jump to next label → Dead jump elimination

## Candidate Optimizations (ordered by estimated impact)

### HIGH IMPACT

#### 1. REP/SEP Cancellation
Our byte stores/loads emit `sep #$20; sta.l; rep #$20`. Two consecutive byte ops produce:
```
sep #$20; sta.l $XXXX; rep #$20; sep #$20; sta.l $YYYY; rep #$20
                       ^^^^^^^^^^^^^^^^^ redundant pair
```
**Implementation**: Track 8/16-bit A mode state in emitter. Skip redundant sep/rep.
**Location**: All Ostoreb, Oloadsb, Oloadub cases + hardware register writes
**Estimated savings**: 6 cycles per eliminated pair (3+3), very frequent in HW init code

#### 2. STZ for Zero Stores
`lda.w #0; sta.b addr` → `stz.b addr` (saves 3 bytes, 2 cycles)
`lda.w #0; sta N,s` → NOT applicable (stz N,s doesn't exist on 65816)
`lda.w #0; sta.l addr` → `stz.l addr` does NOT exist
**Only applicable for direct-page and absolute addressing** (stz.b, stz.w)
**Implementation**: In emitins, when storing constant 0 to dp/abs, emit stz directly
**Estimated savings**: 2 cycles per occurrence, moderate frequency

#### 3. Byte Immediate Optimization
`lda.w #N; sep #$20; sta.l; rep #$20` → `sep #$20; lda.b #N; sta.l; rep #$20`
Saves 1 byte (16-bit immediate = 3 bytes, 8-bit immediate = 2 bytes)
**Implementation**: In Ostoreb with constant value, load in 8-bit mode
**Estimated savings**: 1 byte per occurrence, cycles roughly same

### MEDIUM IMPACT

#### 4. JMP → BRA Shortening
`jmp @label` (3 bytes) → `bra @label` (2 bytes) when target is within ±127 bytes
**Challenge**: Don't know byte distances at emit time (WLA-DX resolves labels)
**Alternative**: WLA-DX may already do this optimization internally
**TODO**: Check if WLA-DX has a branch optimization pass

#### 5. LDX #0 + Indexed Addressing → Direct Addressing
`ldx #0; lda.l addr,x` → `lda.l addr`
Our compiler emits `ldx #0` when array index is 0. Could be caught in isel.c instead.
**Implementation**: In isel.c, if index is constant 0, emit non-indexed load
**Estimated savings**: 3 cycles + 2 bytes per occurrence

#### 6. Consecutive Byte Stores to Same Region
When storing multiple bytes to consecutive addresses (common in HW init):
```
sep #$20; lda.b #val1; sta.l $002100; rep #$20
sep #$20; lda.b #val2; sta.l $002101; rep #$20
```
Could merge into single 8-bit block:
```
sep #$20; lda.b #val1; sta.l $002100; lda.b #val2; sta.l $002101; rep #$20
```
**Implementation**: Track mode state, defer rep #$20 when next instruction also needs 8-bit

### LOW IMPACT (nice-to-have)

#### 7. PEA for 16-bit Constant Push
`lda.w #N; pha` → `pea.w N` (same size, but frees A register)
Not useful for us since our A-cache handles the value tracking.

#### 8. Transfer Instead of Load (tax/tay/txa/tya)
When value is in X/Y and we need it in A (or vice versa). Our compiler rarely uses X/Y
except for indirect addressing, so limited applicability.

## Implementation Strategy
- All optimizations go in `compiler/qbe/w65816/emit.c`
- Use state tracking (like acache) rather than pattern matching
- Mode tracking (8/16-bit A state) is the key enabler for #1, #3, #6
- Add benchmark functions targeting each optimization for measurement
