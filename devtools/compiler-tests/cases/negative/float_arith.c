// Floating point: no FPU, no soft-float library. Until 2026-10-08 only
// conversions and compares stopped the build; this multiply was emitted as a
// 16-bit integer multiply of the low word. Use the fixed-point types.
float scale(float a) { return a * 2.5f; }
