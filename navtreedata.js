/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "OpenSNES", "index.html", [
    [ "OpenSNES Documentation", "index.html", "index" ],
    [ "Audio & Music Tutorial", "tutorial_audio.html", [
      [ "SNES Audio Architecture", "tutorial_audio.html#autotoc_md75", null ],
      [ "Audio Options in OpenSNES", "tutorial_audio.html#autotoc_md76", null ],
      [ "SNESMOD (Recommended)", "tutorial_audio.html#autotoc_md77", [
        [ "Setup", "tutorial_audio.html#autotoc_md78", null ],
        [ "Makefile Configuration", "tutorial_audio.html#autotoc_md79", null ],
        [ "Basic Music Playback", "tutorial_audio.html#autotoc_md80", null ],
        [ "Sound Effects", "tutorial_audio.html#autotoc_md81", null ],
        [ "Volume Control", "tutorial_audio.html#autotoc_md82", null ],
        [ "Creating Music with OpenMPT", "tutorial_audio.html#autotoc_md83", null ],
        [ "smconv Tool", "tutorial_audio.html#autotoc_md84", null ]
      ] ],
      [ "Pitch Constants", "tutorial_audio.html#autotoc_md85", null ],
      [ "Important: Call snesmodProcess()!", "tutorial_audio.html#autotoc_md86", null ],
      [ "Example: Music + SFX", "tutorial_audio.html#autotoc_md87", null ],
      [ "Memory Usage", "tutorial_audio.html#autotoc_md88", null ],
      [ "Tips", "tutorial_audio.html#autotoc_md89", null ],
      [ "Examples", "tutorial_audio.html#autotoc_md90", null ],
      [ "Next Steps", "tutorial_audio.html#autotoc_md91", null ]
    ] ],
    [ "Graphics & Backgrounds Tutorial", "tutorial_graphics.html", [
      [ "SNES Video Modes", "tutorial_graphics.html#autotoc_md92", null ],
      [ "Setting Up Graphics", "tutorial_graphics.html#autotoc_md93", [
        [ "Initialize Display", "tutorial_graphics.html#autotoc_md94", null ]
      ] ],
      [ "Loading Tiles to VRAM", "tutorial_graphics.html#autotoc_md95", null ],
      [ "Setting Up Tilemaps", "tutorial_graphics.html#autotoc_md96", null ],
      [ "Setting Palettes", "tutorial_graphics.html#autotoc_md97", null ],
      [ "Background Scrolling", "tutorial_graphics.html#autotoc_md98", null ],
      [ "Parallax Scrolling", "tutorial_graphics.html#autotoc_md99", null ],
      [ "Using gfx4snes", "tutorial_graphics.html#autotoc_md100", null ],
      [ "Example: Complete Background Setup", "tutorial_graphics.html#autotoc_md101", null ],
      [ "Next Steps", "tutorial_graphics.html#autotoc_md102", null ]
    ] ],
    [ "Controller Input Tutorial", "tutorial_input.html", [
      [ "SNES Controller Layout", "tutorial_input.html#autotoc_md103", null ],
      [ "Button Constants", "tutorial_input.html#autotoc_md104", null ],
      [ "Reading Controllers", "tutorial_input.html#autotoc_md105", [
        [ "Direct Register Access (Recommended)", "tutorial_input.html#autotoc_md106", null ],
        [ "Reading Controller 2", "tutorial_input.html#autotoc_md107", null ]
      ] ],
      [ "Edge Detection", "tutorial_input.html#autotoc_md108", null ],
      [ "Movement Example", "tutorial_input.html#autotoc_md109", null ],
      [ "Two-Player Example", "tutorial_input.html#autotoc_md110", null ],
      [ "Button Combinations", "tutorial_input.html#autotoc_md111", null ],
      [ "Menu Navigation", "tutorial_input.html#autotoc_md112", null ],
      [ "Hardware Registers", "tutorial_input.html#autotoc_md113", null ],
      [ "Tips", "tutorial_input.html#autotoc_md114", null ],
      [ "Example", "tutorial_input.html#autotoc_md115", null ],
      [ "Next Steps", "tutorial_input.html#autotoc_md116", null ]
    ] ],
    [ "Sprites & Animation Tutorial", "tutorial_sprites.html", [
      [ "SNES Sprite Basics", "tutorial_sprites.html#autotoc_md117", null ],
      [ "OAM Structure", "tutorial_sprites.html#autotoc_md118", null ],
      [ "Using OpenSNES Sprite Functions", "tutorial_sprites.html#autotoc_md119", [
        [ "Initialize OAM", "tutorial_sprites.html#autotoc_md120", null ],
        [ "Setting a Sprite", "tutorial_sprites.html#autotoc_md121", null ],
        [ "Updating OAM", "tutorial_sprites.html#autotoc_md122", null ],
        [ "Hiding Sprites", "tutorial_sprites.html#autotoc_md123", null ]
      ] ],
      [ "Loading Sprite Tiles", "tutorial_sprites.html#autotoc_md124", null ],
      [ "Sprite Palettes", "tutorial_sprites.html#autotoc_md125", null ],
      [ "Animation", "tutorial_sprites.html#autotoc_md126", [
        [ "Frame-based Animation", "tutorial_sprites.html#autotoc_md127", null ],
        [ "Movement with Animation", "tutorial_sprites.html#autotoc_md128", null ]
      ] ],
      [ "Sprite Sizes", "tutorial_sprites.html#autotoc_md129", null ],
      [ "Example: Two Players", "tutorial_sprites.html#autotoc_md130", null ],
      [ "Performance Tips", "tutorial_sprites.html#autotoc_md131", null ],
      [ "Next Steps", "tutorial_sprites.html#autotoc_md132", null ]
    ] ],
    [ "Getting Started with OpenSNES", "md_GETTING__STARTED.html", [
      [ "What You'll Need", "md_GETTING__STARTED.html#autotoc_md134", null ],
      [ "Step 1: Install Prerequisites", "md_GETTING__STARTED.html#autotoc_md135", [
        [ "macOS", "md_GETTING__STARTED.html#autotoc_md136", null ],
        [ "Linux (Ubuntu/Debian)", "md_GETTING__STARTED.html#autotoc_md137", null ],
        [ "Linux (Fedora)", "md_GETTING__STARTED.html#autotoc_md138", null ],
        [ "Windows", "md_GETTING__STARTED.html#autotoc_md139", null ]
      ] ],
      [ "Step 2: Get an Emulator", "md_GETTING__STARTED.html#autotoc_md140", null ],
      [ "Step 3: Clone and Build OpenSNES", "md_GETTING__STARTED.html#autotoc_md141", null ],
      [ "Step 4: Run Your First ROM", "md_GETTING__STARTED.html#autotoc_md142", null ],
      [ "Step 5: Create Your Own Project", "md_GETTING__STARTED.html#autotoc_md143", [
        [ "Minimal Project Structure", "md_GETTING__STARTED.html#autotoc_md144", null ],
        [ "Minimal Makefile", "md_GETTING__STARTED.html#autotoc_md145", null ],
        [ "Minimal main.c", "md_GETTING__STARTED.html#autotoc_md146", null ]
      ] ],
      [ "What's Next?", "md_GETTING__STARTED.html#autotoc_md147", null ],
      [ "Troubleshooting", "md_GETTING__STARTED.html#autotoc_md148", [
        [ "\"command not found: make\"", "md_GETTING__STARTED.html#autotoc_md149", null ],
        [ "\"fatal: repository not found\" or empty compiler folder", "md_GETTING__STARTED.html#autotoc_md150", null ],
        [ "\"OPENSNES_HOME not set\" or \"No rule to make target\"", "md_GETTING__STARTED.html#autotoc_md151", null ],
        [ "Black screen when running ROM", "md_GETTING__STARTED.html#autotoc_md152", null ],
        [ "Build fails with \"unhandled op\" or assembly errors", "md_GETTING__STARTED.html#autotoc_md153", null ]
      ] ],
      [ "Getting Help", "md_GETTING__STARTED.html#autotoc_md154", null ]
    ] ],
    [ "OpenSNES Example Walkthroughs", "md_EXAMPLE__WALKTHROUGHS.html", [
      [ "Table of Contents", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md157", null ],
      [ "1. Hello World", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md159", [
        [ "Key Concepts", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md160", null ],
        [ "Code Walkthrough", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md161", [
          [ "Hardware Register Setup", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md162", null ],
          [ "Loading Tile Graphics", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md163", null ],
          [ "Setting the Palette", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md164", null ],
          [ "Writing the Tilemap", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md165", null ],
          [ "Enabling Display", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md166", null ]
        ] ],
        [ "Try It Yourself", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md167", null ]
      ] ],
      [ "2. Sprite Display", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md169", [
        [ "Key Concepts", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md170", null ],
        [ "OAM Structure", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md171", null ],
        [ "Code Walkthrough", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md172", [
          [ "Sprite Size Configuration", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md173", null ],
          [ "Loading Sprite Graphics", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md174", null ],
          [ "Loading Sprite Palette", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md175", null ],
          [ "Setting Up OAM", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md176", null ]
        ] ],
        [ "Try It Yourself", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md177", null ]
      ] ],
      [ "3. Two-Player Input", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md179", [
        [ "Key Concepts", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md180", null ],
        [ "Controller Button Layout", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md181", null ],
        [ "Code Walkthrough", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md182", [
          [ "Player State", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md183", null ],
          [ "Sprite Setup (Two Palettes)", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md184", null ],
          [ "Reading Both Controllers", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md185", null ],
          [ "Independent Movement with Boundary Checking", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md186", null ]
        ] ],
        [ "Try It Yourself", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md187", null ]
      ] ],
      [ "4. SNESMOD Music", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md189", [
        [ "Key Concepts", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md190", null ],
        [ "Makefile Configuration", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md191", null ],
        [ "Code Walkthrough", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md192", [
          [ "Initialization", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md193", null ],
          [ "Per-Frame Processing (Critical!)", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md194", null ],
          [ "Transport Controls", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md195", null ],
          [ "Volume Control", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md196", null ]
        ] ],
        [ "Creating Music", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md197", null ],
        [ "Try It Yourself", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md198", null ],
        [ "Deep Dive: What SNESMOD Does Under the Hood", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md199", null ]
      ] ],
      [ "Common Patterns", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md201", [
        [ "VBlank Synchronization", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md202", null ],
        [ "Main Loop Structure", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md203", null ],
        [ "Hiding Unused Sprites", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md204", null ]
      ] ],
      [ "Building Examples", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md206", null ],
      [ "Next Steps", "md_EXAMPLE__WALKTHROUGHS.html#autotoc_md207", null ]
    ] ],
    [ "OpenSNES Code Style Guide", "md_CODE__STYLE.html", [
      [ "C Code", "md_CODE__STYLE.html#autotoc_md209", [
        [ "General", "md_CODE__STYLE.html#autotoc_md210", null ],
        [ "Naming Conventions", "md_CODE__STYLE.html#autotoc_md211", null ],
        [ "Types", "md_CODE__STYLE.html#autotoc_md212", null ],
        [ "Function Documentation", "md_CODE__STYLE.html#autotoc_md213", null ],
        [ "Error Handling", "md_CODE__STYLE.html#autotoc_md214", null ],
        [ "Memory", "md_CODE__STYLE.html#autotoc_md215", null ]
      ] ],
      [ "Assembly (65816 - WLA-DX)", "md_CODE__STYLE.html#autotoc_md216", [
        [ "General", "md_CODE__STYLE.html#autotoc_md217", null ],
        [ "Format", "md_CODE__STYLE.html#autotoc_md218", null ],
        [ "Register Documentation", "md_CODE__STYLE.html#autotoc_md219", null ],
        [ "Labels", "md_CODE__STYLE.html#autotoc_md220", null ]
      ] ],
      [ "File Organization", "md_CODE__STYLE.html#autotoc_md221", [
        [ "Headers (.h)", "md_CODE__STYLE.html#autotoc_md222", null ],
        [ "Source (.c)", "md_CODE__STYLE.html#autotoc_md223", null ]
      ] ],
      [ "Formatting Tools", "md_CODE__STYLE.html#autotoc_md224", null ]
    ] ],
    [ "Contributing to OpenSNES", "md__2home_2runner_2work_2opensnes_2opensnes_2CONTRIBUTING.html", [
      [ "Code of Conduct", "md__2home_2runner_2work_2opensnes_2opensnes_2CONTRIBUTING.html#autotoc_md226", null ],
      [ "How to Contribute", "md__2home_2runner_2work_2opensnes_2opensnes_2CONTRIBUTING.html#autotoc_md227", [
        [ "Reporting Bugs", "md__2home_2runner_2work_2opensnes_2opensnes_2CONTRIBUTING.html#autotoc_md228", null ],
        [ "Suggesting Features", "md__2home_2runner_2work_2opensnes_2opensnes_2CONTRIBUTING.html#autotoc_md229", null ],
        [ "Submitting Code", "md__2home_2runner_2work_2opensnes_2opensnes_2CONTRIBUTING.html#autotoc_md230", [
          [ "Setup", "md__2home_2runner_2work_2opensnes_2opensnes_2CONTRIBUTING.html#autotoc_md231", null ],
          [ "Development Workflow", "md__2home_2runner_2work_2opensnes_2opensnes_2CONTRIBUTING.html#autotoc_md232", null ],
          [ "Commit Message Format", "md__2home_2runner_2work_2opensnes_2opensnes_2CONTRIBUTING.html#autotoc_md233", null ],
          [ "Pull Request Checklist", "md__2home_2runner_2work_2opensnes_2opensnes_2CONTRIBUTING.html#autotoc_md234", null ],
          [ "Code Review", "md__2home_2runner_2work_2opensnes_2opensnes_2CONTRIBUTING.html#autotoc_md235", null ]
        ] ]
      ] ],
      [ "Development Guidelines", "md__2home_2runner_2work_2opensnes_2opensnes_2CONTRIBUTING.html#autotoc_md236", [
        [ "Documentation Requirements", "md__2home_2runner_2work_2opensnes_2opensnes_2CONTRIBUTING.html#autotoc_md237", null ],
        [ "Test Requirements", "md__2home_2runner_2work_2opensnes_2opensnes_2CONTRIBUTING.html#autotoc_md238", null ],
        [ "Attribution Requirements", "md__2home_2runner_2work_2opensnes_2opensnes_2CONTRIBUTING.html#autotoc_md239", null ]
      ] ],
      [ "Questions?", "md__2home_2runner_2work_2opensnes_2opensnes_2CONTRIBUTING.html#autotoc_md240", null ]
    ] ],
    [ "Deprecated List", "deprecated.html", null ],
    [ "Todo List", "todo.html", null ],
    [ "Topics", "topics.html", "topics" ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", null ],
        [ "Variables", "functions_vars.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "File Members", "globals.html", [
        [ "All", "globals.html", "globals_dup" ],
        [ "Functions", "globals_func.html", "globals_func" ],
        [ "Variables", "globals_vars.html", null ],
        [ "Typedefs", "globals_type.html", null ],
        [ "Macros", "globals_defs.html", "globals_defs" ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"animation_8h.html",
"group__audio__const.html#gab0fbec89961f7279fac65054ec8762fc",
"group__scope__input.html#ga70b379ab162b1146fc9cd87e990bab5d",
"md_GETTING__STARTED.html#autotoc_md139",
"structt__objs.html#a2027380a15444371412d5266bc3d2b39"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';