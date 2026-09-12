# Measured ROM coverage of the public lib API

luna v1.21.0 · `luna profile --pc-set` to each example's first manifest frame, no input · 85 ROMs · **133 of 301 public functions executed, 168 never**

> Executed = at least one PC inside the function's `.sym` label range on at least one example (boot + idle path; input-driven code is under-counted). The never-executed list is the ratchet in `baselines/never_executed.txt`.

| header | never executed |
|---|---|
| `anim.h` | `animRestart` |
| `apu.h` | `apuReset` |
| `audio.h` | `audioDisableEcho`, `audioGetFreeMemory`, `audioGetSampleInfo`, `audioGetVoiceState`, `audioGetVolume`, `audioIsReady`, `audioSetGain`, `audioSetVoicePitch`, `audioSetVoiceVolume`, `audioSetVolume`, `audioStopAll`, `audioStopVoice`, `audioUnloadSample`, `audioUpdate` |
| `background.h` | `bgGetScrollX`, `bgGetScrollY`, `bgInit`, `bgInitTileSetData`, `bgSetScrollX`, `bgSetScrollY` |
| `collision.h` | `collidePoint`, `collideRectEx`, `collideRectTile`, `collideTileEx`, `rectContains`, `rectGetCenter`, `rectInit`, `rectSetPos` |
| `colormath.h` | `colorMathSetBrightness`, `colorMathSetChannel`, `colorMathSetCondition`, `colorMathSetFixedColor`, `colorMathShadow`, `colorMathTint`, `colorMathTransparency50` |
| `console.h` | `consoleInitEx`, `fadeIn`, `fadeOut`, `getRegion`, `isInVBlank`, `isPAL`, `resetFrameCount`, `srand` |
| `debug.h` | `consoleMesenBreakpoint`, `consoleNocashMessage` |
| `dma.h` | `dmaCopyCGramBank`, `dmaCopyOam`, `dmaCopyVramBank`, `dmaTransfer` |
| `dsp1.h` | `dsp1Distance`, `dsp1Multiply`, `dsp1Range`, `dsp1Rotate`, `dsp1Target`, `dsp1Triangle` |
| `fixed32.h` | `fix32Div` |
| `hdma.h` | `hdmaBrightnessGradient`, `hdmaBrightnessGradientStop`, `hdmaColorGradient`, `hdmaColorGradientStop`, `hdmaDisableAll`, `hdmaGetEnabled`, `hdmaGradient`, `hdmaIrisWipe`, `hdmaIrisWipeStop`, `hdmaWaterRipple`, `hdmaWaveH`, `hdmaWaveInit`, `hdmaWaveStop`, `hdmaWaveUpdate`, `hdmaWindowShape` |
| `input.h` | `mouseButtonsHeld`, `mouseButtonsPressed`, `mouseGetSensitivity`, `mouseGetX`, `mouseGetY`, `mouseIsConnected`, `mouseSetSensitivity`, `padIsConnected`, `padRaw`, `scopeButtonsDown`, `scopeButtonsHeld`, `scopeButtonsPressed`, `scopeGetRawX`, `scopeGetRawY`, `scopeGetX`, `scopeGetY`, `scopeSetRepeatDelay`, `scopeSinceShot` |
| `interrupt.h` | `irqClear`, `irqDisable`, `irqSet`, `irqSetVTimer`, `nmiClear`, `nmiSet`, `void` |
| `map.h` | `mapGetMetaTile`, `mapGetMetaTilesProp`, `mapSetMapOptions` |
| `math.h` | `div16`, `fixAbs`, `fixClamp`, `fixDiv`, `fixLerp`, `fixSqrt`, `mod16`, `mul16` |
| `mode7.h` | `mode7Rotate`, `mode7SetMatrix`, `mode7SetPivot`, `mode7Transform` |
| `mosaic.h` | `mosaicDisable`, `mosaicEnable`, `mosaicFadeIn`, `mosaicFadeOut`, `mosaicGetSize`, `mosaicSetSize` |
| `object.h` | `objCollidMap1D`, `objCollidObj`, `objInitFunctions`, `objInitGravity`, `objKill`, `objKillAll`, `objRefreshAll` |
| `panel.h` | `panelClear` |
| `profile.h` | `profileColorEnd`, `profileColorStart`, `profileGetFrameCount`, `profileGetLagFrames`, `profileGetScanline`, `profileInit`, `profileScanlineEnd`, `profileScanlineStart` |
| `scene.h` | `scenePop`, `scenePush` |
| `snesmod.h` | `snesmodAllocateSoundRegion`, `snesmodFadeVolume`, `snesmodFlush`, `snesmodGetPosition`, `snesmodPause`, `snesmodPlayEffect`, `snesmodResume`, `snesmodSetSoundTable`, `snesmodStop` |
| `sprite.h` | `oamDrawMetaFlip`, `oamDynamicSetSize`, `oamSetTile` |
| `sram.h` | `sramChecksum`, `sramClear`, `sramLoad`, `sramLoadOffset`, `sramSave`, `sramSaveOffset` |
| `text.h` | `textFlush`, `textGetX`, `textGetY` |
| `video.h` | `videoSetObjInterlace`, `videoSetOverscan`, `videoSetPseudoHires` |
| `window.h` | `windowCentered`, `windowDisable`, `windowDisableAll`, `windowInit`, `windowSetPos`, `windowSetSubMask`, `windowSplit` |

