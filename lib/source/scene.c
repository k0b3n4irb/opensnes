/**
 * @file scene.c
 * @brief Implementation of sceneRun() / scenePush() / scenePop() —
 *        see scene.h for the contract.
 *
 * The state is a single static array of `Scene *` plus a depth byte.
 * Push and pop are array-index updates; dispatch is one indirect call
 * per frame. No allocation, no NMI hook, no global state outside this
 * file.
 */

#include <snes.h>
#include <snes/scene.h>

static const Scene *scene_stack[SCENE_STACK_MAX];
static u8 scene_top;

void sceneRun(const Scene *initial) {
    scene_stack[0] = initial;
    scene_top = 1;
    if (initial->init)
        initial->init();
    while (1) {
        WaitForVBlank();
        scene_stack[scene_top - 1]->update();
    }
}

void scenePush(const Scene *next) {
    if (scene_top >= SCENE_STACK_MAX)
        return;
    scene_stack[scene_top] = next;
    scene_top++;
    if (next->init)
        next->init();
}

void scenePop(void) {
    if (scene_top <= 1)
        return;
    scene_top--;
}
