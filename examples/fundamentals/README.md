# Fundamentals

The under-the-hood tier. Where the topic families (Text, Sprites,
Backgrounds…) show you the *easy* way with the SDK modules, these examples
strip the module away and do it by hand — raw tiles, direct VRAM writes,
hardware registers — so you can see exactly what the machine is doing.

Read the API-first version in its topic family first; come here when you want
to understand what that module hides.

## Examples

| Example | Demystifies | Description |
|---------|-------------|-------------|
| [text_glyphs](text_glyphs/) | [text/print_string](../text/print_string/) | How a glyph becomes pixels: a hand-coded 2bpp font written straight into VRAM, no `text` module |

## Why this tier exists

A good example bundle answers two different questions with two different
examples: *"how do I do X with the SDK?"* (the topic families) and *"how does
X actually work on the hardware?"* (here). Both are legitimate — the same
screen can teach a different lesson. Keep this tier small: one or two
under-the-hood examples per demystified concept, no more.
