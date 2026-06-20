#!/usr/bin/env bash
# install-luna.sh — fetch the pinned luna emulator binary for the test harness.
#
# Mirrors scripts/install-mesen2.sh: downloads a pinned release binary, verifies
# its SHA-256, and installs it locally. luna is consumed as a *pinned binary*,
# not a submodule (see /tmp/luna_migration_FINAL_2026-06-20.md §0bis).
#
#   - Version pin:   tools/luna-test/luna.version  (e.g. "v0.2.0")
#   - Install path:  tools/luna-test/bin/luna      (gitignored)
#   - Dev override:  $LUNA_BIN  → if set to an existing file, skip the download
#                    and use that binary (local luna build for co-development).
#
# Usage:  scripts/install-luna.sh
# Exit 0 on success (bin/luna present and runnable); non-zero otherwise.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LUNA_DIR="$REPO_ROOT/tools/luna-test"
BIN_DIR="$LUNA_DIR/bin"
BIN="$BIN_DIR/luna"
REPO="k0b3n4irb/luna"

mkdir -p "$BIN_DIR"

# --- Dev override: use a local luna build, skip the download ----------------
if [[ -n "${LUNA_BIN:-}" ]]; then
    if [[ ! -x "$LUNA_BIN" ]]; then
        echo "install-luna: \$LUNA_BIN set but not executable: $LUNA_BIN" >&2
        exit 1
    fi
    ln -sf "$LUNA_BIN" "$BIN"
    echo "install-luna: using \$LUNA_BIN override → $LUNA_BIN"
    "$BIN" --version
    exit 0
fi

# --- Pinned release download ------------------------------------------------
VERSION="$(tr -d '[:space:]' < "$LUNA_DIR/luna.version")"
case "$(uname -m)" in
    x86_64)         ARCH=x86_64 ;;
    aarch64|arm64)  ARCH=aarch64 ;;
    *) echo "install-luna: unsupported arch $(uname -m)" >&2; exit 1 ;;
esac

TARBALL="luna-${VERSION}-linux-${ARCH}.tar.gz"
BASE="https://github.com/${REPO}/releases/download/${VERSION}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

echo "install-luna: fetching ${TARBALL} (${VERSION}, ${ARCH})"
if command -v gh >/dev/null 2>&1; then
    gh release download "$VERSION" --repo "$REPO" \
        --pattern "${TARBALL}" --pattern "${TARBALL}.sha256" \
        --dir "$TMP" --clobber
else
    curl -fsSL "$BASE/${TARBALL}"        -o "$TMP/${TARBALL}"
    curl -fsSL "$BASE/${TARBALL}.sha256" -o "$TMP/${TARBALL}.sha256"
fi

echo "install-luna: verifying SHA-256"
( cd "$TMP" && sha256sum -c "${TARBALL}.sha256" )

tar xzf "$TMP/${TARBALL}" -C "$TMP"
install -m755 "$TMP/luna-${VERSION}-linux-${ARCH}/luna" "$BIN"

echo "install-luna: installed → $BIN"
"$BIN" --version
