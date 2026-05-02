#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

run() {
  printf 'run_local_verification: %s\n' "$*"
  "$@"
}

cd "$ROOT_DIR"

run cmake -S . -B build-check-off -G Ninja -DMRGEE_USE_YSFX=OFF
run cmake --build build-check-off
run ./build-check-off/mrgee_jsfx_smoke_artefacts/mrgee_jsfx_smoke
run ./build-check-off/mrgee_jsfx_asset_smoke_artefacts/mrgee_jsfx_asset_smoke
run cmake -S tests/external_consumer -B build-external-consumer -G Ninja
run cmake --build build-external-consumer

run cmake -S . -B build-check-on -G Ninja -DMRGEE_USE_YSFX=ON
run cmake --build build-check-on
run ./build-check-on/mrgee_jsfx_smoke_artefacts/RelWithDebInfo/mrgee_jsfx_smoke
run ./build-check-on/mrgee_jsfx_asset_smoke_artefacts/RelWithDebInfo/mrgee_jsfx_asset_smoke
run ./build-check-on/mrgee_midi_bridge_smoke_artefacts/RelWithDebInfo/mrgee_midi_bridge_smoke
run ./build-check-on/mrgee_vst3_smoke_artefacts/RelWithDebInfo/mrgee_vst3_smoke \
  "./build-check-on/MrgeeSwitchableFilter_artefacts/RelWithDebInfo/VST3/Mrgee Switchable Filter.vst3"
