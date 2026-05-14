---
name: OAM Y quirk — mouse/superscope pending validation
description: Two examples (mouse, superscope) using direct OAM writes have Y-1 fix applied but couldn't be visually validated due to missing peripheral hardware/setup
type: project
originSessionId: b92ad4fc-ea9c-4479-b229-46e53ee464ae
---
The SDK-wide Y-1 compensation for the SNES PPU sprite +1 scanline quirk
(commit landing 2026-04-27) updated `examples/input/mouse/main.c` and
`examples/input/superscope/main.c` to write `(u8)(y - 1)` for the cursor /
red-dot sprite position.

These two examples were **not** visually re-validated in Mesen2 because the
user does not have SNES Mouse / Super Scope peripheral mapping set up.
All other affected examples (sprites, games, sa1_starfield, collision_demo)
were validated and pass.

**Why:** Class B change touched these files; no regression suspected
(the math is the same Y-1 used in 4 validated examples), but visual
confirmation is still owed.

**How to apply:** Next time the user wires up Mesen2's mouse / Super Scope
emulation, run `mouse.sfc` and `superscope.sfc` and confirm the cursor / dot
land exactly where pointed. If misaligned by 1 px, either the peripheral
return values need a +1 adjustment, or the local Y-1 is doubling up with
the SDK fix (it shouldn't — direct OAM writes bypass `oamSet`, so Y-1 is
required, not redundant).
