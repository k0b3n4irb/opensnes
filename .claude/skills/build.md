# /build - Build OpenSNES SDK

Build the entire SDK or specific components.

## Usage
```
/build              # Build everything
/build clean        # Clean and rebuild
/build tools        # Build only tools
/build lib          # Build only library
/build examples     # Build only examples
/build <example>    # Build specific example (e.g., /build audio/1_tone)
```

## Implementation

When invoked, execute:

```bash
cd /Users/k0b3/workspaces/github/opensnes

# Parse argument
case "$1" in
  "clean")
    make clean && make
    ;;
  "tools")
    make -C tools
    ;;
  "lib")
    make -C lib
    ;;
  "examples")
    make -C examples
    ;;
  *)
    if [ -d "examples/$1" ]; then
      make -C "examples/$1"
    else
      make
    fi
    ;;
esac
```

## After Building

Report:
1. Build success/failure
2. Any warnings or errors
3. Output files created (*.sfc ROMs)
