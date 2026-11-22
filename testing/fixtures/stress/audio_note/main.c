/*
 * A note in two handshakes (stress fixture, 2026-11-19).
 *
 * audioPlaySampleOn() used to send up to four commands: volume, pitch,
 * envelope, key on. When volume and pitch both change it now sends one
 * VSET (audioSetVoiceVolumePitch(): the volume split by the driver's pan
 * weight, and the pitch) then the key on: two commands, where it was three.
 *
 * The commands a call posts are counted by the sequence bits of the port
 * (audio_seq cycles 1, 2, 3): the count is the difference mod 3, so two
 * is read 2, three 0 and one 1. Cases on one voice, envelope already set:
 *   A fresh voice, centre          VSET, KON          2
 *   B volume and pitch change      VSET, KON          2
 *   C same note again              KON                1
 *   D pitch only                   VPITCH, KON        2
 *   E volume only                  VVOL, KON          2
 *   F pan changes with both        VPAN, VSET, KON    3 (reads 0)
 *   G an inner pan, then repeated  (VPAN,VSET,KON) then KON   0 then 1
 * Read by audio_note.toml. FAULT is not needed: on the old code A and B
 * read 0, not 2.
 */
#include <snes.h>
#include <snes/audio.h>

#define SMP 0
#define V   3

extern u8 audio_seq;

static const u8 loop_brr[9] = { 0xB3, 0x77, 0x77, 0x77, 0x77, 0x99, 0x99, 0x99, 0x99 };

u16 r_a, r_b, r_c, r_d, r_e, r_f, r_g1, r_g2;
u16 r_bad;                  /* a play that did not return its voice -> 0 */
u16 r_done;

static u16 note(u8 vol, u8 pan, u16 pitch)
{
    u8 s0 = audio_seq;
    u8 s1;
    u16 d;
    if (audioPlaySampleOn(V, SMP, vol, pan, pitch) != V) {
        r_bad++;
    }
    s1 = audio_seq;
    d = (u16)(s1 + 3 - s0);
    return d % 3;
}

int main(void)
{
    consoleInit();
    setScreenOn();
    audioInit();
    audioLoadSample(SMP, loop_brr, sizeof(loop_brr), 0);
    audioSetADSR(V, 15, 7, 7, 0);       /* the envelope is the caller's */
    WaitForVBlank();

    r_a = note(100, AUDIO_PAN_CENTER, 0x1000);
    r_b = note(80, AUDIO_PAN_CENTER, 0x1400);
    r_c = note(80, AUDIO_PAN_CENTER, 0x1400);
    r_d = note(80, AUDIO_PAN_CENTER, 0x1800);
    r_e = note(60, AUDIO_PAN_CENTER, 0x1800);
    r_f = note(50, AUDIO_PAN_LEFT, 0x1000);
    r_g1 = note(70, 3, 0x1200);
    r_g2 = note(70, 3, 0x1200);

    r_done = 0xC0DE;
    while (1) {
        WaitForVBlank();
    }
    return 0;
}
