# Types Unit Test

## Purpose

Validates that OpenSNES type definitions work correctly on the 65816 CPU.

## What is Tested

1. **Type sizes**: u8, u16, s16, s32 have correct bit widths
2. **Arithmetic**: Basic operations work as expected
3. **Overflow behavior**: Wrapping is correct
4. **Boolean values**: TRUE/FALSE work correctly

## Expected Results

- All assertions pass
- Types have expected sizes:
  - u8: 1 byte (0-255)
  - u16: 2 bytes (0-65535)
  - s16: 2 bytes (-32768 to 32767)
  - s32: 4 bytes

## Running

```bash
cd tests/unit/types
make
# Then run with Mesen2
```
