/**
 * @file audio.h
 * @brief OpenSNES Audio System
 *
 * @note Built on the raw-APU path: a pure C command layer (`audio.c`)
 *       driving a resident SPC700 driver built from source at lib build
 *       time. The full surface below is implemented: init/ready, master
 *       volume, per-voice control, dynamic BRR loading, playback with
 *       pan/pitch, echo with FIR filter, and live voice-state readback.
 *
 * @warning Main-thread only — do not call audio functions from an NMI
 *          callback. One engine per ROM: do not link `audio` and
 *          `snesmod` together (both own the APU). `apuReset()` hot-swap
 *          works over this driver ($FE stays reserved).
 *
 * Comprehensive audio API featuring:
 * - 8 simultaneous voices with independent volume/pan/pitch
 * - Dynamic BRR sample loading (up to 64 samples)
 * - Echo/reverb effects with configurable FIR filter
 * - Per-voice ADSR envelope control
 *
 * ## Quick Start
 *
 * @code
 * // Makefile: LIB_MODULES := console audio input  (audio pulls in apu)
 * #include <snes.h>
 *
 * extern u8 beep_brr[];
 * extern u8 beep_brr_end[];
 *
 * int main(void) {
 *     consoleInit();
 *     audioInit();                       // uploads + starts the driver
 *     audioLoadSample(0, beep_brr, (u16)(beep_brr_end - beep_brr), 0);
 *
 *     while (1) {
 *         WaitForVBlank();
 *         if (padPressed(0) & KEY_A) {
 *             audioPlaySample(0);
 *         }
 *     }
 * }
 * @endcode
 *
 * ## Features
 *
 * - Up to 8 simultaneous sound effects (SPC700 voices)
 * - Up to 64 sample slots with dynamic loading/unloading
 * - Per-voice volume, pan, and pitch control
 * - Echo/reverb with configurable delay and FIR filter
 * - Per-voice ADSR envelope control
 * - Fully documented, readable SPC700 driver (~500 bytes)
 *
 * @author OpenSNES Team
 * @copyright MIT License
 */

#ifndef OPENSNES_AUDIO_H
#define OPENSNES_AUDIO_H

#include <snes/types.h>

/* Removed on 2026-10-05 (1.0 plan, lot C): audioUpdate.
 * The replacements are in docs/UPGRADING.md; `make check-upgrade` names them. */

/*============================================================================
 * Constants
 *============================================================================*/

/**
 * @defgroup audio_const Audio Constants
 * @{
 */

/** @brief Maximum number of sample slots */
#define AUDIO_MAX_SAMPLES   64

/** @brief Maximum number of voices */
#define AUDIO_MAX_VOICES    8

/** @brief Let audioPlaySampleOn() pick the voice (round-robin), as
 *         audioPlaySample() / audioPlaySampleEx() always do */
#define AUDIO_VOICE_AUTO    0xFF

/** @brief Returned by audioPlaySample() / audioPlaySampleEx() when nothing was
 *  played (driver not ready, unknown or unloaded sample, command timeout).
 *  Was a bare 0xFF in the code until 2026-09-21. */
#define AUDIO_VOICE_NONE    0xFF

/** @brief Maximum volume value */
#define AUDIO_VOL_MAX       127
#define AUDIO_VOL_MIN       0

/** @brief Pan positions (0-15 scale; 8 is an exact centre, L = R = vol/2) */
#define AUDIO_PAN_LEFT      0
#define AUDIO_PAN_CENTER    8
#define AUDIO_PAN_RIGHT     15

/** @brief Default pitch (1.0x playback rate) */
#define AUDIO_PITCH_DEFAULT 0x1000

/** @brief Common pitch values */
/* Pitch is a ratio to the sample's own rate (0x1000 = 1.0); these three
 * double per octave and assume a sample that plays just under middle C at
 * 0x1000 (~250 Hz). Until 2026-10-04 the comments were one octave off
 * their names (C3 called "Middle C"). */
#define AUDIO_PITCH_C3      0x085F  /**< C3 (~130.8 Hz with such a sample) */
#define AUDIO_PITCH_C4      0x10BE  /**< C4, middle C (~261.6 Hz) */
#define AUDIO_PITCH_C5      0x217C  /**< C5 (~523.3 Hz) */

