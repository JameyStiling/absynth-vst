#!/usr/bin/env bash
#
# Local build + package + publish for the Lattice macOS demo.
#
# Mirrors .github/workflows/release.yml, but runs on your Mac — which means
# the sibling ../../lattice-modules is picked up if present, so the demo can
# ship WITH the paid expansion modules (CI can't do that).
#
#   npm run release:patch     0.1.0 -> 0.1.1
#   npm run release:minor     0.1.0 -> 0.2.0
#   npm run release:major     0.1.0 -> 1.0.0
#
# The next version is derived from the newest v*-demo git tag in this repo.
# On success it bumps project() in CMakeLists.txt, commits that, tags
# vX.Y.Z-demo, pushes, and creates the GitHub Release with both zips.
#
# NOTE: bundles are ad-hoc signed (codesign --sign -), same as CI. Downloaders
# on other Macs will get a Gatekeeper prompt and must right-click > Open.
#
set -euo pipefail

bump="${1:-}"
case "$bump" in
  patch|minor|major) ;;
  *) echo "usage: release.sh <patch|minor|major>" >&2; exit 2 ;;
esac

VST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
UI_DIR="$(cd "$VST_DIR/.." && pwd)"          # absynth (absynth-ui)
BUILD_DIR="$VST_DIR/build"
DIST="$VST_DIR/dist"
REPO="JameyStiling/absynth-vst"
cd "$VST_DIR"

# ── Preflight ────────────────────────────────────────────────────────────────
[ -z "$(git status --porcelain)" ] || { echo "working tree not clean — commit or stash first" >&2; exit 1; }
[ "$(git rev-parse --abbrev-ref HEAD)" = "main" ] || { echo "not on main" >&2; exit 1; }
gh auth status >/dev/null 2>&1 || { echo "gh not authenticated — run: gh auth login" >&2; exit 1; }
git fetch --quiet origin main
[ "$(git rev-parse HEAD)" = "$(git rev-parse origin/main)" ] || { echo "local main differs from origin/main — push or pull first" >&2; exit 1; }

# ── Resolve next version from newest v*-demo tag ─────────────────────────────
latest="$(git tag --list 'v*-demo' --sort=-v:refname | head -1)"
base="${latest#v}"; base="${base%-demo}"; base="${base:-0.0.0}"
IFS=. read -r major minor patch <<< "$base"
case "$bump" in
  major) major=$((major + 1)); minor=0; patch=0 ;;
  minor) minor=$((minor + 1)); patch=0 ;;
  patch) patch=$((patch + 1)) ;;
esac
version="${major}.${minor}.${patch}"
tag="v${version}-demo"
git rev-parse -q --verify "refs/tags/$tag" >/dev/null && { echo "tag $tag already exists" >&2; exit 1; }
echo "==> ${latest:-<none>}  ->  $tag"

# ── Bump CMakeLists version; revert it if the build fails ───────────────────
/usr/bin/sed -i '' -E "s/(project\(Lattice VERSION )[0-9]+\.[0-9]+\.[0-9]+/\1${version}/" CMakeLists.txt
grep -q "project(Lattice VERSION ${version})" CMakeLists.txt || { echo "failed to bump CMakeLists.txt" >&2; git checkout -- CMakeLists.txt; exit 1; }
trap 'git checkout -- CMakeLists.txt 2>/dev/null || true' ERR

# ── 1. UI bundle ───────────────────────────────────────────────────────────
( cd "$UI_DIR" && npm run build-plugin )
test -f "$UI_DIR/dist/index.html"

# ── 2. Native build (mirrors CI: no explicit build type) ───────────────────
cmake . -B "$BUILD_DIR" -G Ninja -DJUCE_BUILD_EXAMPLES=OFF
cmake --build "$BUILD_DIR" --parallel 8

APP="$BUILD_DIR/Lattice_artefacts/Standalone/Lattice.app"
VST3="$BUILD_DIR/Lattice_artefacts/VST3/Lattice.vst3"
test -d "$APP" && test -d "$VST3"

# ── 3. Embed the UI bundle into both artefacts ────────────────────────────
for dest in "$VST3/Contents/Resources/ui" "$APP/Contents/Resources/ui"; do
  rm -rf "$dest"; mkdir -p "$dest"
  cp -R "$UI_DIR/dist/." "$dest/"
done

# ── 4. Package (ad-hoc sign, same as CI) ──────────────────────────────────
mkdir -p "$DIST"; rm -f "$DIST"/Lattice_*_Mac.zip
xattr -cr "$APP" "$VST3" 2>/dev/null || true
codesign --force --deep --sign - "$APP"
codesign --force --deep --sign - "$VST3"
ditto -c -k --sequesterRsrc --keepParent "$APP"  "$DIST/Lattice_Demo_Mac.zip"
( cd "$(dirname "$VST3")" && ditto -c -k --sequesterRsrc --keepParent "$(basename "$VST3")" "$DIST/Lattice_VST3_Mac.zip" )

# ── 5. Commit the bump, tag, push, publish ────────────────────────────────
trap - ERR
git add CMakeLists.txt
git commit -m "chore: release ${tag}"
git tag "$tag"
git push --follow-tags origin main
gh release create "$tag" --repo "$REPO" --title "$tag" --generate-notes \
  "$DIST/Lattice_Demo_Mac.zip" "$DIST/Lattice_VST3_Mac.zip"

echo "==> https://github.com/${REPO}/releases/tag/${tag}"
echo "==> lattice-landing download URL follows /releases/latest/ automatically"
