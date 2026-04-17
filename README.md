# mrgee-fx bootstrap (JSFX ➜ VST3/AU via C++ host)

This repository is a **starter kit** for building a JUCE-based plugin that can host JSFX logic (via `ysfx`) and export to non-Reaper plugin formats.

## Goals this bootstrap covers

- Build modern plugins with **JUCE + CMake**.
- Keep your DSP in **JSFX-compatible code** and map host controls to JSFX sliders.
- Ship for:
  - macOS first (AU + VST3)
  - then Windows/Linux (VST3)
  - then Raspberry Pi (Linux arm)
- Build a polished, customizable UI in JUCE.

> Right now, this repo includes a complete JUCE plugin shell and slider mapping flow. If `ysfx` is not present, it runs in a mock DSP mode so you can still build UI and routing quickly.

---

## Architecture

- `src/PluginProcessor.*`
  - Defines 8 automatable parameters (`slider1` ... `slider8`).
  - Pushes those values into `JsfxHost::setSlider(...)` every block.
- `src/JsfxHost.h`
  - Thin runtime wrapper that is intended to call into `ysfx`.
  - Currently includes `TODO` points for direct ysfx API hookup.
  - Includes a mock `tanh` soft-clip fallback for immediate testability.
- `src/PluginEditor.*`
  - “Sexy UI” rotary control layout + gradient styling.
  - Uses `AudioProcessorValueTreeState::SliderAttachment` to bind controls.

---

## Quick start (macOS first)

### 1) Clone and bootstrap dependencies

```bash
git clone <your-fork-or-repo-url>
cd mrgee-fx
./scripts/bootstrap_deps.sh
```

### 2) Configure + build

```bash
cmake -S . -B build -DMRGEE_USE_YSFX=ON
cmake --build build --config Release
```

### 3) Output artifacts

- VST3 and AU targets are configured via JUCE in `CMakeLists.txt`.
- Standalone app target is also enabled for quick local testing.

---

## Mapping JUCE controls to JSFX sliders

All slider parameters are created in `createParameterLayout()` and named:

- `slider1`
- `slider2`
- ...
- `slider8`

Each process block:

1. Reads APVTS param value.
2. Calls `jsfxHost.setSlider(index, value)`.
3. Runs JSFX process step via `jsfxHost.process(...)`.

This is the exact place to wire `ysfx_slider_set_value` (or equivalent) once you settle on your ysfx integration style.

---


## Optional alternatives to JUCE

If you later want smaller footprint or different licensing/ergonomics, evaluate:

- **iPlug2** (C++ plugin framework with custom UI path).
- **DPF** (lightweight, good Linux story).

This bootstrap uses JUCE because it is fastest for high-polish UI + multi-format shipping.

---

## Cross-platform roadmap

### macOS

- Build AU + VST3 first.
- Codesign/notarization later once productized.

### Windows

- Generate Visual Studio solution via CMake.
- Build VST3 target only if preferred.

### Linux

- Build VST3 and/or standalone.
- Useful for Bitwig, Reaper, Carla, etc.

### Raspberry Pi

- Cross-compile Linux ARM.
- Keep UI lean and meter redraw rates low.
- Prefer lower oversampling and cautious denormal handling.

---

## Performance notes

- Keep JSFX state allocations out of audio thread.
- Batch parameter updates once per block (already scaffolded).
- If adding FFT/convolution, use JUCE DSP or optimized libs and pre-allocate buffers.
- Move expensive painting to cached images/layers in UI-heavy pages.

---

## Next actions to productionize

1. Implement real `ysfx` loader + VM lifecycle in `JsfxHost`.
2. Add file browser or embedded script preset system.
3. Add CI matrix for macOS/Windows/Linux.
4. Add plugin validation steps (`pluginval`, DAW smoke tests).
5. Add preset management and MIDI learn mapping.

---

## Why this setup

You get JUCE’s plugin-format portability + modern C++ tooling while still authoring logic in JSFX style. That gives a practical bridge from Reaper-native scripting workflows to wider DAW distribution.
