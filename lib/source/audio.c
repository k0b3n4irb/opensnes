/**
 * @file audio.c
 * @brief OpenSNES Audio System v2 — C command layer over the resident
 *        SPC700 driver (audio_driver.spc700.asm)
 *
 * Replaces the legacy PVSnesLib-ABI audio.asm (retired 2026-07 — it
 * had zero callers and its multi-arg functions read garbage under
 * cc65816; see .claude/notes/tech/audio_legacy_pvsneslib_abi.md).
 * Pure C: the cc65816 ABI comes for free and the ABI lint has nothing
 * to verify. None of these calls sit on a per-frame hot path.
 *
 * Protocol (spec: .claude/notes/chantiers/audio_v2.md): one command at
 * a time over $2140-$2143. Command byte = (seq << 6) | opcode with seq
 * cycling 1..3, so consecutive commands always differ and the driver's
 * echo-ack is unambiguous. $FE is never sent: it is APU_RESET_MAGIC,
 * so apuReset() hot-swap works over the driver. Every wait is bounded
 * — the audio API returns AUDIO_ERR_TIMEOUT instead of hanging.
 *
 * Main-thread only: do not call audio functions from an NMI callback
 * (same stance as the DMA queue).
 */

#include <snes/audio.h>
#include <snes/apu.h>

/* APU I/O ports. Reading returns the SPC700's output latch; writing
 * sets its input latch — same address, different registers. volatile
 * is honoured by QBE (chantier A2). */
#define APU_IO0 (*(volatile u8 *)0x2140)
#define APU_IO1 (*(volatile u8 *)0x2141)
#define APU_IO2 (*(volatile u8 *)0x2142)
#define APU_IO3 (*(volatile u8 *)0x2143)

/* Driver opcodes (must match audio_driver.spc700.asm's cmd_table) */
#define OP_MVOL      0x01
#define OP_KON       0x02
#define OP_KOFF      0x03
#define OP_VVOL      0x04
#define OP_VPITCH    0x05
#define OP_VADSR     0x06
#define OP_VGAIN     0x07
#define OP_ECHO_CFG  0x08
#define OP_ECHO_FIR  0x09   /* p0 = tap 0-7; 8 = EFB extension */
#define OP_ECHO_ON   0x0A
#define OP_DIR_SET   0x0B
#define OP_LOAD      0x0C
#define OP_ENVX      0x0D
#define OP_PING      0x0E
#define OP_LOAD_SIZE 0x0F

/* ARAM sample area (spec: driver $0200-$09FF, directory $0A00,
 * samples from $0B00 up to the echo region at $C000) */
#define SAMPLE_BASE 0x0B00
#define SAMPLE_CEIL 0xC000

/* Default one-shot envelope for a voice whose ADSR/GAIN the user never
 * configured: instant attack, no decay, full sustain — KOFF releases. */
#define DEFAULT_ADSR1 0x8F
#define DEFAULT_ADSR2 0xE0

#define DRIVER_VERSION 1
#define SPC_DRIVER_BASE 0x0200

/* Bounded-wait iterations for one ack (~2 frames of slow-ROM loop). */
#define ACK_SPIN_MAX 20000

/*============================================================================
 * Module state (WRAM mirror — the driver is write-mostly; reads that
 * don't need the DSP are answered from here without an APU round-trip)
 *============================================================================*/

extern u8 audio_driver_blob[], audio_driver_blob_end[];

static u8 audio_ready;          /* PING succeeded after upload */
static u8 audio_seq;            /* command sequence bits, cycles 1..3 */
static u8 audio_mvol;           /* mirrored master volume */

typedef struct {
    u8 sample_id;
    u8 volume;
    u8 pan;
    u8 env_set;         /* user called SetADSR/SetGain for this voice */
    u16 pitch;
} VoiceMirror;

static VoiceMirror voice_mirror[AUDIO_MAX_VOICES];

/* Sample directory mirror + bump allocator (CPU side owns allocation;
 * the driver only writes the ARAM directory entries it is told to). */
static AudioSample sample_mirror[AUDIO_MAX_SAMPLES];
static u16 sample_next_free;
static u8 audio_rr_voice;   /* round-robin auto-allocation cursor */

/*============================================================================
 * Command primitive
 *============================================================================*/

/**
 * @brief Send one command to the driver and wait (bounded) for the ack.
 * @return AUDIO_OK or AUDIO_ERR_TIMEOUT
 */
