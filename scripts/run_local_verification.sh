#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

run() {
  printf 'run_local_verification: %s\n' "$*"
  "$@"
}

cd "$ROOT_DIR"

run cmake -S tests/external_consumer -B build-external-consumer -G Ninja
run cmake --build build-external-consumer

run cmake -S . -B build-check -G Ninja
run cmake --build build-check
run ./build-check/mrgee_jsfx_smoke_artefacts/RelWithDebInfo/mrgee_jsfx_smoke
run ./build-check/mrgee_jsfx_asset_smoke_artefacts/RelWithDebInfo/mrgee_jsfx_asset_smoke
run ./build-check/mrgee_midi_bridge_smoke_artefacts/RelWithDebInfo/mrgee_midi_bridge_smoke
run ./build-check/mrgee_vst3_smoke_artefacts/RelWithDebInfo/mrgee_vst3_smoke \
  "./build-check/MrgeeSwitchableFilter_artefacts/RelWithDebInfo/VST3/Mrgee Switchable Filter.vst3"