/** @brief ADSR attack rates (0=4.1s, 15=instant) */
#define AUDIO_ATTACK_INSTANT    15
#define AUDIO_ATTACK_FAST       12
#define AUDIO_ATTACK_MEDIUM     8
#define AUDIO_ATTACK_SLOW       4

/** @brief ADSR decay rates */
#define AUDIO_DECAY_NONE        0
#define AUDIO_DECAY_FAST        7
#define AUDIO_DECAY_MEDIUM      4
#define AUDIO_DECAY_SLOW        1

/** @brief ADSR sustain levels (0=1/8, 7=full) */
#define AUDIO_SUSTAIN_FULL      7
#define AUDIO_SUSTAIN_HALF      3
#define AUDIO_SUSTAIN_QUARTER   1

/** @brief ADSR sustain RATE (ADSR2 bits 0-4): how fast the envelope keeps
 *  falling from the sustain level while the key is held. 0 holds the level
 *  for ever, 31 falls fastest. The S-DSP has no programmable release: on
 *  key-off the level drops at a fixed rate (fullsnes "Rate=31, Step=-8";
 *  snesdev-wiki Errata). These names kept the "release" word of the API;
 *  AUDIO_RELEASE_INSTANT makes a held note die away quickly, it does not
 *  cut it. (Until 2026-10-04 this comment called the field the release
 *  rate — library audit D1.) */
#define AUDIO_RELEASE_INSTANT   31
#define AUDIO_RELEASE_FAST      24
#define AUDIO_RELEASE_MEDIUM    16
#define AUDIO_RELEASE_SLOW      8

/** @brief Echo delay (delay_ms = value * 16ms) */
#define AUDIO_ECHO_DELAY_MIN    1   /**< 16ms */
#define AUDIO_ECHO_DELAY_MAX    7   /**< 112 ms — audioSetEcho() clamps here: EDL 8-15 would run the echo ring into the IPL region. Was 15, a value the function never accepted */

/** @brief Error codes */
#define AUDIO_OK                0   /**< Success */
#define AUDIO_ERR_NO_MEMORY     1   /**< SPC RAM exhausted */
#define AUDIO_ERR_INVALID_ID    2   /**< Invalid sample/voice ID */
#define AUDIO_ERR_NOT_LOADED    3   /**< Sample not loaded */
#define AUDIO_ERR_TIMEOUT       4   /**< SPC communication timeout */

/** @} */

/*============================================================================
 * Data Types
 *============================================================================*/

/**
 * @brief BRR sample descriptor
 */
typedef struct {
    u16 spcAddress;     /**< Address in SPC RAM */
    u16 size;           /**< Size in bytes */
    u16 loopPoint;      /**< Loop offset in bytes, as given to audioLoadSample() */
    u8  flags;          /**< Status flags */
    u8  reserved;       /**< Padding */
} AudioSample;

/**
 * @brief Voice state information
 */
typedef struct {
    u8  active;         /**< Non-zero if playing */
    u8  sampleId;       /**< Current sample ID */
    u8  volume;         /**< Current volume */
    u8  pan;            /**< Current pan */
    u16 pitch;          /**< Current pitch */
} AudioVoiceState;

/*============================================================================
 * Initialization
 *============================================================================*/

/**
 * @defgroup audio_init Initialization
 * @{
 */

/**
 * @brief Initialize the audio system
 *
 * Uploads the SPC700 driver (built into the lib as audio_driver_blob)
 * through the IPL boot-ROM protocol and starts it, then verifies the
 * command channel with a PING. Call once during game initialization;
 * check audioIsReady() afterwards.
 *
 * @note Blocks for the duration of the upload (~a frame for the small
 *       driver). Interrupts stay enabled; the NMI handler does not
 *       touch the APU ports.
 *
 * @return AUDIO_OK, or AUDIO_ERR_TIMEOUT if the driver did not answer the
 *         handshake (audioIsReady() then reads 0). Returned nothing until
 *         2026-09-21; so did the setters below, which now return the AUDIO_*
 *         code of the command they sent (AUDIO_ERR_INVALID_ID for a voice
 *         out of range) instead of swallowing it. Existing callers that
 *         ignore the value are unaffected.
 *
 * **Return codes of the functions that send a command** (volume, pitch,
 * envelope, key on / off, echo taps): a command is sent and the function
 * returns at once; the driver's acknowledgement is awaited by the NEXT
 * command, before it writes. So AUDIO_OK means "sent", and
 * AUDIO_ERR_TIMEOUT from one of these functions means the command BEFORE it
 * was never acknowledged (the one just asked for was then not sent).
 * Loading a sample, audioSetEcho() and the functions that read a state back
 * wait for their own acknowledgement, as before.
 */