static u8 cmd_send(u8 op, u8 p0, u16 p1) {
    u8 cmd;
    u16 spin;

    audio_seq = (u8)(audio_seq >= 3 ? 1 : audio_seq + 1);
    cmd = (u8)((audio_seq << 6) | op);

    APU_IO1 = p0;
    APU_IO2 = (u8)p1;
    APU_IO3 = (u8)(p1 >> 8);
    APU_IO0 = cmd;

    for (spin = 0; spin < ACK_SPIN_MAX; spin++) {
        if (APU_IO0 == cmd) {
            return AUDIO_OK;
        }
    }
    return AUDIO_ERR_TIMEOUT;
}

/*============================================================================
 * Initialization
 *============================================================================*/

void audioInit(void) {
    u8 i;

    audio_ready = 0;
    audio_seq = 0;
    audio_mvol = AUDIO_VOL_MAX;
    audio_rr_voice = 0;
    sample_next_free = SAMPLE_BASE;
    for (i = 0; i < AUDIO_MAX_VOICES; i++) {
        voice_mirror[i].sample_id = 0xFF;
        voice_mirror[i].volume = AUDIO_VOL_MAX;
        voice_mirror[i].pan = AUDIO_PAN_CENTER;
        voice_mirror[i].env_set = 0;
        voice_mirror[i].pitch = AUDIO_PITCH_DEFAULT;
    }
    for (i = 0; i < AUDIO_MAX_SAMPLES; i++) {
        sample_mirror[i].flags = 0;
    }

    apuWaitBoot();
    apuUpload(audio_driver_blob, SPC_DRIVER_BASE,
              (u16)(audio_driver_blob_end - audio_driver_blob));
    apuExecute(SPC_DRIVER_BASE);

    /* Handshake: the driver answers PING with its version on result0 */
    if (cmd_send(OP_PING, 0, 0) == AUDIO_OK && APU_IO1 == DRIVER_VERSION) {
        audio_ready = 1;
    }
}

u8 audioIsReady(void) {
    return audio_ready;
}

void audioUpdate(void) {
    /* v2 is command-driven — nothing to pump. Kept as a no-op for
     * source compatibility with the historical API. */
}

/*============================================================================
 * Master volume
 *============================================================================*/

void audioSetVolume(u8 volume) {
    if (volume > AUDIO_VOL_MAX) {
        volume = AUDIO_VOL_MAX;
    }
    if (cmd_send(OP_MVOL, volume, 0) == AUDIO_OK) {
        audio_mvol = volume;
    }
}

u8 audioGetVolume(void) {
    return audio_mvol;
}

/*============================================================================
 * Voice control
 *============================================================================*/

void audioStopVoice(u8 voice) {
    if (voice >= AUDIO_MAX_VOICES) {
        return;
    }
    cmd_send(OP_KOFF, (u8)(1 << voice), 0);
}

void audioStopAll(void) {
    cmd_send(OP_KOFF, 0xFF, 0);
}

void audioSetVoiceVolume(u8 voice, u8 volumeL, u8 volumeR) {
    if (voice >= AUDIO_MAX_VOICES) {
        return;
    }
    if (cmd_send(OP_VVOL, voice,
                 (u16)((u16)volumeR << 8 | volumeL)) == AUDIO_OK) {
        /* mirror keeps the max of both for state reporting */
        voice_mirror[voice].volume = volumeL > volumeR ? volumeL : volumeR;
    }
}

void audioSetVoicePitch(u8 voice, u16 pitch) {
    if (voice >= AUDIO_MAX_VOICES) {
        return;
    }
    if (pitch > 0x3FFF) {
        pitch = 0x3FFF;        /* DSP pitch is 14-bit */
    }
    if (cmd_send(OP_VPITCH, voice, pitch) == AUDIO_OK) {
        voice_mirror[voice].pitch = pitch;
    }
}

void audioSetADSR(u8 voice, u8 attack, u8 decay, u8 sustain, u8 release) {
    u8 adsr1, adsr2;
    if (voice >= AUDIO_MAX_VOICES) {
        return;
    }
    adsr1 = (u8)(0x80 | ((decay & 0x07) << 4) | (attack & 0x0F));
    adsr2 = (u8)(((sustain & 0x07) << 5) | (release & 0x1F));
    if (cmd_send(OP_VADSR, voice, (u16)((u16)adsr2 << 8 | adsr1)) == AUDIO_OK) {
        voice_mirror[voice].env_set = 1;
    }
}

