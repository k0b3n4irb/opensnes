#!/usr/bin/env bash
# install-luna.sh — fetch the pinned luna emulator binary for the test harness.
#
# Downloads the pinned luna release for this OS (Linux, macOS, Windows under
# MSYS2 / Git Bash), verifies its SHA-256 against testing/luna.sha256,
# and installs it locally. luna is consumed as a *pinned binary*,
# not a submodule (see /tmp/luna_migration_FINAL_2026-06-20.md §0bis).
#
#   - Version pin:   testing/luna.version  (e.g. "v1.32.0")
#   - Archive sums:  testing/luna.sha256   (the four zips of that version)
#   - Install path:  testing/bin/luna      (gitignored)
#   - Dev override:  $LUNA_BIN  → if set to an existing file, skip the download
#                    and use that binary (local luna build for co-development).
#
# Usage:  scripts/install-luna.sh
# Exit 0 on success (bin/luna present and runnable); non-zero otherwise.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
LUNA_DIR="$REPO_ROOT/testing"
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
    # The override may name the install path itself (LUNA_BIN=testing/bin/luna
    # to reuse what is there): then there is nothing to link.
    if [[ "$(realpath "$LUNA_BIN")" != "$(realpath -m "$BIN")" ]]; then
        ln -sf "$LUNA_BIN" "$BIN"
    fi
    echo "install-luna: using \$LUNA_BIN override → $LUNA_BIN"
    "$BIN" --version
    exit 0
fi

# --- Pinned release download ------------------------------------------------
# luna publishes one zip per platform, named luna_<version>_<os>_<arch>.zip
# with a top-level directory of the same name (layout since the release
# cleanup of 2026-10-04: before, Linux and macOS got a tar.gz named
# luna-<version>-<os>-<aarch64|x86_64> with a .sha256 sidecar). The archive's
# SHA-256 is pinned in testing/luna.sha256, next to the version pin:
# a release re-published with other bytes fails here instead of installing.
# Linux, macOS and Windows (MSYS2 / Git Bash) are all published.
VERSION="$(tr -d '[:space:]' < "$LUNA_DIR/luna.version")"
case "$(uname -m)" in
    x86_64|amd64)   ARCH=x86_64 ;;
    aarch64|arm64)  ARCH=arm64 ;;
    *) echo "install-luna: unsupported arch $(uname -m)" >&2; exit 1 ;;
esac
EXE=""
case "$(uname -s)" in
    Linux)                  OS=linux ;;
    Darwin)                 OS=darwin ;;
    MINGW*|MSYS*|CYGWIN*)   OS=windows; EXE=.exe ;;
    *) echo "install-luna: unsupported OS $(uname -s)" >&2; exit 1 ;;
esac

NAME="luna_${VERSION}_${OS}_${ARCH}"
ARCHIVE="${NAME}.zip"
BASE="https://github.com/${REPO}/releases/download/${VERSION}"
SUMS="$LUNA_DIR/luna.sha256"
STAMP="$BIN_DIR/luna.installed"

WANT="$(grep -E "^[0-9a-f]{64}  ${ARCHIVE}\$" "$SUMS" 2>/dev/null | cut -d' ' -f1 || true)"
if [[ -z "$WANT" ]]; then
    echo "install-luna: no SHA-256 for ${ARCHIVE} in ${SUMS} — after a pin bump, add the" >&2
    echo "  four archives' sums (see the header of that file)" >&2
    exit 1
fi

# Already installed from this very archive: nothing to fetch (make clean keeps
# bin/, and a test run must not need the network when the binary is in place).
if [[ -x "${BIN}${EXE}" && -f "$STAMP" && "$(cat "$STAMP")" == "${WANT}  ${ARCHIVE}" ]]; then
    echo "install-luna: ${ARCHIVE} already installed → ${BIN}${EXE}"
    "${BIN}${EXE}" --version
    exit 0
fi

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

echo "install-luna: fetching ${ARCHIVE} (${VERSION})"
# luna is a public repo: the release-download URL needs no auth. Prefer curl so
# a stale/absent GH_TOKEN (locally or in CI) can't break the install with a 401
# — the old gh-first path did exactly that. gh is a fallback for a private repo.
# luna keeps binaries for its five newest releases only (rule since 2026-10-03);
# an older pinned version has its tag but no asset. Say so instead of leaving a
# bare download error.
no_asset_hint() {
    code="$(curl -sIL -o /dev/null -w '%{http_code}' "$BASE/${ARCHIVE}" 2>/dev/null || echo 000)"
    if [ "$code" = "404" ]; then
        cat >&2 <<EOM
install-luna: no binary for ${VERSION} at ${BASE}
  luna publishes binaries for its five newest releases only; older versions
  keep their git tag. Either pin a newer version in testing/luna.version
  or build this one from its tag:
    git clone https://github.com/${REPO} && cd luna && git checkout ${VERSION} && cargo build --release -p luna-cli
  then copy target/release/luna to testing/bin/.
EOM
    fi
}
if curl -fsSL "$BASE/${ARCHIVE}" -o "$TMP/${ARCHIVE}"; then
    :
elif command -v gh >/dev/null 2>&1 \
     && gh release download "$VERSION" --repo "$REPO" \
        --pattern "${ARCHIVE}" --dir "$TMP" --clobber; then
    :
else
    no_asset_hint
    echo "install-luna: download of ${ARCHIVE} (${VERSION}) failed" >&2
    exit 1
fi

echo "install-luna: verifying SHA-256 against ${SUMS}"
if command -v sha256sum >/dev/null 2>&1; then
    ( cd "$TMP" && echo "${WANT}  ${ARCHIVE}" | sha256sum -c - )
else    # macOS
    ( cd "$TMP" && echo "${WANT}  ${ARCHIVE}" | shasum -a 256 -c - )
fi

( cd "$TMP" && unzip -q "${ARCHIVE}" )
install -m755 "$TMP/${NAME}/luna${EXE}" "${BIN}${EXE}"
# The GUI (`luna-gui <rom.sfc>` opens a window) ships in the same archive.
if [[ -f "$TMP/${NAME}/luna-gui${EXE}" ]]; then
    install -m755 "$TMP/${NAME}/luna-gui${EXE}" "$BIN_DIR/luna-gui${EXE}"
fi
echo "${WANT}  ${ARCHIVE}" > "$STAMP"

echo "install-luna: installed → ${BIN}${EXE}"
"${BIN}${EXE}" --version
