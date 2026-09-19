# Measured ROM coverage of the public lib API

luna v1.24.0 · `luna profile --pc-set` per ROM: the input-free idle path to the first capture frame, plus every `luna test` manifest's joypad-1 script to its last checkpoint · 86 ROMs (examples + the library fixture), 196 legs · **218 of 301 public functions executed, 83 never**

> Executed = at least one PC inside the function's `.sym` label range on at least one leg. Mouse and Super Scope scripts are not replayed (`luna profile` has no `--mouse` / `--superscope`), so those legs run input-free. The never-executed list is the ratchet in `baselines/never_executed.txt`.

| header | never executed |
|---|---|
| `audio.h` | `audioStopAll`, `audioStopVoice`, `audioUnloadSample`, `audioUpdate` |
| `background.h` | `bgGetScrollX`, `bgGetScrollY`, `bgInit`, `bgInitTileSetData`, `bgSetScrollX`, `bgSetScrollY` |
| `colormath.h` | `colorMathSetBrightness`, `colorMathSetChannel`, `colorMathSetCondition`, `colorMathTransparency50` |
| `console.h` | `consoleInitEx`, `resetFrameCount` |
| `debug.h` | `consoleMesenBreakpoint`, `consoleNocashMessage` |
| `dma.h` | `dmaCopyCGramBank`, `dmaCopyOam`, `dmaCopyVramBank`, `dmaTransfer` |
| `dsp1.h` | `dsp1Distance`, `dsp1Multiply`, `dsp1Range`, `dsp1Rotate`, `dsp1Target` |
| `hdma.h` | `hdmaGetEnabled`, `hdmaGradient`, `hdmaWaveH`, `hdmaWaveInit`, `hdmaWindowShape` |
| `input.h` | `mouseButtonsHeld`, `mouseButtonsPressed`, `mouseGetSensitivity`, `mouseGetX`, `mouseGetY`, `mouseIsConnected`, `mouseSetSensitivity`, `padRaw`, `scopeButtonsDown`, `scopeButtonsHeld`, `scopeButtonsPressed`, `scopeGetRawX`, `scopeGetRawY`, `scopeGetX`, `scopeGetY`, `scopeSetRepeatDelay`, `scopeSinceShot` |
| `interrupt.h` | `nmiSet` |
| `map.h` | `mapSetMapOptions` |
| `math.h` | `fixAbs`, `fixClamp`, `fixSqrt` |
| `mode7.h` | `mode7Rotate`, `mode7SetMatrix`, `mode7SetPivot`, `mode7Transform` |
| `mosaic.h` | `mosaicGetSize`, `mosaicSetSize` |
| `object.h` | `objCollidObj`, `objInitGravity`, `objRefreshAll` |
| `profile.h` | `profileColorEnd`, `profileColorStart`, `profileGetFrameCount`, `profileGetLagFrames`, `profileGetScanline`, `profileInit`, `profileScanlineEnd`, `profileScanlineStart` |
| `snesmod.h` | `snesmodAllocateSoundRegion`, `snesmodFlush`, `snesmodGetPosition`, `snesmodSetSoundTable` |
| `sprite.h` | `oamDrawMetaFlip`, `oamDynamicSetSize`, `oamSetTile` |
| `text.h` | `textFlush`, `textGetX` |
| `video.h` | `videoSetObjInterlace`, `videoSetOverscan`, `videoSetPseudoHires` |

## Least-covered executed functions (one ROM only)

