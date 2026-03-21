/**
 * @file sa1.c
 * @brief SA-1 Enhancement Chip Library
 * @author OpenSNES Team
 * @copyright MIT License
 */

#include <snes.h>
#include <snes/sa1.h>

/* sa1_status is set by crt0.asm during SA-1 init ($A5=OK, $00=failed) */
extern u8 sa1_status;

u8 sa1Init(void) {
    return (sa1_status == SA1_READY_MAGIC) ? 1 : 0;
}
