"""testing/lib — what every harness script and fixture test imports.

`luna`: the binary, its pinned version, the repository root.
`corpus`: the example ROMs and their keys.
`probes`: run a ROM in luna and read it back (peek, assert_mem, dump_vram…).
"""
from .luna import LUNA_VERSION, REPO_ROOT, TESTING, find_luna, firmware_dir  # noqa: F401
from .corpus import discover_example_roms, example_key  # noqa: F401
from .probes import *  # noqa: F401,F403
from .probes import (  # noqa: F401  (names the star import does not carry)
    A, B, L, R, X, Y, UP, DOWN, LEFT, RIGHT, SELECT, START, Addr,
    assert_mem, capture_srm, cgram_words, dsp1_instructions, dump_vram, peek,
    peek_byte, peek_sword, peek_word, rom_path, sym_size, trace_lines, word_bytes,
)
