/**
 * @file anim.h
 * @brief Declarative animation player — data-driven frame sequencing
 *
 * Replaces the hand-rolled tempo counters every game rewrites (a tick
 * variable, a modulo gate, manual oamframeid/oamrefresh pokes) with two
 * small structs and one explicit per-frame call.
 *
 * A **frame value is an opaque u16** — the player sequences it without
 * interpreting it. Choose what it means per clip:
 *   - an `oamframeid` for the dynamic sprite engine (see animTickOam()),
 *   - an index into a gfx4snes `-T` metasprite pointer table
 *     (see animTickMeta()),
 *   - a raw OAM tile number for oamSetTile().
 *
 * Zero global state, zero init call, zero link dependencies: all state
 * lives in caller-owned AnimPlayer structs, and nothing runs unless you
 * tick. Pausing an animation = not ticking it (costs nothing).
 *
 * @par Layout contract (tooling-facing)
 * AnimClip (12 bytes: two 4-byte far pointers + 4 u8 fields) and
 * AnimPlayer (8 bytes: one far pointer + 4 u8 fields) layouts below are a
 * public contract: editors (Cooper) emit AnimClip as generated C data.
 * Field order, sizes and semantics are stable; `reserved` fields must
 * be 0.
 *
 * @par Cost (per PHILOSOPHY.md P5)
 * animTick() on the non-advancing path is a NULL check, a finished
 * check, a decrement/compare and one indexed u16 load — on the order of
 * 60–90 cycles per player per frame including call overhead. RAM cost is
 * 8 bytes per player; ROM cost 12 bytes per clip + 2 bytes per frame
 * (+1/frame if per-frame durations are used). Nothing runs in NMI; VRAM
 * upload only happens when a frame actually changes (the dynamic
 * engine's existing budget).
 *
 * @warning Clips declared with DECLARE_ANIM_CLIP are ROM const data and
 * count against the bank $00 budget (see KNOWN_LIMITATIONS.md): if bank
 * $00 is full they spill to bank $01+ and are read as garbage. The build
 * hard-fails when that happens (symmap ratchet). Mitigation for a
 * nearly-full ROM: declare the clip and its frames array as initialized
 * non-const statics — RAM-backed clips have the identical layout and API
 * (see examples/games/likemario).
 *
 * Requires 'anim' in LIB_MODULES.
 *
 * License: CC0 (Public Domain)
 */

#ifndef OPENSNES_ANIM_H
#define OPENSNES_ANIM_H

#include <snes/types.h>

/** @brief Loop mode: wrap to frame 0 after the last frame. */
#define ANIM_LOOP  0
/** @brief Loop mode: hold the last frame and raise ANIM_F_FINISHED. */
#define ANIM_ONCE  1

/** @brief AnimPlayer.flags bit: an ANIM_ONCE clip reached its last frame. */
#define ANIM_F_FINISHED  0x01

/** @brief animTick()/animFrame() result when the player has no clip. */
#define ANIM_NONE  0xFFFF

/**
 * @brief An animation clip — ROM-resident, 12 bytes.
 *
 * Declare with DECLARE_ANIM_CLIP() for the uniform-speed case, or as a
 * raw struct when per-frame durations are needed:
 * @code
 * static const u16 boom_frames[]    = { 4, 5, 6, 7 };
 * static const u8  boom_durations[] = { 2, 2, 3, 6 };
 * static const AnimClip boom =
 *     { boom_frames, boom_durations, 4, 0, ANIM_ONCE, 0 };
 * @endcode
 */
typedef struct {
    const u16 *frames;   /**< frame values, len entries (never NULL) */
    const u8 *durations; /**< per-frame ticks, or NULL = uniform `speed` */
    u8 len;              /**< frame count, >= 1 */
    u8 speed;            /**< ticks per frame when durations==NULL (0 acts as 1) */
    u8 mode;             /**< ANIM_LOOP or ANIM_ONCE */
    u8 reserved;         /**< must be 0 (layout stability) */
} AnimClip;

/**
 * @brief A playback head — RAM, 8 bytes, array-friendly.
 *
 * Zero-initialised (or ANIM_PLAYER_INIT) = stopped. One per animated
 * entity; e.g. `static AnimPlayer obj_anim[OB_MAX];` next to the object
 * engine's slots.
 */