| function | the one ROM |
|---|---|
| `LzssDecodeVram` | `backgrounds/mode1_lz77` |
| `animRestart` | `libtest` |
| `apuReset` | `audio/apu_switch` |
| `atan2_8` | `basics/aim_target` |
| `audioDisableEcho` | `audio/echo` |
| `audioGetFreeMemory` | `libtest` |
| `audioGetSampleInfo` | `libtest` |
| `audioGetVoiceState` | `libtest` |
| `audioGetVolume` | `libtest` |
| `audioIsReady` | `libtest` |
| `audioSetGain` | `libtest` |
| `audioSetVoicePitch` | `libtest` |
| `audioSetVoiceVolume` | `libtest` |
| `audioSetVolume` | `libtest` |
| `collidePoint` | `libtest` |
| `collideRectEx` | `libtest` |
| `collideRectTile` | `libtest` |
| `collideTileEx` | `libtest` |
| `colorMathSetDirectColor` | `color/direct_color` |
| `colorMathSetFixedColor` | `color/shadow_tint` |
| `colorMathShadow` | `color/shadow_tint` |
| `colorMathTint` | `color/shadow_tint` |
| `div16` | `libtest` |
| `dsp1Attitude` | `chips/dsp1_cube` |
| `dsp1Objective` | `chips/dsp1_cube` |
| `dsp1Project` | `chips/dsp1_cube` |
| `dsp1Raster` | `mode7/dsp1_ground` |
| `dsp1Triangle` | `mode7/dsp1_ground` |
| `fix32Div` | `libtest` |
| `fixDiv` | `libtest` |
| `fixLerp` | `libtest` |
| `getRegion` | `libtest` |
| `gsuDmaFullFrame` | `chips/superfx_3d` |
| `gsuLaunch` | `chips/superfx_3d` |
| `gsuSetupBitmapTilemap` | `chips/superfx_3d` |
| `gsuSetupHdmaBlanking` | `chips/superfx_3d` |
| `hdmaBrightnessGradient` | `hdma/hdma_helpers` |
| `hdmaBrightnessGradientStop` | `hdma/hdma_helpers` |
| `hdmaColorGradient` | `hdma/hdma_helpers` |
| `hdmaColorGradientStop` | `hdma/hdma_helpers` |
| `hdmaDisableAll` | `hdma/gradient_colors` |
| `hdmaIrisWipe` | `hdma/hdma_helpers` |
| `hdmaIrisWipeStop` | `hdma/hdma_helpers` |
| `hdmaParallax` | `scrolling/parallax_scroll` |
| `hdmaSetupBank` | `hdma/hdma_helpers` |
| `hdmaSetupIndirect` | `hdma/hdma_indirect_gradient` |
| `hdmaWaterRipple` | `hdma/hdma_helpers` |
| `hdmaWaveStop` | `hdma/hdma_helpers` |
| `hdmaWaveUpdate` | `hdma/hdma_helpers` |
| `irqClear` | `libtest` |
| `irqDisable` | `libtest` |
| `irqSet` | `libtest` |
| `irqSetVTimer` | `libtest` |
| `isInVBlank` | `libtest` |
| `isPAL` | `libtest` |
| `mapGetMetaTile` | `libtest` |
| `mapGetMetaTilesProp` | `libtest` |
| `mod16` | `libtest` |
| `mode7SetSettings` | `mode7/perspective_rotate` |
| `mosaicDisable` | `transitions/mosaic` |
| `mosaicEnable` | `transitions/mosaic` |
| `mosaicFadeIn` | `transitions/mosaic` |
| `mosaicFadeOut` | `transitions/mosaic` |
| `mouseInit` | `input/mouse` |
| `mul16` | `libtest` |
| `nmiClear` | `libtest` |
| `oamDynamicDrainQueue` | `sprites/dynamic_metasprite` |
| `oamHide` | `sprites/animated_sprite` |
| `oamMetaDrawDyn` | `sprites/dynamic_metasprite` |
| `oamSetX` | `input/move_sprite` |
| `oamSetXY` | `input/move_sprite` |
| `oamSetY` | `input/move_sprite` |
| `objCollidMap` | `games/mapandobjects` |
| `objCollidMap1D` | `libtest` |
| `objCollidMapWithSlopes` | `maps/slope_collision` |
| `objInitFriction1D` | `libtest` |
| `objKill` | `libtest` |
| `objKillAll` | `libtest` |
| `padIsConnected` | `libtest` |
| `padReleased` | `maps/dynamic_map` |
| `panelClear` | `basics/panel_hud` |
| `rectContains` | `libtest` |
| `rectGetCenter` | `libtest` |
| `rectInit` | `libtest` |
| `rectSetPos` | `libtest` |
| `sa1Init` | `chips/sa1_starfield` |
| `scenePop` | `basics/scene_stack` |
| `scenePush` | `basics/scene_stack` |
| `sceneRun` | `basics/scene_stack` |
| `scopeInit` | `input/superscope` |
| `scopeIsConnected` | `input/superscope` |
| `snesmodFadeVolume` | `audio/snesmod_music` |
| `snesmodPause` | `audio/snesmod_music` |
| `snesmodPlayEffect` | `audio/snesmod_sfx` |
| `snesmodResume` | `audio/snesmod_music` |
| `snesmodStop` | `audio/snesmod_music` |
| `sramChecksum` | `libtest` |
| `sramClear` | `libtest` |
| `sramLoad` | `libtest` |
| `sramSave` | `libtest` |
| `textGetY` | `libtest` |
| `videoSetInterlace` | `backgrounds/mode5_hires` |
| `windowCentered` | `libtest` |
| `windowDisable` | `libtest` |
| `windowDisableAll` | `libtest` |
| `windowInit` | `libtest` |
| `windowSetPos` | `libtest` |
| `windowSetSubMask` | `libtest` |
| `windowSplit` | `libtest` |
