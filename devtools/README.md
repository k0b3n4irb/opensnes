# devtools/ — Developer Tools (Internal)

These tools are intended for **SDK contributors and advanced developers** only.
They are **NOT distributed** in release packages.

End users should use the compiled tools in `tools/` (gfx4snes, smconv, font2snes, img2snes).

## Prerequisites

All scripts require Python 3.10+. We use [uv](https://docs.astral.sh/uv/) to manage
the Python version and dependencies.

### Install uv

```bash
curl -LsSf https://astral.sh/uv/install.sh | sh
```

### Setup

From the `devtools/` directory:

```bash
cd devtools
uv sync
```

This installs the correct Python version and all dependencies (Pillow, etc.) in
an isolated virtual environment.

### Running a script

```bash
# Using the managed environment
uv run brr2it.py input.brr output.it "SampleName" 8363

uv run gen_hud_bar.py
```

Or activate the environment first:

```bash
source .venv/bin/activate
python3 brr2it.py input.brr output.it "SampleName" 8363
```

## Available Tools

| Tool | Description | Doc |
|------|-------------|-----|
| `brr2it.py` | Convert SNES BRR samples to Impulse Tracker (.it) | [brr2it.md](brr2it.md) |
| `cyclecount.py` | Estimate CPU cycle costs of 65816 assembly | — |
| `font2snes.py` | Font conversion (Python reference implementation) | — |
| `gen_hud_bar.py` | Generate HUD health bar sprite sheets | [gen_hud_bar.md](gen_hud_bar.md) |
| `memprofiler.py` | Analyze WRAM memory usage from .sym files | — |
| `symmap.py` | Check WRAM memory overlaps (used by test suite) | — |
| `vramcheck.py` | Check VRAM layout overlaps (used by test suite) | — |
