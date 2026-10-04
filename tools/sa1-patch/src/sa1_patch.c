/*
 * sa1_patch — flip the SA-1 map-mode bits in a SNES ROM header.
 *
 * SNES carts identify themselves to the CPU via byte $7FD5 in LoROM
 * (and analogous offsets in HiROM). For an SA-1 cart, bits 0-1 of
 * that byte must be set ($23), versus $20 for plain LoROM. wlalink
 * cannot emit the SA-1 marker directly because the SA-1 cart's
 * memory map is otherwise indistinguishable from LoROM at link time,
 * so the patch happens after link as a small post-processing step.
 *
 * This used to be a one-liner Python invocation embedded inside
 * make/common.mk; the audit (P2.4 #3) called it out as a fragile
 * inline that should be replaced with a dedicated tool. This is
 * that tool — same behaviour, but a real binary that fits the
 * tools/ pattern alongside gfx4snes / smconv / etc.
 *
 * Behaviour: open the ROM read+write, OR byte $7FD5 with $03, close.
 * Idempotent. Exit 0 on success, 1 on I/O error.
 *
 * Usage:
 *     sa1_patch <rom.sfc>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAP_MODE_OFFSET   0x7FD5
#define SA1_MAP_MODE_BITS 0x03   /* bits 0-1: SA-1 = $23 vs LoROM = $20 */
#define COMPLEMENT_OFFSET 0x7FDC /* checksum complement, then the checksum at $7FDE */
#define CHECKSUM_OFFSET   0x7FDE

static void
usage(const char *argv0)
{
    fprintf(stderr, "usage: %s <rom.sfc>\n", argv0);
}

int
main(int argc, char **argv)
{
    FILE *fp;
    int byte;
    const char *path;

    if (argc != 2) {
        usage(argv[0]);
        return 1;
    }
    path = argv[1];

    fp = fopen(path, "r+b");
    if (!fp) {
        perror(path);
        return 1;
    }

    if (fseek(fp, MAP_MODE_OFFSET, SEEK_SET) != 0) {
        perror("fseek");
        fclose(fp);
        return 1;
    }

    byte = fgetc(fp);
    if (byte == EOF) {
        fprintf(stderr, "%s: short read at $%X\n", path, MAP_MODE_OFFSET);
        fclose(fp);
        return 1;
    }

    if (fseek(fp, MAP_MODE_OFFSET, SEEK_SET) != 0) {
        perror("fseek");
        fclose(fp);
        return 1;
    }

    if (fputc(byte | SA1_MAP_MODE_BITS, fp) == EOF) {
        perror("fputc");
        fclose(fp);
        return 1;
    }
    /* The header checksum ($7FDE, little-endian) is the 16-bit sum of every
     * ROM byte and its complement sits at $7FDC; the linker computed them
     * for the byte we just changed. Add the difference, so the pair stays
     * right (every SA-1 ROM was off by the +3 of the mode bits until
     * 2026-10-03; luna and most emulators check only that the two fields
     * are complements, a console or a strict tool sees the mismatch). */
    {
        int delta = (byte | SA1_MAP_MODE_BITS) - byte;
        unsigned sum;
        int lo, hi;
        if (fseek(fp, CHECKSUM_OFFSET, SEEK_SET) != 0 || (lo = fgetc(fp)) == EOF
            || (hi = fgetc(fp)) == EOF) {
            fprintf(stderr, "%s: short read at $%X\n", path, CHECKSUM_OFFSET);
            fclose(fp);
            return 1;
        }
        sum = ((unsigned)hi << 8 | (unsigned)lo) + (unsigned)delta;
        sum &= 0xFFFF;
        if (fseek(fp, COMPLEMENT_OFFSET, SEEK_SET) != 0
            || fputc((sum ^ 0xFFFF) & 0xFF, fp) == EOF || fputc(((sum ^ 0xFFFF) >> 8) & 0xFF, fp) == EOF
            || fputc(sum & 0xFF, fp) == EOF || fputc((sum >> 8) & 0xFF, fp) == EOF) {
            perror("fputc");
            fclose(fp);
            return 1;
        }
    }

    if (fclose(fp) != 0) {
        perror("fclose");
        return 1;
    }
    return 0;
}
