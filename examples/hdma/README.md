# HDMA & raster

**Colour & effects — changing a register mid-frame.** Normal
DMA moves data during VBlank; **HDMA** hands the PPU a table and fires a
transfer at the start of each scanline, so a register holds a *different* value
on every line. That is how the SNES bends backgrounds, gradients the sky, and
draws effects that look impossible for a 2D tile machine.

## The ladder

| Example | Developer question |
|---------|--------------------|
| [gradient_colors](gradient_colors/) | How do I gradient the backdrop colour per scanline — with a direct table, or an indirect one? |
| [hdma_wave](hdma_wave/) | How do I build an HDMA table by hand, animate it, and then let the `hdma` module do it? |
| [hdma_helpers](hdma_helpers/) | How do I just call the `hdma` module helpers (wave, ripple, iris)? |

> `hdma_wave` is the "build the table, then use the helper" step: krom's
> hand-built table at boot, the module's `hdmaWaveH` on A.

## The idea in one screen

An HDMA table is a list of `(line count, value)` entries; the PPU walks it,
writing `value` to a target register for `count` scanlines, then moves on. Aim
it at the backdrop colour and you get a per-line gradient (`gradient_colors`);
aim it at a BG scroll register with a sine table and the background ripples
(`hdma_wave`). **Indirect** mode adds a level of pointer indirection so the
table entries point at data elsewhere (`gradient_colors`, press X). The `hdma`
module wraps the common cases (`hdmaWaveH`, `hdmaWaterRipple`, `hdmaIrisWipe`)
so you rarely hand-roll the table — but building one once (`hdma_wave`, before
you press A) is how you understand what the helpers do.