void audioSetGain(u8 voice, u8 mode) {
    if (voice >= AUDIO_MAX_VOICES) {
        return;
    }
    /* driver also zeroes VxADSR1 so GAIN mode actually applies */
    if (cmd_send(OP_VGAIN, voice, mode) == AUDIO_OK) {
        voice_mirror[voice].env_set = 1;
    }
}

/*============================================================================
 * Sample management
 *============================================================================*/

u8 audioLoadSample(u8 id, const u8 *brrData, u16 size, u16 loopPoint) {
    u16 i, spin;
    u8 idx;

    if (id >= AUDIO_MAX_SAMPLES) {
        return AUDIO_ERR_INVALID_ID;
    }
    if (!audio_ready) {
        return AUDIO_ERR_TIMEOUT;
    }
    if (size == 0 || (u16)(SAMPLE_CEIL - sample_next_free) < size) {
        return AUDIO_ERR_NO_MEMORY;
    }

    if (cmd_send(OP_LOAD_SIZE, 0, size) != AUDIO_OK ||
        cmd_send(OP_LOAD, 0, sample_next_free) != AUDIO_OK) {
        return AUDIO_ERR_TIMEOUT;
    }

    /* Sized block stream (IPL-shaped): data on IO1, index low byte on
     * IO0; the driver stores and echoes the index. Both sides count
     * `size` bytes, so the end needs no in-band marker. */
    for (i = 0; i < size; i++) {
        idx = (u8)i;
        APU_IO1 = brrData[i];
        APU_IO0 = idx;
        for (spin = 0; spin < ACK_SPIN_MAX; spin++) {
            if (APU_IO0 == idx) {
                break;
            }
        }
        if (spin == ACK_SPIN_MAX) {
            return AUDIO_ERR_TIMEOUT;
        }
    }

    /* Epilogue: park the input latch at 0; the driver mirrors it and
     * returns to command mode (unambiguous even if the last index
     * byte was already 0 — both sides converge on 0/0). */
    APU_IO0 = 0;
    for (spin = 0; spin < ACK_SPIN_MAX; spin++) {
        if (APU_IO0 == 0) {
            break;
        }
    }
    if (spin == ACK_SPIN_MAX) {
        return AUDIO_ERR_TIMEOUT;
    }

    if (cmd_send(OP_DIR_SET, id, loopPoint) != AUDIO_OK) {
        return AUDIO_ERR_TIMEOUT;
    }

    sample_mirror[id].spcAddress = sample_next_free;
    sample_mirror[id].size = size;
    sample_mirror[id].loopPoint = loopPoint;
    sample_mirror[id].flags = 1;
    sample_mirror[id].reserved = 0;
    sample_next_free += size;
    return AUDIO_OK;
}

void audioUnloadSample(u8 id) {
    if (id >= AUDIO_MAX_SAMPLES || !sample_mirror[id].flags) {
        return;
    }
    sample_mirror[id].flags = 0;
    /* LIFO reclaim only: memory returns when the freed sample is the
     * most recently loaded one. No compaction in v2 (documented). */
    if ((u16)(sample_mirror[id].spcAddress + sample_mirror[id].size)
            == sample_next_free) {
        sample_next_free = sample_mirror[id].spcAddress;
    }
}

u8 audioGetSampleInfo(u8 id, AudioSample *info) {
    if (id >= AUDIO_MAX_SAMPLES || !info) {
        return AUDIO_ERR_INVALID_ID;
    }
    if (!sample_mirror[id].flags) {
        return AUDIO_ERR_NOT_LOADED;
    }
    /* Field-by-field on purpose: struct assignment is not lowered by
     * the w65816 backend yet (QBE blit — see the compiler issue; the
     * emitter now hard-fails on it instead of dropping the copy). */
    info->spcAddress = sample_mirror[id].spcAddress;
    info->size = sample_mirror[id].size;
    info->loopPoint = sample_mirror[id].loopPoint;
    info->flags = sample_mirror[id].flags;
    info->reserved = sample_mirror[id].reserved;
    return AUDIO_OK;
}

u16 audioGetFreeMemory(void) {
    return (u16)(SAMPLE_CEIL - sample_next_free);
}

/*============================================================================
 * Playback
 *============================================================================*/

