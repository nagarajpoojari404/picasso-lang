#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# release.sh — build, tag, publish a GitHub release, and update Homebrew tap
#
# Usage:
#   ./release.sh <version>          e.g.  ./release.sh 1.0.4
#
# Prerequisites:
#   - gh CLI installed and authenticated  (brew install gh)
#   - Both repos have clean working trees or only expected changes
#   - homebrew-picasso repo is at ../homebrew-picasso relative to this script
# ---------------------------------------------------------------------------
set -euo pipefail

# ── colours ────────────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[0;33m'; BOLD='\033[1m'; RESET='\033[0m'
info()  { echo -e "${GREEN}[release]${RESET} $*"; }
warn()  { echo -e "${YELLOW}[release]${RESET} $*"; }
die()   { echo -e "${RED}[release] ERROR:${RESET} $*" >&2; exit 1; }
step()  { echo -e "\n${BOLD}── $* ──${RESET}"; }

# ── args ───────────────────────────────────────────────────────────────────
VERSION="${1:-}"
[ -z "$VERSION" ] && die "Usage: ./release.sh <version>   e.g. ./release.sh 1.0.4"

# Strip leading 'v' if user typed it — we manage the prefix ourselves
VERSION="${VERSION#v}"
TAG="v${VERSION}"
ARCH="darwin_arm64"

# ── paths ──────────────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$SCRIPT_DIR"
DIST_DIR="$ROOT_DIR/dist"
PKG_NAME="picasso_${TAG}_${ARCH}"
PKG_DIR="$DIST_DIR/$PKG_NAME"
TARBALL="$DIST_DIR/${PKG_NAME}.tar.gz"
HOMEBREW_REPO="$(cd "$ROOT_DIR/../homebrew-picasso" 2>/dev/null && pwd)" || \
    die "homebrew-picasso repo not found at ../homebrew-picasso"
FORMULA="$HOMEBREW_REPO/Formula/picasso.rb"
GITHUB_REPO="nagarajRPoojari/picasso"
RELEASE_URL="https://github.com/${GITHUB_REPO}/releases/download/${TAG}/${PKG_NAME}.tar.gz"

# ── preflight checks ───────────────────────────────────────────────────────
step "Preflight checks"

command -v bazel >/dev/null 2>&1 || die "'bazel' not found in PATH"
command -v gh    >/dev/null 2>&1 || die "'gh' not found in PATH — install with: brew install gh"
command -v git   >/dev/null 2>&1 || die "'git' not found in PATH"

gh auth status >/dev/null 2>&1 || die "gh is not authenticated — run: gh auth login"

# Warn (don't block) on dirty working tree in picasso repo
if ! git -C "$ROOT_DIR" diff --quiet || ! git -C "$ROOT_DIR" diff --cached --quiet; then
    warn "picasso repo has uncommitted changes — tagging will include current state"
fi

# Warn on dirty homebrew repo
if ! git -C "$HOMEBREW_REPO" diff --quiet || ! git -C "$HOMEBREW_REPO" diff --cached --quiet; then
    warn "homebrew-picasso repo has uncommitted changes"
fi

# Guard against re-releasing an existing tag
if git -C "$ROOT_DIR" rev-parse "$TAG" >/dev/null 2>&1; then
    die "Tag $TAG already exists. Bump the version or delete the tag first."
fi

info "Releasing  : $TAG"
info "Tarball    : $TARBALL"
info "Formula    : $FORMULA"
info "GitHub repo: $GITHUB_REPO"

# ── 1. build ───────────────────────────────────────────────────────────────
step "Building binaries"

rm -rf "$DIST_DIR"
mkdir -p "$PKG_DIR"

(cd "$ROOT_DIR" && bazel build //cli:picasso //irgen)

cp "$ROOT_DIR/bazel-bin/cli/picasso"        "$PKG_DIR/picasso"
cp "$ROOT_DIR/bazel-bin/irgen/irgen_/irgen" "$PKG_DIR/irgen"
cp "$ROOT_DIR/bazel-bin/libruntime_lib.a"   "$PKG_DIR/libruntime_lib.a"
cp -R "$ROOT_DIR/libs"                      "$PKG_DIR/libs"
cp -R "$ROOT_DIR/runtime"                   "$PKG_DIR/runtime"

chmod +x "$PKG_DIR/picasso" "$PKG_DIR/irgen"

info "Package contents:"
ls -lh "$PKG_DIR"

# ── 2. tarball + sha256 ────────────────────────────────────────────────────
step "Creating tarball"

(cd "$DIST_DIR" && tar -czf "$TARBALL" "$PKG_NAME")
SHA256=$(shasum -a 256 "$TARBALL" | awk '{print $1}')

info "Tarball : $TARBALL"
info "SHA256  : $SHA256"

# ── 3. tag + push picasso repo ────────────────────────────────────────────
step "Tagging picasso repo"

git -C "$ROOT_DIR" add -A
if ! git -C "$ROOT_DIR" diff --cached --quiet; then
    git -C "$ROOT_DIR" commit -m "chore: release $TAG"
fi

git -C "$ROOT_DIR" tag "$TAG"
git -C "$ROOT_DIR" push origin HEAD
git -C "$ROOT_DIR" push origin "$TAG"

info "Pushed tag $TAG to origin"

# ── 4. GitHub release + upload tarball ────────────────────────────────────
step "Creating GitHub release"

gh release create "$TAG" \
    "$TARBALL" \
    --repo "$GITHUB_REPO" \
    --title "Picasso $TAG" \
    --notes "Release $TAG — darwin/arm64 binary distribution"

info "GitHub release $TAG created and tarball uploaded"

# ── 5. Update Homebrew formula ────────────────────────────────────────────
step "Updating Homebrew formula"

# sed replacements that work on both GNU and BSD sed
sed -i '' "s|url \".*\"|url \"${RELEASE_URL}\"|"        "$FORMULA"
sed -i '' "s|sha256 \".*\"|sha256 \"${SHA256}\"|"        "$FORMULA"
sed -i '' "s|version \".*\"|version \"${VERSION}\"|"     "$FORMULA"

info "Formula updated:"
grep -E 'url|sha256|version' "$FORMULA"

# ── 6. Commit + push homebrew tap ─────────────────────────────────────────
step "Pushing Homebrew tap"

git -C "$HOMEBREW_REPO" add Formula/picasso.rb
git -C "$HOMEBREW_REPO" commit -m "picasso $VERSION"
git -C "$HOMEBREW_REPO" push origin HEAD

info "Homebrew tap updated"

# ── done ──────────────────────────────────────────────────────────────────
echo ""
echo -e "${GREEN}${BOLD}Release $TAG complete.${RESET}"
echo ""
echo "  Install with:"
echo "    brew update && brew upgrade picasso"
echo "  Or fresh install:"
echo "    brew install nagarajRPoojari/picasso/picasso"