u8 audioInit(void);

/**
 * @brief Check if audio system is ready
 *
 * audioInit() blocks until the driver answers, so there is no
 * "still initialising" state to poll: 0 after audioInit() means the SPC700
 * driver never answered (and every audio call is then a no-op).
 *
 * @return Non-zero if the driver answered audioInit()'s ping, 0 otherwise
 */
u8 audioIsReady(void);

/** @} */

/*============================================================================
 * Sample Management
 *============================================================================*/

/**
 * @defgroup audio_samples Sample Management
 * @{
 */

/**
 * @brief Load a BRR sample into SPC700 RAM
 *
 * @param id Sample slot (0-63)
 * @param brrData Pointer to BRR sample data
 * @param size Size in bytes (must be multiple of 9)
 * @param loopPoint Loop offset in bytes from the start of the sample (a
 *                  multiple of 9, one BRR block). It is where a sample that
 *                  loops comes back to — 0 for a loop from the very start.
 *                  WHETHER the sample loops is the loop flag of its last
 *                  BRR block (opensnes-sample sets it), not this value: a
 *                  one-shot sample ignores it.
 * @return AUDIO_OK on success, or error code
 *
 * @code
 * extern u8 explosion_brr[];
 * extern u8 explosion_brr_end[];
 * audioLoadSample(0, explosion_brr, explosion_brr_end - explosion_brr, 0);
 * @endcode
 */
u8 audioLoadSample(u8 id, const u8 *brrData, u16 size, u16 loopPoint);

/** @brief What audioLoadSampleStep() returns when the driver stopped answering */
#define AUDIO_LOAD_FAILED       0xFFFF

/**
 * @brief Start loading a sample in steps, while the game runs
 *
 * audioLoadSample() does not return before the whole sample is in the sound
 * CPU's memory: about 560 bytes a frame, ten frames of a stopped game for
 * 5.6 KB. This pair sends the same bytes a piece at a time, when the game
 * says so: voices that are playing go on, and voice commands (play, volume,
 * pitch, stop) pass between two steps.
 *
 * audioLoadSampleBegin() takes the place in memory and sends nothing; each
 * audioLoadSampleStep() sends a piece. The slot is "not loaded" — playing
 * it is refused — until a step returns 0.
 *
 * - One sample at a time: a second audioLoadSampleBegin() before the first
 *   is whole is refused (AUDIO_ERR_INVALID_ID).
 * - audioLoadSample() on other slots stays allowed meanwhile; those samples
 *   go above the place taken here.
 * - audioUnloadSample(id) before the last step gives the load up; the
 *   memory comes back if nothing was loaded above it.
 * - @p brrData must stay valid until the last step (it is ROM data in
 *   every case we know).
 *
 * @param id Sample slot (0-63)
 * @param brrData Pointer to the BRR data
 * @param size Size in bytes (3 at least)
 * @param loopPoint As for audioLoadSample()
 * @return AUDIO_OK; AUDIO_ERR_NO_MEMORY; AUDIO_ERR_INVALID_ID (bad slot, or
 *         a load in steps is already under way); AUDIO_ERR_TIMEOUT before
 *         audioInit()
 *
 * @code
 * audioLoadSampleBegin(SMP_VOICE, voice_brr, (u16)(voice_brr_end - voice_brr), 0);
 * // then once a frame, in the game loop:
 * if (voice_loading && audioLoadSampleStep(64) == 0) voice_loading = 0;
 * @endcode
 */
u8 audioLoadSampleBegin(u8 id, const u8 *brrData, u16 size, u16 loopPoint);

