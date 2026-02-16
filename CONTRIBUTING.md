# Contributing to OpenSNES

Thank you for your interest in contributing to OpenSNES!

## Code of Conduct

Be respectful, constructive, and welcoming to newcomers.

## How to Contribute

### Reporting Bugs

1. Search existing issues first
2. Use the bug report template
3. Include:
   - OpenSNES version
   - Steps to reproduce
   - Expected vs actual behavior
   - ROM/example that demonstrates the bug

### Suggesting Features

1. Check the roadmap first
2. Open a feature request issue
3. Explain the use case

### Submitting Code

#### Setup

```bash
# Fork the repository
gh repo fork k0b3n4irb/opensnes

# Clone your fork
git clone https://github.com/YOUR_NAME/opensnes
cd opensnes

# Add upstream remote
git remote add upstream https://github.com/k0b3n4irb/opensnes
```

#### Development Workflow

```bash
# Create feature branch
git checkout -b feature/my-feature

# Make changes...

# Run tests
./tests/compiler/run_tests.sh
OPENSNES_HOME=$(pwd) ./tests/examples/validate_examples.sh --quick

# Commit with descriptive message
git commit -m "feat(component): add new feature

Detailed description of changes.

Closes #123"
```

#### Commit Message Format

We follow [Conventional Commits](https://www.conventionalcommits.org/):

```
<type>(<scope>): <description>

[optional body]

[optional footer]
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation only
- `test`: Adding tests
- `refactor`: Code change that neither fixes nor adds
- `perf`: Performance improvement
- `chore`: Maintenance task

#### Pull Request Checklist

Before submitting:

- [ ] Tests pass locally
- [ ] New code has tests
- [ ] New features have documentation
- [ ] Code follows style guide
- [ ] Commit messages follow format
- [ ] Attribution added (if using external code)

#### Code Review

- All PRs require review before merge
- Address feedback constructively
- Squash commits if requested

## Development Guidelines

### Documentation Requirements

Every new feature must have:

1. **Header documentation** - Doxygen comments in headers
2. **Usage example** - In docs or as example code
3. **Test coverage** - Unit and/or integration tests

### Test Requirements

- Unit tests for all public functions
- Integration tests for feature interactions
- Minimum 80% coverage for new code

### Attribution Requirements

When using external code:

1. Add to `ATTRIBUTION.md`
2. Include header comment in source file
3. Ensure license compatibility

## Questions?

- Open a discussion issue
- Check existing documentation
- Ask in SNESdev community channels
