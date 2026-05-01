/**
 * @file gameloop.c
 * @brief Implementation of gameLoopRun().
 *
 * Three lines of actual logic — but the work this saves a beginner is
 * the realisation that "an SNES program is a do-init-then-loop-on-NMI"
 * shape, not a "main does everything once and exits" shape. Putting
 * the loop behind a named entry point also gives us a place to grow
 * without contributors having to refactor every example main.
 *
 * Per `gameloop.h`'s documented behaviour, this function never
 * returns. The compiler currently has no `noreturn` attribute path
 * through cproc/QBE so we just document the contract; callers should
 * place a `return 0;` after the call to keep `int main(void)` happy
 * and silence "control reaches end of non-void function" warnings.
 */

#include <snes.h>
#include <snes/gameloop.h>

void gameLoopRun(const GameLoopConfig *cfg) {
    if (cfg->init)
        cfg->init();
    while (1) {
        WaitForVBlank();
        cfg->update();
    }
}