/**
 * @brief Send the next piece of the sample audioLoadSampleBegin() started
 *
 * @param maxBytes How much to send at most. Bytes travel three at a time,
 *                 so the step is @p maxBytes rounded down to a multiple of
 *                 3 (3 at least); the last step takes up to two bytes more
 *                 rather than leave them alone. The cost of a step is what
 *                 the game chooses here, plus a fixed part: see
 *                 docs/tutorials/audio_samples.md for the measured figures.
 * @return Bytes still to send; 0 when the sample is whole and playable (and
 *         when no load is under way); AUDIO_LOAD_FAILED if the driver
 *         stopped answering — the load is given up
 */
u16 audioLoadSampleStep(u16 maxBytes);

/**
 * @brief Unload a sample from a slot
 * @param id Sample slot (0-63)
 *
 * On the sample audioLoadSampleBegin() started and no step has finished:
 * gives that load up.
 *
 * It does NOT stop a voice that is playing the sample — stop it first. The
 * allocator is a bump pointer: SPC memory comes back only when the sample
 * unloaded is the most recently loaded one, and loading a slot that is
 * already loaded leaks the old block. (The "voices will be stopped" sentence
 * that stood here was never implemented.)
 */
void audioUnloadSample(u8 id);

/**
 * @brief Get information about a loaded sample
 * @param id Sample ID
 * @param info Pointer to AudioSample struct to fill
 * @return AUDIO_OK if loaded, AUDIO_ERR_NOT_LOADED otherwise
 */
u8 audioGetSampleInfo(u8 id, AudioSample *info);

/**
 * @brief Get available SPC700 RAM for samples
 *
 * The sample area starts at $0B00 of the sound CPU's 64 KB (below it: the
 * driver and the sample directory) and ends
 * - at **$FFC0, 62,656 bytes**, as long as the game has configured no
 *   echo;
 * - at the **echo ring** once audioSetEcho() has been called: the ring
 *   takes the top of the memory under $FF00, 2,048 bytes per unit of
 *   delay — 58,112 bytes of samples with a delay of 2, 47,872 with the
 *   longest, 7. audioSetEcho() answers AUDIO_ERR_NO_MEMORY when samples
 *   already loaded reach into the ring it is asked for, so a game that
 *   wants an echo calls audioSetEcho() before loading, or picks a delay
 *   its samples leave room for. A shorter delay gives memory back, and
 *   audioReleaseEcho() gives the whole ring back.
 *
 * @return Free bytes available, under the limit that applies now
 */
u16 audioGetFreeMemory(void);

/** @} */

/*============================================================================
 * Playback Control
 *============================================================================*/

/**
 * @defgroup audio_playback Playback Control
 * @{
 */

/**
 * @brief Play a sample with default settings
 *
 * @param sampleId Sample slot (0-63)
 * @return Voice number (0-7) or 0xFF on failure
 *
 * Plays at full volume, center pan, default pitch.
 */
u8 audioPlaySample(u8 sampleId);

/**
 * @brief Play a sample with custom settings
 *
 * @param sampleId Sample slot (0-63)
 * @param volume Volume level (0-127)
 * @param pan Pan position (0=left, 8=center, 15=right)
 * @param pitch Pitch value ($1000 = normal)
 * @return Voice number (0-7) or 0xFF on failure
 *
 * @code
 * audioPlaySampleEx(0, 64, AUDIO_PAN_RIGHT, 0x1800);
 * @endcode
 */
u8 audioPlaySampleEx(u8 sampleId, u8 volume, u8 pan, u16 pitch);

/**
 * @brief Play a sample on a voice YOU choose
 *
 * audioPlaySample() and audioPlaySampleEx() pick the voice round-robin and
 * only tell you afterwards. That is too late for anything that must be set
 * BEFORE key-on — audioSetADSR() / audioSetGain() shape the attack — and it
 * lets a long sound be stolen by the eighth effect after it. Choosing the
 * voice fixes both: reserve, say, voice 7 for the engine hum, aim its envelope
 * once, and keep effects on the automatic voices. (Added 2026-09-21;
 * AUDIO_VOICE_AUTO had been defined since v2 with nothing to pass it to.)
 *
 * An explicit voice does not advance the round-robin counter.
 *
 * @param voice Voice 0-7, or AUDIO_VOICE_AUTO for the round-robin choice
 * @param sampleId Sample slot (0-63)
 * @param volume Volume level (0-127)
 * @param pan Pan position (0=left, 8=center, 15=right)
 * @param pitch Pitch value ($1000 = normal)
 * @return The voice used (0-7), or AUDIO_VOICE_NONE: voice out of range,
 *         sample not loaded, driver not ready
 *
 * @code
 * audioSetADSR(7, 4, 7, 7, 0);                 // slow attack, on voice 7
 * audioPlaySampleOn(7, SFX_ENGINE, 90, AUDIO_PAN_CENTER, 0x1000);
 * @endcode
 */
