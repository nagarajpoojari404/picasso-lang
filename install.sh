#!/usr/bin/env bash
set -e

# ---------------------------------------------------------------------------
# install.sh — install picasso globally from a dist package
#
# Usage:
#   ./install.sh                        # auto-detects latest dist package
#   ./install.sh dist/picasso_v1.0.3_darwin_arm64
#
# What it does:
#   1. Copies the dist package to /usr/local/lib/picasso
#   2. Symlinks the binary to /usr/local/bin/picasso
#   3. Adds PICASSO_HOME / PICASSO_IRGEN / PICASSO_RUNTIME_LIB to ~/.zshrc
# ---------------------------------------------------------------------------

INSTALL_LIB="/usr/local/lib/picasso"
INSTALL_BIN="/usr/local/bin/picasso"
ZSHRC="$HOME/.zshrc"

# ── 1. Resolve source package ───────────────────────────────────────────────

if [ -n "$1" ]; then
    PKG_DIR="$1"
else
    # Auto-detect: pick the most recently modified dist directory
    PKG_DIR=$(find "$(dirname "$0")/dist" -maxdepth 1 -type d -name "picasso_*" \
               2>/dev/null | sort | tail -1)
fi

if [ -z "$PKG_DIR" ] || [ ! -d "$PKG_DIR" ]; then
    echo "error: could not find a dist package directory."
    echo "Usage: ./install.sh [dist/picasso_<version>_<arch>]"
    exit 1
fi

PKG_DIR="$(cd "$PKG_DIR" && pwd)"   # absolute path

echo "Installing from: $PKG_DIR"

# Sanity-check required files
for f in picasso irgen libruntime_lib.a libs runtime; do
    if [ ! -e "$PKG_DIR/$f" ]; then
        echo "error: required file/dir '$f' not found in $PKG_DIR"
        exit 1
    fi
done

# ── 2. Copy package to /usr/local/lib/picasso ───────────────────────────────

echo "Copying package to $INSTALL_LIB ..."
sudo rm -rf "$INSTALL_LIB"
sudo cp -R "$PKG_DIR" "$INSTALL_LIB"
sudo chmod +x "$INSTALL_LIB/picasso" "$INSTALL_LIB/irgen"

# ── 3. Symlink binary ───────────────────────────────────────────────────────

echo "Linking $INSTALL_BIN ..."
sudo ln -sf "$INSTALL_LIB/picasso" "$INSTALL_BIN"

# ── 4. Write env vars to ~/.zshrc ───────────────────────────────────────────

# Remove any previous picasso env-var block we may have written
if grep -q "# >>> picasso install >>>" "$ZSHRC" 2>/dev/null; then
    # Strip the block between the markers
    sed -i '' '/# >>> picasso install >>>/,/# <<< picasso install <<</d' "$ZSHRC"
fi

# Also remove any loose lines left from manual installs
sed -i '' '/^export PICASSO_HOME=/d'        "$ZSHRC" 2>/dev/null || true
sed -i '' '/^export PICASSO_IRGEN=/d'       "$ZSHRC" 2>/dev/null || true
sed -i '' '/^export PICASSO_RUNTIME_LIB=/d' "$ZSHRC" 2>/dev/null || true

cat >> "$ZSHRC" << EOF

# >>> picasso install >>>
export PICASSO_HOME=$INSTALL_LIB
export PICASSO_IRGEN=$INSTALL_LIB/irgen
export PICASSO_RUNTIME_LIB=$INSTALL_LIB/libruntime_lib.a
# <<< picasso install <<<
EOF

echo ""
echo "Done. Run the following to activate in your current shell:"
echo ""
echo "  source ~/.zshrc"
echo ""
echo "Then verify:"
echo "  picasso build <project-dir>"
