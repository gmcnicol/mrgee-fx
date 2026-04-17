#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
mkdir -p "$ROOT_DIR/third_party"

if [[ ! -d "$ROOT_DIR/third_party/JUCE/.git" ]]; then
  git clone --depth 1 --branch 8.0.6 https://github.com/juce-framework/JUCE.git "$ROOT_DIR/third_party/JUCE"
else
  echo "JUCE already present"
fi

if [[ ! -d "$ROOT_DIR/third_party/ysfx/.git" ]]; then
  git clone --depth 1 https://github.com/jpcima/ysfx.git "$ROOT_DIR/third_party/ysfx"
else
  echo "ysfx already present"
fi

echo "Dependencies ready in third_party/."
