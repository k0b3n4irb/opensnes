/**
 * @file audio.h
 * @brief OpenSNES Audio System
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
 * #include <snes.h>
 *
 * extern u8 beep_brr[];
 * extern u8 beep_brr_end[];
 *
 * int main(void) {
 *     audioInit();
 *     audioLoadSample(0, beep_brr, beep_brr_end - beep_brr, 0);
 *
 *     while (1) {
 *         WaitForVBlank();
 *         audioUpdate();
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

/** @brief Auto-allocate voice in audioPlaySampleEx */
#define AUDIO_VOICE_AUTO    0xFF

/** @brief Maximum volume value */
#define AUDIO_VOL_MAX       127
#define AUDIO_VOL_MIN       0

/** @brief Pan positions (0-15 scale) */
#define AUDIO_PAN_LEFT      0
#define AUDIO_PAN_CENTER    8
#define AUDIO_PAN_RIGHT     15

/** @brief Default pitch (1.0x playback rate) */
#define AUDIO_PITCH_DEFAULT 0x1000

/** @brief Common pitch values */
#define AUDIO_PITCH_C3      0x085F  /**< Middle C (261.63 Hz) */
#define AUDIO_PITCH_C4      0x10BE  /**< C4 (523.25 Hz) */
#define AUDIO_PITCH_C5      0x217C  /**< C5 (1046.5 Hz) */

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

/** @brief ADSR release rates (0=infinite, 31=instant) */
#define AUDIO_RELEASE_INSTANT   31
#define AUDIO_RELEASE_FAST      24
#define AUDIO_RELEASE_MEDIUM    16
#define AUDIO_RELEASE_SLOW      8

/** @brief Echo delay (delay_ms = value * 16ms) */
#define AUDIO_ECHO_DELAY_MIN    1   /**< 16ms */
#define AUDIO_ECHO_DELAY_MAX    15  /**< 240ms */

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
    u16 loopPoint;      /**< Loop offset (0 = no loop) */
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
 * Uploads the SPC700 driver and prepares for playback.
 * Call once during game initialization.
 *
 * @note Takes several frames to upload driver via IPL protocol.
 *       Interrupts are disabled during upload.
 */
void audioInit(void);

/**
 * @brief Check if audio system is ready
 * @return Non-zero if ready, 0 if still initializing
 */
u8 audioIsReady(void);

/**
 * @brief Process audio updates
 *
 * Call once per frame in your main loop.
 * Handles command queue processing and streaming.
 */
void audioUpdate(void);

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
 * @param loopPoint Loop offset in bytes, or 0 for no loop
 * @return AUDIO_OK on success, or error code
 *
 * @code
 * extern u8 explosion_brr[];
 * extern u8 explosion_brr_end[];
 * audioLoadSample(0, explosion_brr, explosion_brr_end - explosion_brr, 0);
 * @endcode
 */
u8 audioLoadSample(u8 id, const u8 *brrData, u16 size, u16 loopPoint);

/**
 * @brief Unload a sample from a slot
 * @param id Sample slot (0-63)
 *
 * Any voices playing this sample will be stopped.
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
 * @return Free bytes available
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
 * @brief Stop a specific voice
 * @param voice Voice number (0-7)
 */
void audioStopVoice(u8 voice);

/**
 * @brief Stop all audio playback
 */
void audioStopAll(void);

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
void audioSetVolume(u8 volume);

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
void audioSetVoiceVolume(u8 voice, u8 volumeL, u8 volumeR);

/**
 * @brief Set pitch for a specific voice
 * @param voice Voice number (0-7)
 * @param pitch Pitch value ($1000 = normal)
 */
void audioSetVoicePitch(u8 voice, u16 pitch);

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
void audioSetADSR(u8 voice, u8 attack, u8 decay, u8 sustain, u8 release);

/**
 * @brief Set GAIN mode for a voice (alternative to ADSR)
 * @param voice Voice number (0-7)
 * @param mode GAIN mode and value
 */
void audioSetGain(u8 voice, u8 mode);

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
 * @param delay Echo delay (1-15, delay_ms = value * 16)
 * @param feedback Feedback amount (-128 to 127)
 * @param volumeL Left echo volume (-128 to 127)
 * @param volumeR Right echo volume (-128 to 127)
 */
void audioSetEcho(u8 delay, s8 feedback, s8 volumeL, s8 volumeR);

/**
 * @brief Set FIR filter coefficients for echo
 * @param fir Array of 8 signed coefficients
 */
void audioSetEchoFilter(const s8 fir[8]);

/**
 * @brief Enable echo for specific voices
 * @param voiceMask Bitmask (bit 0 = voice 0, etc.)
 */
void audioEnableEcho(u8 voiceMask);

/**
 * @brief Disable echo for all voices
 */
void audioDisableEcho(void);

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
