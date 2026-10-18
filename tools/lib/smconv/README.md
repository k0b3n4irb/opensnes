# smconv — library, not a tool

The converter of SNESMOD's smconv (Mukunda Johnson; via PVSnesLib, see ATTRIBUTION.md). Its binary is no longer built or shipped: the
code is linked into `opensnes-music` (the converter) and `opensnes-sample` (`brr.c`, the BRR encoder), which write the same bytes.

What is here: the IT loader, the IT-to-SPC conversion, the BRR encoder and the report; `tests/` holds the goldens, the fixtures and `itcompress.py`, which opensnes-music's suite runs.

A project that used the `smconv` command: `docs/tools/legacy.md` maps its
options to the tool that replaced it.
