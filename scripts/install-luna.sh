#!/usr/bin/env bash
# install-luna.sh — fetch the pinned luna emulator binary for the test harness.
#
# Downloads the pinned luna release for this OS (Linux, macOS, Windows under
# MSYS2 / Git Bash), verifies its SHA-256, and installs it locally. luna is consumed as a *pinned binary*,
# not a submodule (see /tmp/luna_migration_FINAL_2026-06-20.md §0bis).
#
#   - Version pin:   tools/luna-test/luna.version  (e.g. "v0.3.0")
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
# Linux, macOS and Windows (MSYS2 / Git Bash): luna publishes all three.
# Until 2026-09-26 this script only knew Linux, while the release zip ships
# it on every OS — `make test` in a user project failed to install on macOS
# and Windows.
VERSION="$(tr -d '[:space:]' < "$LUNA_DIR/luna.version")"
case "$(uname -m)" in
    x86_64|amd64)   ARCH=x86_64 ;;
    aarch64|arm64)  ARCH=aarch64 ;;
    *) echo "install-luna: unsupported arch $(uname -m)" >&2; exit 1 ;;
esac
EXE=""
case "$(uname -s)" in
    Linux)                  OS=linux;   EXT=tar.gz ;;
    Darwin)                 OS=macos;   EXT=tar.gz ;;
    MINGW*|MSYS*|CYGWIN*)   OS=windows; EXT=zip; EXE=.exe ;;
    *) echo "install-luna: unsupported OS $(uname -s)" >&2; exit 1 ;;
esac

NAME="luna-${VERSION}-${OS}-${ARCH}"
ARCHIVE="${NAME}.${EXT}"
BASE="https://github.com/${REPO}/releases/download/${VERSION}"
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
  keep their git tag. Either pin a newer version in tools/luna-test/luna.version
  or build this one from its tag:
    git clone https://github.com/${REPO} && cd luna && git checkout ${VERSION} && cargo build --release -p luna-cli
  then copy target/release/luna to tools/luna-test/bin/.
EOM
    fi
}
if curl -fsSL "$BASE/${ARCHIVE}"        -o "$TMP/${ARCHIVE}" \
   && curl -fsSL "$BASE/${ARCHIVE}.sha256" -o "$TMP/${ARCHIVE}.sha256"; then
    :
elif command -v gh >/dev/null 2>&1 \
     && gh release download "$VERSION" --repo "$REPO" \
        --pattern "${ARCHIVE}" --pattern "${ARCHIVE}.sha256" \
        --dir "$TMP" --clobber; then
    :
else
    no_asset_hint
    echo "install-luna: download of ${ARCHIVE} (${VERSION}) failed" >&2
    exit 1
fi

echo "install-luna: verifying SHA-256"
if command -v sha256sum >/dev/null 2>&1; then
    ( cd "$TMP" && sha256sum -c "${ARCHIVE}.sha256" )
else    # macOS
    ( cd "$TMP" && shasum -a 256 -c "${ARCHIVE}.sha256" )
fi

if [[ "$EXT" == zip ]]; then
    ( cd "$TMP" && unzip -q "${ARCHIVE}" )
else
    tar xzf "$TMP/${ARCHIVE}" -C "$TMP"
fi
install -m755 "$TMP/${NAME}/luna${EXE}" "${BIN}${EXE}"
# The GUI (`luna-gui <rom.sfc>` opens a window) ships in the same archive.
if [[ -f "$TMP/${NAME}/luna-gui${EXE}" ]]; then
    install -m755 "$TMP/${NAME}/luna-gui${EXE}" "$BIN_DIR/luna-gui${EXE}"
fi

echo "install-luna: installed → ${BIN}${EXE}"
"${BIN}${EXE}" --version
