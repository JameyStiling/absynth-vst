#!/usr/bin/env bash
# JUCE is vendored locally but not committed (.gitignore). CI and fresh clones use this script.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
JUCE_DIR="$ROOT_DIR/JUCE"
JUCE_VERSION="${JUCE_VERSION:-8.0.6}"

if [[ -f "$JUCE_DIR/CMakeLists.txt" ]]; then
  echo "==> JUCE already present at $JUCE_DIR"
  exit 0
fi

echo "==> Cloning JUCE $JUCE_VERSION into $JUCE_DIR"
git clone --depth 1 --branch "$JUCE_VERSION" https://github.com/juce-framework/JUCE.git "$JUCE_DIR"