## Least-covered executed functions (one example only)

| function | the one example |
|---|---|
| `LzssDecodeVram` | `backgrounds/mode1_lz77` |
| `atan2_8` | `basics/aim_target` |
| `audioEnableEcho` | `audio/echo` |
| `audioPlaySample` | `audio/echo` |
| `audioPlaySampleEx` | `audio/echo` |
| `audioSetADSR` | `audio/soundboard` |
| `audioSetEcho` | `audio/echo` |
| `audioSetEchoFilter` | `audio/echo` |
| `collideRect` | `basics/collision_demo` |
| `collideTile` | `basics/collision_demo` |
| `colorMathSetDirectColor` | `color/direct_color` |
| `colorMathSetHalf` | `color/hicolor_blend` |
| `dsp1Attitude` | `chips/dsp1_cube` |
| `dsp1Objective` | `chips/dsp1_cube` |
| `dsp1Project` | `chips/dsp1_cube` |
| `dsp1Raster` | `mode7/dsp1_ground` |
| `fix32Mul` | `basics/fix32_orbit` |
| `gsuDmaFullFrame` | `chips/superfx_3d` |
| `gsuLaunch` | `chips/superfx_3d` |
| `gsuSetupBitmapTilemap` | `chips/superfx_3d` |
| `gsuSetupHdmaBlanking` | `chips/superfx_3d` |
| `hdmaParallax` | `scrolling/parallax_scroll` |
| `hdmaSetTable` | `mode7/dsp1_ground` |
| `hdmaSetupBank` | `hdma/hdma_helpers` |
| `hdmaSetupIndirect` | `hdma/hdma_indirect_gradient` |
| `irqEnable` | `color/hicolor_1792` |
| `irqSetBank` | `color/hicolor_1792` |
| `irqSetHTimer` | `color/hicolor_1792` |
| `mode7SetSettings` | `mode7/perspective_rotate` |
| `mouseInit` | `input/mouse` |
| `nmiSetBank` | `color/hicolor_1792` |
| `oamDynamicDrainQueue` | `sprites/dynamic_metasprite` |
| `oamHide` | `sprites/animated_sprite` |
| `oamMetaDrawDyn` | `sprites/dynamic_metasprite` |
| `oamSetX` | `input/move_sprite` |
| `oamSetXY` | `input/move_sprite` |
| `oamSetY` | `input/move_sprite` |
| `objCollidMap` | `games/mapandobjects` |
| `objCollidMapWithSlopes` | `maps/slope_collision` |
| `padReleased` | `maps/dynamic_map` |
| `rand` | `basics/random` |
| `sa1Init` | `chips/sa1_starfield` |
| `sceneRun` | `basics/scene_stack` |
| `scopeInit` | `input/superscope` |
| `scopeIsConnected` | `input/superscope` |
| `setBrightness` | `games/rpg` |
| `snesmodSetModuleVolume` | `games/likemario` |
| `sqrt16` | `basics/aim_target` |
| `videoSetInterlace` | `backgrounds/mode5_hires` |
| `windowEnable` | `windows/window_multi_hdma` |
| `windowSetInvert` | `windows/window_multi_hdma` |
| `windowSetLogic` | `windows/window_multi_hdma` |
| `windowSetMainMask` | `windows/window_multi_hdma` |
