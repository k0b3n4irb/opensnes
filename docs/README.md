# OpenSNES Documentation

Welcome to the OpenSNES documentation.

## For Users

| Document | Description |
|----------|-------------|
| [Getting Started](GETTING_STARTED.md) | Quick start guide |
| [API Reference](api/README.md) | Complete API documentation |
| [Hardware Overview](hardware/README.md) | SNES hardware concepts |
| [Tutorials](tutorials/README.md) | Step-by-step guides |
| [FAQ](FAQ.md) | Frequently asked questions |

## For Contributors

| Document | Description |
|----------|-------------|
| [Contributing Guide](CONTRIBUTING.md) | How to contribute |
| [Code Style](CODE_STYLE.md) | Coding standards |
| [Architecture](ARCHITECTURE.md) | SDK design overview |
| [Testing Guide](TESTING.md) | Writing tests |

## Documentation Standards

All documentation should:

1. **Be clear and concise** - Write for developers new to SNES
2. **Include examples** - Show, don't just tell
3. **Stay current** - Update when code changes
4. **Cross-reference** - Link to related docs

## Building Documentation

```bash
# Generate API docs from source (requires Doxygen)
make docs

# View locally
open docs/api/html/index.html
```
