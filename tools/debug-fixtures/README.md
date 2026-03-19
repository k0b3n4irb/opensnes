# debug-fixtures

Test fixture ROMs for the opensnes-emu test suite. These are pre-built ROMs that test specific failure modes.

## Structure

```
debug-fixtures/
├── broken/
│   └── wram_overlap/    — ROM with intentional WRAM overlap bug
└── clean/
    └── hello_world/     — Known-good reference ROM
```

## Purpose

The opensnes-emu test runner uses these fixtures to verify that its static analysis (symmap overlap detection, bank overflow checks) correctly identifies known bugs and passes known-clean ROMs.

These are NOT examples to learn from — they are automated test inputs.