/* pan 0..15 -> L/R 7-bit volumes, linear crossfade scaled by vol */
static void pan_to_lr(u8 vol, u8 pan, u8 *l, u8 *r) {
    if (pan > AUDIO_PAN_RIGHT) {
        pan = AUDIO_PAN_RIGHT;
    }
    *l = (u8)(((u16)vol * (u16)(AUDIO_PAN_RIGHT - pan)) / AUDIO_PAN_RIGHT);
    *r = (u8)(((u16)vol * (u16)pan) / AUDIO_PAN_RIGHT);
}

u8 audioPlaySampleEx(u8 sampleId, u8 volume, u8 pan, u16 pitch) {
    u8 voice, l, r;

    if (sampleId >= AUDIO_MAX_SAMPLES || !sample_mirror[sampleId].flags
            || !audio_ready) {
        return 0xFF;
    }
    if (volume > AUDIO_VOL_MAX) {
        volume = AUDIO_VOL_MAX;
    }
    if (pitch > 0x3FFF) {
        pitch = 0x3FFF;
    }

    voice = audio_rr_voice;
    audio_rr_voice = (u8)((audio_rr_voice + 1) & (AUDIO_MAX_VOICES - 1));

    pan_to_lr(volume, pan, &l, &r);
    if (cmd_send(OP_VVOL, voice, (u16)((u16)r << 8 | l)) != AUDIO_OK ||
        cmd_send(OP_VPITCH, voice, pitch) != AUDIO_OK) {
        return 0xFF;
    }
    if (!voice_mirror[voice].env_set) {
        if (cmd_send(OP_VADSR, voice,
                     (u16)(DEFAULT_ADSR2 << 8 | DEFAULT_ADSR1)) != AUDIO_OK) {
            return 0xFF;
        }
    }
    if (cmd_send(OP_KON, voice, sampleId) != AUDIO_OK) {
        return 0xFF;
    }

    voice_mirror[voice].sample_id = sampleId;
    voice_mirror[voice].volume = volume;
    voice_mirror[voice].pan = pan;
    voice_mirror[voice].pitch = pitch;
    return voice;
}

u8 audioPlaySample(u8 sampleId) {
    return audioPlaySampleEx(sampleId, AUDIO_VOL_MAX, AUDIO_PAN_CENTER,
                             AUDIO_PITCH_DEFAULT);
}

void audioGetVoiceState(u8 voice, AudioVoiceState *state) {
    if (voice >= AUDIO_MAX_VOICES || !state) {
        return;
    }
    /* The one DSP->CPU read: the voice's live envelope. Everything
     * else is answered from the WRAM mirror. */
    state->active = 0;
    if (cmd_send(OP_ENVX, voice, 0) == AUDIO_OK && APU_IO1 > 0) {
        state->active = 1;
    }
    state->sampleId = voice_mirror[voice].sample_id;
    state->volume = voice_mirror[voice].volume;
    state->pan = voice_mirror[voice].pan;
    state->pitch = voice_mirror[voice].pitch;
}

/*============================================================================
 * Echo
 *============================================================================*/

void audioSetEcho(u8 delay, s8 feedback, s8 volumeL, s8 volumeR) {
    if (delay < AUDIO_ECHO_DELAY_MIN) {
        delay = AUDIO_ECHO_DELAY_MIN;
    }
    if (delay > 7) {
        /* The ARAM map reserves $C000-$F7FF for the ring (7 x 2 KB max);
         * EDL 8-15 would collide with the IPL region. Documented v2 cap. */
        delay = 7;
    }
    /* EFB rides the FIR command as tap index 8 (same signed-byte shape),
     * keeping ECHO_CFG's params free for EDL + both volumes. */
    if (cmd_send(OP_ECHO_FIR, 8, (u16)(u8)feedback) != AUDIO_OK) {
        return;
    }
    cmd_send(OP_ECHO_CFG, delay,
             (u16)((u16)(u8)volumeR << 8 | (u8)volumeL));
}

void audioSetEchoFilter(const s8 fir[8]) {
    u8 i;
    if (!fir) {
        return;
    }
    for (i = 0; i < 8; i++) {
        if (cmd_send(OP_ECHO_FIR, i, (u16)(u8)fir[i]) != AUDIO_OK) {
            return;
        }
    }
}

void audioEnableEcho(u8 voiceMask) {
    cmd_send(OP_ECHO_ON, voiceMask, 0);
}

void audioDisableEcho(void) {
    /* Mask 0 makes the driver also mute EVOL and stop ring writes. */
    cmd_send(OP_ECHO_ON, 0, 0);
}