u8 audioPlaySampleOn(u8 voice, u8 sampleId, u8 volume, u8 pan, u16 pitch);

/**
 * @brief Stop a specific voice
 * @param voice Voice number (0-7)
 */
u8 audioStopVoice(u8 voice);

/**
 * @brief Stop all audio playback
 */
u8 audioStopAll(void);

/** @} */

/*============================================================================
 * Volume Control
 *============================================================================*/

/**
 * @defgroup audio_volume Volume Control
 * @{
 */

/**
 * @brief Set master volume
 * @param volume Volume level (0-127)
 */
u8 audioSetVolume(u8 volume);

/**
 * @brief Get current master volume
 * @return Current volume (0-127)
 */
u8 audioGetVolume(void);

/**
 * @brief Set volume for a specific voice
 * @param voice Voice number (0-7)
 * @param volumeL Left channel volume (0-127)
 * @param volumeR Right channel volume (0-127)
 */
u8 audioSetVoiceVolume(u8 voice, u8 volumeL, u8 volumeR);

/**
 * @brief Set pitch for a specific voice
 * @param voice Voice number (0-7)
 * @param pitch Pitch value ($1000 = normal)
 */
u8 audioSetVoicePitch(u8 voice, u16 pitch);

/**
 * @brief Set the volume and the pitch of a voice in one command
 *
 * What a tracker or a sequencer does to a voice at every tick: the
 * tremolo moves its volume and the vibrato its pitch. audioSetVoiceVolume()
 * then audioSetVoicePitch() cost two handshakes with the driver; this is
 * one, and the call itself is assembly (four voices updated every frame:
 * about 10 600 master cycles a voice with the two calls, 5 500 with this
 * one, measured on luna).
 *
 * The volume is split between the two channels by the voice's pan: the one
 * the last audioPlaySampleOn() / audioPlaySampleEx() gave it, or the one
 * audioSetVoicePan() set since — the same split as those calls make, so a
 * voice that plays hard left stays hard left. The driver learns a voice's
 * pan once, and again only when it changes.
 *
 * @param voice  Voice number (0-7)
 * @param volume Volume (0-127), before the pan
 * @param pitch  Pitch value ($1000 = normal, 14-bit)
 * @return AUDIO_OK, AUDIO_ERR_INVALID_ID, or AUDIO_ERR_TIMEOUT when the
 *         previous command was never acknowledged
 */
u8 audioSetVoiceVolumePitch(u8 voice, u8 volume, u16 pitch);

/**
 * @brief Set the pan audioSetVoiceVolumePitch() splits a voice's volume with
 *
 * Changes nothing that is sounding: it takes effect at the voice's next
 * audioSetVoiceVolumePitch(). A voice started with audioPlaySampleOn()
 * already has the pan of that call; this is for a voice whose side is
 * decided once (a tracker channel) or changes without a new note.
 *
 * @param voice Voice number (0-7)
 * @param pan   AUDIO_PAN_LEFT (0) to AUDIO_PAN_RIGHT (15), AUDIO_PAN_CENTER (8)
 */
void audioSetVoicePan(u8 voice, u8 pan);

/**
 * @brief Get current state of a voice
 * @param voice Voice number (0-7)
 * @param state Pointer to AudioVoiceState to fill
 */
void audioGetVoiceState(u8 voice, AudioVoiceState *state);

/** @} */

/*============================================================================
 * ADSR Envelope Control
 *============================================================================*/

/**
 * @defgroup audio_adsr ADSR Control
 * @{
 */

/**
 * @brief Set ADSR envelope for a voice
 *
 * @param voice Voice number (0-7)
 * @param attack Attack rate (0-15, higher = faster)
 * @param decay Decay rate (0-7, higher = faster)
 * @param sustain Sustain level (0-7, higher = louder)
 * @param release Release rate (0-31, higher = faster)
 */
u8 audioSetADSR(u8 voice, u8 attack, u8 decay, u8 sustain, u8 release);

