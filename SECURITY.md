# Security Policy

OpenSNES is a development SDK: it compiles C into Super Nintendo ROMs and ships
prebuilt toolchain binaries in its releases. The most relevant security concerns
are therefore around the build toolchain and the release artifacts rather than a
running service.

## Supported versions

Security fixes land on the latest released version and the `develop` branch. The
project is in beta and does not backport fixes to older tags.

| Version            | Supported          |
| ------------------ | ------------------ |
| Latest release     | :white_check_mark: |
| Older releases      | :x:                |
| `develop` (tip)    | :white_check_mark: |

## Reporting a vulnerability

**Please do not open a public issue for a security problem.**

Report privately via GitHub's built-in **[Security Advisories](https://github.com/k0b3n4irb/opensnes/security/advisories/new)**
("Report a vulnerability" on the repository's Security tab). This keeps the
report confidential until a fix is available.

If you cannot use GitHub Security Advisories, contact the maintainer through the
profile listed at <https://github.com/k0b3n4irb>.

When reporting, please include:

- a description of the issue and its impact,
- steps to reproduce (a minimal ROM, source snippet, or build command),
- the OpenSNES version or commit, and your OS/architecture.

## What to expect

- **Acknowledgement** within a reasonable time once the report is seen.
- An assessment of severity and scope, and a discussion of the fix.
- **Coordinated disclosure**: we'll agree on a timeline before any public
  write-up, and credit you in the advisory and `CHANGELOG.md` unless you prefer
  to stay anonymous.

## Scope notes

- **In scope:** issues in the OpenSNES library, templates, build system, the
  bundled tools (`gfx4snes`, `font2snes`, `smconv`, …), and the release packaging.
- **Toolchain submodules** (`cproc`, `qbe`, `wla-dx`) are pinned third-party
  projects — report issues there upstream, but a heads-up here is welcome so we
  can pin a fixed revision.
- **Out of scope:** vulnerabilities in third-party emulators used to run ROMs,
  or in software you build *with* OpenSNES.
