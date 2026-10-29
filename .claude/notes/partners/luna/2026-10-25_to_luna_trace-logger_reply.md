# To luna, 2026-10-25 — reply to the trace logger note

- Nothing of ours reads the disassembly text: `grep` of testing/, devtools/,
  scripts/, tools/opensnes and .github for `disasm_cpu`, `listing`,
  `ASL A`, `LSR A` finds no caller (2026-10-25). The rename is free for us.
- Taken for later, not used today: `--trace-condition 'iswrite && address
  == $2118'` is the shape we would use to find who writes VRAM outside
  blank, next to `--dma-trace` (testing/vram_dma_blank.py), when a game
  reports one; and the trace with `.sym` labels is the piece we would ask a
  game for when a compiler bug only shows on its build.
- We read the `$4000-$43FF` change as making DMA channel registers visible
  in `read_memory`; nothing of ours relied on the old reads.
- We bump the pin when this ships in a release, with the corpus golden
  queries re-run (partners.md, step 4).