typedef struct {
    const AnimClip *clip; /**< current clip, NULL = stopped */
    u8 frame;             /**< cursor into clip->frames */
    u8 ticks;             /**< countdown; the frame advances when it hits 0 */
    u8 flags;             /**< ANIM_F_* */
    u8 reserved;          /**< pad to even size */
} AnimPlayer;

/** @brief Static initializer: `AnimPlayer p = ANIM_PLAYER_INIT;` */
#define ANIM_PLAYER_INIT  {0, 0, 0, 0, 0}

/**
 * @brief Start (or keep) a clip — continue-if-same semantics.
 *
 * Safe to call unconditionally every frame from a state machine:
 * - same clip, still playing: no-op (the animation continues);
 * - same clip, finished (ANIM_ONCE): restarts from frame 0 (re-trigger);
 * - different clip: switches immediately — frame 0, ticks reloaded,
 *   flags cleared;
 * - NULL clip (or len == 0): equivalent to animStop().
 *
 * @param p    Player (caller-owned)
 * @param clip ROM clip to play
 */
void animPlay(AnimPlayer *p, const AnimClip *clip);

/**
 * @brief Force-restart the current clip from frame 0, even mid-flight.
 */
void animRestart(AnimPlayer *p);

/**
 * @brief Advance one tick; return the current frame value.
 *
 * Call once per game-loop iteration per active player (after
 * WaitForVBlank(), like the rest of the frame logic).
 *
 * @return The current frame value; ANIM_NONE if the player is stopped.
 *         A finished ANIM_ONCE player keeps returning its last frame.
 */
u16 animTick(AnimPlayer *p);

/** @brief Current frame value WITHOUT advancing (ANIM_NONE if stopped). */
#define animFrame(_p)  ((_p)->clip ? (_p)->clip->frames[(_p)->frame] : ANIM_NONE)

/** @brief Nonzero once an ANIM_ONCE clip holds its last frame. */
#define animDone(_p)   ((_p)->flags & ANIM_F_FINISHED)

/** @brief Stop: detach the clip, clear flags. Next animPlay starts fresh. */
#define animStop(_p)   do { (_p)->clip = 0; (_p)->flags = 0; } while (0)

/**
 * @brief Tick + apply to the dynamic sprite engine (the 90% one-liner).
 *
 * Writes oambuffer[_id].oamframeid and sets oamrefresh = 1 ONLY when the
 * frame actually changed — the VRAM re-upload is the expensive part, so
 * a 1-frame clip (stand pose) costs nothing after the first apply.
 *
 * Macro (not a function) so the anim module carries no link dependency
 * on the dynamic engine: the oambuffer reference lands in YOUR translation
 * unit, which already links sprite/sprite_dynamic.
 */
#define animTickOam(_p, _id) do { \
    u16 _af = animTick(_p); \
    if (_af != ANIM_NONE && oambuffer[_id].oamframeid != _af) { \
        oambuffer[_id].oamframeid = _af; \
        oambuffer[_id].oamrefresh = 1; \
    } \
} while (0)

/**
 * @brief Tick + resolve against a gfx4snes metasprite pointer table.
 *
 * Feeds oamDrawMeta()/oamMetaDrawDyn() directly:
 * @code
 * oamDrawMeta(0, x, y, animTickMeta(&hero, hero_metasprites), 0, 0, sz);
 * @endcode
 *
 * @warning The player must be running (frame values index _tbl); a
 * stopped player would index with ANIM_NONE. Guard with
 * `animFrame(p) != ANIM_NONE` if the clip can be absent — the hot path
 * stays branch-free by contract.
 */
#define animTickMeta(_p, _tbl)  ((_tbl)[animTick(_p)])

/**
 * @brief Declare a static const AnimClip with uniform frame duration.
 *
 * @code
 * DECLARE_ANIM_CLIP(mario_walk, ANIM_LOOP, 4, FRAME_WALK0, FRAME_WALK1);
 * animPlay(&mario_anim, &mario_walk);
 * @endcode
 *
 * Per-frame durations can't be expressed variadically — declare those
 * clips as raw structs (see the AnimClip example above).
 */
#define DECLARE_ANIM_CLIP(name, mode_, speed_, ...) \
    static const u16 name##_frames[] = { __VA_ARGS__ }; \
    static const AnimClip name = { \
        name##_frames, \
        0, \
        (u8)(sizeof(name##_frames) / sizeof(u16)), \
        (speed_), \
        (mode_), \
        0, \
    }

#endif /* OPENSNES_ANIM_H */
