#!/usr/bin/env python3
"""
brr2it.py — Convert SNES BRR sample to Impulse Tracker (.it) file

Decodes BRR (Bit Rate Reduction) to 16-bit signed PCM, then wraps it
in a minimal IT file with 1 instrument and 1 sample. The resulting .it
can be fed to smconv to generate a SNESMOD soundbank entry.

Usage:
    python3 brr2it.py input.brr output.it [sample_name] [sample_rate]

BRR format: 9-byte blocks (1 header + 8 data bytes = 16 samples).
  Header byte: RRRR FF EL
    R = shift/range (0-12), F = filter (0-3), E = end, L = loop
"""

import struct
import sys


def _sar(val, shift):
    """Arithmetic right shift (sign-preserving), matching SPC700 behavior."""
    if val >= 0:
        return val >> shift
    # Python >> on negative ints is already arithmetic, but be explicit
    return -((-val - 1) >> shift) - 1


def decode_brr(data):
    """Decode BRR data to 16-bit signed PCM samples.

    Filter coefficients match bsnes/higan DSP implementation exactly.
    """
    if len(data) % 9 != 0:
        print(f"Warning: BRR size {len(data)} not multiple of 9, truncating", file=sys.stderr)
        data = data[:len(data) - len(data) % 9]

    samples = []
    prev1 = 0  # s(n-1)
    prev2 = 0  # s(n-2)

    for block_start in range(0, len(data), 9):
        header = data[block_start]
        shift = (header >> 4) & 0x0F
        filt = (header >> 2) & 0x03

        for i in range(8):
            byte = data[block_start + 1 + i]
            # Each byte has 2 nybbles (high first)
            for nybble_idx in range(2):
                if nybble_idx == 0:
                    nybble = (byte >> 4) & 0x0F
                else:
                    nybble = byte & 0x0F

                # Sign-extend 4-bit to signed
                if nybble >= 8:
                    nybble -= 16

                # Apply shift (bsnes: shifts 13+ clamp to -2048 or 0)
                if shift <= 12:
                    sample = (nybble << shift) >> 1
                else:
                    sample = -2048 if nybble < 0 else 0

                # Apply BRR filter (bsnes/higan exact formulas)
                if filt == 1:
                    # s += p1 + ((-p1) >> 4)  ≈ p1 * 15/16
                    sample += prev1 + _sar(-prev1, 4)
                elif filt == 2:
                    # s += p1*2 + ((-p1*3) >> 5) - p2 + (p2 >> 4)
                    sample += (prev1 << 1) + _sar(-prev1 * 3, 5) \
                              - prev2 + _sar(prev2, 4)
                elif filt == 3:
                    # s += p1*2 + ((-p1*13) >> 6) - p2 + ((p2*3) >> 4)
                    sample += (prev1 << 1) + _sar(-prev1 * 13, 6) \
                              - prev2 + _sar(prev2 * 3, 4)

                # Clamp to 16-bit signed range
                if sample > 32767:
                    sample = 32767
                elif sample < -32768:
                    sample = -32768

                # Clip to 15-bit (SNES DSP uses 15-bit samples internally)
                # Equivalent to: (int16_t)(sample << 1) >> 1
                if sample > 16383:
                    sample = 16383
                elif sample < -16384:
                    sample = -16384

                prev2 = prev1
                prev1 = sample
                samples.append(sample)

        # Check end flag
        if header & 0x01:
            break

    return samples


