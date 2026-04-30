---
name: new-example
description: Create a new SNES example project from template with Makefile and main.c
argument-hint: "<category>/<name> (e.g., graphics/mode7)"
allowed-tools: Bash(*), Read, Write, Edit
---

# /new-example - Create New Example Project

Create a new example project from template.

## Usage
```
/new-example <category>/<name>    # Create new example
/new-example audioecho         # Example: audio category, echo example
/new-example graphics/mode7       # Example: graphics category, mode7 example
```

## Categories
- `audio` - Sound examples
- `basics` - Simple demonstrations
- `game` - Game examples
- `graphics` - Video/PPU examples
- `input` - Input/controller examples
- `text` - Text output examples

## Implementation

1. Create directory: `examples/<category>/<name>/`

2. Create files from template:

**Makefile:**
```makefile
OPENSNES := $(shell cd ../../.. && pwd)
TARGET   := <name>.sfc
ROM_NAME := "OPENSNES <NAME>      "  # 21 chars, pad with spaces

CSRC     := main.c
USE_LIB  := 1
LIB_MODULES := console

include $(OPENSNES)/make/common.mk
```

**main.c:**
```c
/**
 * @file main.c
 * @brief <Name> Example
 */

#include <snes.h>

int main(void) {
    // Initialize
    consoleInit();

    // Main loop
    while (1) {
        WaitForVBlank();
    }

    return 0;
}
```

3. Update parent Makefile if needed (examples/<category>/Makefile)

4. Verify build: `cd examples/<category>/<name> && make`

## For Audio Examples with SNESMOD

**Makefile:**
```makefile
OPENSNES := $(shell cd ../../.. && pwd)
TARGET   := <name>.sfc
ROM_NAME := "OPENSNES <NAME>      "

CSRC     := main.c
USE_LIB  := 1
USE_SNESMOD := 1
SOUNDBANK_SRC := music.it

include $(OPENSNES)/make/common.mk
```
