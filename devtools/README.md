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
uv run brr2it/brr2it.py input.brr output.it "SampleName" 8363

uv run gen_hud_bar/gen_hud_bar.py
```

Or activate the environment first:

```bash
source .venv/bin/activate
python3 brr2it/brr2it.py input.brr output.it "SampleName" 8363
```

## Available Tools

| Tool | Description | Doc |
|------|-------------|-----|
| [`symmap/`](symmap/) | Check WRAM memory overlaps (used by test suite) | [README](symmap/README.md) |
| [`cyclecount/`](cyclecount/) | Estimate CPU cycle costs of 65816 assembly | [README](cyclecount/README.md) |
| [`check_mvn/`](check_mvn/) | Detect suspicious MVN/MVP bank operands in assembly | [README](check_mvn/README.md) |
| [`brr2it/`](brr2it/) | Convert SNES BRR samples to Impulse Tracker (.it) | [README](brr2it/README.md) |
| [`font2snes/`](font2snes/) | Font conversion (Python reference implementation) | [README](font2snes/README.md) |
| [`gen_hud_bar/`](gen_hud_bar/) | Generate HUD health bar sprite sheets | [README](gen_hud_bar/README.md) |
| [`benchmark/`](benchmark/) | Compiler benchmark (OpenSNES vs PVSnesLib) | [README](benchmark/README.md) |
| [`snesdbg/`](snesdbg/) | Lua debugging library for Mesen2 | [README](snesdbg/README.md) |