def write_it(filename, pcm_samples, sample_name="Sample", sample_rate=16000):
    """Write a minimal IT file with 1 order, 1 pattern, 1 instrument, 1 sample."""
    name_bytes = sample_name.encode('ascii')[:25].ljust(26, b'\x00')

    num_samples = len(pcm_samples)

    # === IT Header (192 bytes) ===
    header = bytearray(192)
    header[0:4] = b'IMPM'                               # Magic
    header[4:30] = name_bytes                            # Song name (26 bytes)
    struct.pack_into('<H', header, 32, 0x0000)           # OrdNum = 2 (need at least 1 order + terminator)
    struct.pack_into('<H', header, 32, 2)                # OrdNum
    struct.pack_into('<H', header, 34, 1)                # InsNum
    struct.pack_into('<H', header, 36, 1)                # SmpNum
    struct.pack_into('<H', header, 38, 1)                # PatNum
    struct.pack_into('<H', header, 40, 0x0214)           # Cwt/v (tracker version)
    struct.pack_into('<H', header, 42, 0x0214)           # Cmwt (compatible with)
    struct.pack_into('<H', header, 44, 0x0009)           # Flags: stereo + instruments
    struct.pack_into('<H', header, 46, 0x0000)           # Special
    header[48] = 128                                     # Global volume
    header[49] = 48                                      # Mix volume
    header[50] = 125                                     # Initial speed
    header[51] = 6                                       # Initial tempo
    header[52] = 128                                     # Pan separation
    header[53] = 0                                       # Pitch wheel depth

    # Channel pan (64 channels, offset 64)
    for i in range(64):
        header[64 + i] = 32  # Center pan

    # Channel volume (64 channels, offset 128)
    for i in range(64):
        header[128 + i] = 64  # Default volume

    # === Orders (2 bytes) ===
    orders = bytes([0, 0xFF])  # Pattern 0, then end marker

    # Calculate offsets
    off_after_orders = 192 + len(orders)
    # Instrument offsets (1 pointer)
    off_after_ins_ptrs = off_after_orders + 4
    # Sample offsets (1 pointer)
    off_after_smp_ptrs = off_after_ins_ptrs + 4
    # Pattern offsets (1 pointer)
    off_after_pat_ptrs = off_after_smp_ptrs + 4

    # Instrument data starts here
    ins_offset = off_after_pat_ptrs
    ins_size = 554  # IT instrument header size

    # Sample header starts after instrument
    smp_offset = ins_offset + ins_size
    smp_header_size = 80  # IT sample header size

    # Pattern starts after sample header
    pat_offset = smp_offset + smp_header_size

    # Minimal pattern: 1 row with note C-5 instrument 1 on channel 1
    # Pattern format: length (2 bytes) + rows (2 bytes) + padding (4 bytes) + data
    pat_data = bytearray()
    # Channel 1, note + instrument + volume
    pat_data.append(0x01 | 0x80)  # Channel 1, mask variable follows
    pat_data.append(0x07)         # Mask: note + instrument + volume
    pat_data.append(60)           # Note: C-5
    pat_data.append(1)            # Instrument 1
    pat_data.append(64)           # Volume 64
    pat_data.append(0)            # End of row

    pat_header = struct.pack('<HHxxxx', len(pat_data), 1)  # length, 1 row, 4 padding bytes
    full_pattern = pat_header + pat_data

    # Sample data starts after pattern
    sample_data_offset = pat_offset + len(full_pattern)

    # === Build instrument (554 bytes) ===
    instrument = bytearray(554)
    instrument[0:4] = b'IMPI'                             # Magic
    instrument[4:17] = b'\x00' * 13                       # DOS filename
    instrument[17] = 0                                     # NNA
    instrument[18] = 0                                     # DCT
    instrument[19] = 0                                     # DCA
    struct.pack_into('<H', instrument, 20, 0)              # Fade out
    instrument[22] = 128                                   # PPS
    instrument[23] = 0                                     # PPC
    instrument[24] = 128                                   # GbV (global volume)
    instrument[25] = 128                                   # DfP (default pan, bit 7=use)
    instrument[26:30] = name_bytes[:4]                     # Name start
    # Instrument name at offset 32 (26 bytes)
    instrument[32:32+26] = name_bytes[:26]

    # Note-Sample-Instrument table (240 bytes at offset 68)
    # Map all notes to sample 1
    for note in range(120):
        offset = 68 + note * 2
        instrument[offset] = note      # Note
        instrument[offset + 1] = 1     # Sample 1

    # Volume envelope (disabled)
    instrument[308] = 0                # Flags = 0 (disabled)

    # === Build sample header (80 bytes) ===
    sample_hdr = bytearray(80)
    sample_hdr[0:4] = b'IMPS'                               # Magic
    sample_hdr[4:17] = b'\x00' * 13                          # DOS filename
    sample_hdr[17] = 127                                     # Global volume (max)
    sample_hdr[18] = 0x03                                    # Flags: sample present + 16-bit
    sample_hdr[19] = 127                                     # Default volume (max)
    sample_hdr[20:46] = name_bytes[:26]                      # Sample name
    sample_hdr[46] = 0x01                                    # Convert: signed
    sample_hdr[47] = 0                                       # Default pan
    struct.pack_into('<I', sample_hdr, 48, num_samples)      # Length (in samples)
    struct.pack_into('<I', sample_hdr, 52, 0)                # Loop begin
    struct.pack_into('<I', sample_hdr, 56, num_samples)      # Loop end
    struct.pack_into('<I', sample_hdr, 60, sample_rate)      # C5 speed
    struct.pack_into('<I', sample_hdr, 64, 0)                # SusLoop begin
    struct.pack_into('<I', sample_hdr, 68, 0)                # SusLoop end
    struct.pack_into('<I', sample_hdr, 72, sample_data_offset)  # Sample pointer
    sample_hdr[76] = 0                                       # ViS
    sample_hdr[77] = 0                                       # ViD
    sample_hdr[78] = 0                                       # ViR
    sample_hdr[79] = 0                                       # ViT

    # === Encode PCM as 16-bit signed ===
    pcm_data = b''.join(struct.pack('<h', s) for s in pcm_samples)

    # === Assemble file ===
    with open(filename, 'wb') as f:
        f.write(header)
        f.write(orders)
        # Instrument pointer
        f.write(struct.pack('<I', ins_offset))
        # Sample pointer
        f.write(struct.pack('<I', smp_offset))
        # Pattern pointer
        f.write(struct.pack('<I', pat_offset))
        # Instrument data
        f.write(instrument)
        # Sample header
        f.write(sample_hdr)
        # Pattern
        f.write(full_pattern)
        # Sample data
        f.write(pcm_data)

    return num_samples


def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} input.brr output.it [sample_name] [sample_rate]")
        sys.exit(1)

    brr_file = sys.argv[1]
    it_file = sys.argv[2]
    sample_name = sys.argv[3] if len(sys.argv) > 3 else "Sample"
    sample_rate = int(sys.argv[4]) if len(sys.argv) > 4 else 16000

    with open(brr_file, 'rb') as f:
        brr_data = f.read()

    print(f"BRR: {len(brr_data)} bytes ({len(brr_data) // 9} blocks)")

    pcm = decode_brr(brr_data)
    print(f"Decoded: {len(pcm)} PCM samples")

    num = write_it(it_file, pcm, sample_name, sample_rate)
    print(f"Wrote {it_file}: {num} samples @ {sample_rate} Hz")


if __name__ == '__main__':
    main()