/**
 * @brief Set GAIN mode for a voice (alternative to ADSR)
 * @param voice Voice number (0-7)
 * @param mode GAIN mode and value
 */
u8 audioSetGain(u8 voice, u8 mode);

/** @} */

/*============================================================================
 * Echo/Reverb Effects
 *============================================================================*/

/**
 * @defgroup audio_echo Echo Effects
 * @{
 */

/**
 * @brief Configure echo parameters
 *
 * @param delay Echo delay 1-7 (delay_ms = value * 16; values above 7
 *              are clamped). The echo ring takes delay x 2 KB at the top
 *              of the sample area: see audioGetFreeMemory()
 * @param feedback Feedback amount (-128 to 127)
 * @param volumeL Left echo volume (-128 to 127)
 * @param volumeR Right echo volume (-128 to 127)
 *
 * **This call blocks.** The S-DSP takes a new delay only when the ring it
 * is running reaches its end, so the driver keeps the ring silent for that
 * long before it uses the new one: up to 240 ms on the first call, 16 ms
 * per unit of the previous delay after; then it clears the new ring (~9 ms
 * per unit). Call it at scene setup or with the screen off, never during
 * play. To turn the same echo off and on, use audioDisableEcho() and
 * audioEnableEcho(): they cost one command.
 * Call this BEFORE audioEnableEcho().
 *
 * @return AUDIO_OK; AUDIO_ERR_NO_MEMORY when samples already loaded reach
 *         into the ring this delay needs (nothing is changed then);
 *         AUDIO_ERR_TIMEOUT if the driver does not answer
 */
u8 audioSetEcho(u8 delay, s8 feedback, s8 volumeL, s8 volumeR);

/**
 * @brief Set FIR filter coefficients for echo
 * @param fir Array of 8 signed coefficients
 *
 * Optional: from audioInit() the filter is { 127, 0, 0, 0, 0, 0, 0, 0 },
 * which passes the echo through unchanged. (Until 2026-10-13 it was all
 * zeros, and an echo configured without this call was silent.)
 */
u8 audioSetEchoFilter(const s8 fir[8]);

/**
 * @brief Enable echo for specific voices
 * @param voiceMask Bitmask (bit 0 = voice 0, etc.)
 *
 * After audioDisableEcho(), this starts the echo again as audioSetEcho()
 * configured it, on a cleared ring (~9 ms per unit of delay in the driver;
 * the call itself returns at once).
 */
u8 audioEnableEcho(u8 voiceMask);

/**
 * @brief Disable echo for all voices
 *
 * Mutes the echo and stops the ring's writes. The ring keeps its memory:
 * audioEnableEcho() brings the same echo back.
 */
u8 audioDisableEcho(void);

/**
 * @brief Turn the echo off and give its ring back to the samples
 *
 * Does what audioDisableEcho() does, then frees the ring: the sample area
 * reaches $FFC0 again (audioGetFreeMemory() counts it at once), and
 * samples may be loaded where the ring was as soon as this returns — the
 * driver answers only once the sound chip has stopped writing there.
 *
 * For a game that wants an echo in one scene only: audioSetEcho() when
 * the scene starts, this when it ends. audioEnableEcho() alone no longer
 * brings an echo back afterwards; audioSetEcho() does, with its usual
 * wait (16 ms per unit of the delay released), and it answers
 * AUDIO_ERR_NO_MEMORY while samples loaded in the meantime occupy the
 * ring it needs.
 *
 * @return AUDIO_OK, or AUDIO_ERR_TIMEOUT if the driver does not answer
 *         (the ring is then still counted as taken)
 */
u8 audioReleaseEcho(void);

/** @} */

/*============================================================================
 * Legacy API (backward compatibility)
 *============================================================================*/

/**
 * @defgroup audio_legacy Legacy API
 * @brief Functions for backward compatibility with older code
 * @{
 */

/** @brief Alias for audioStopAll */
#define audioStop() audioStopAll()

/** @brief Alias for audioStopVoice */
#define audioStopSample(id) audioStopVoice(id)

/** @brief Legacy pan conversion (0-127 to 0-15) */
#define audioSetPan(id, pan) /* No-op, use audioPlaySampleEx instead */

/** @} */

#endif /* OPENSNES_AUDIO_H */
