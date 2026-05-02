# mrgee-fx

This repo is for exporting dedicated JUCE plugins from bundled JSFX projects, with `ysfx` providing the JSFX runtime and JUCE providing the product UI.

It is not intended to ship a generic user-facing script runner.

## Architecture

- One plugin target per bundled JSFX project
- The JSFX source is embedded into that target at build time
- `ysfx` executes the bundled JSFX at runtime
- JUCE owns the editor, parameter bindings, state, branding, and plugin packaging

Today the repo ships one dedicated example target:

- CMake target: `MrgeeSwitchableFilter`
- Product name: `Mrgee Switchable Filter`
- Bundled JSFX: `assets/jsfx/SwitchableFilter.jsfx`

The intended v1 controls are:

- `Mode`
- `Cutoff`
- `Q`
- `Slope`

## Importable CMake API

External projects can import this repo with `FetchContent` or `add_subdirectory` and create a plugin target with:

```cmake
FetchContent_MakeAvailable(mrgee_fx)

mrgee_add_jsfx_plugin(
    TARGET AcmeTapeDelay
    PRODUCT_NAME "Acme Tape Delay"
    PLUGIN_CODE TdL1
    JSFX_FILE "${CMAKE_CURRENT_SOURCE_DIR}/jsfx/tape-delay.jsfx"
    JSFX_ASSETS
        "${CMAKE_CURRENT_SOURCE_DIR}/jsfx/lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/jsfx/Data"
    NEEDS_MIDI_INPUT
)
```

Required arguments:

- `TARGET`
- `PRODUCT_NAME`
- `PLUGIN_CODE`
- `JSFX_FILE`

Optional metadata arguments:

- `COMPANY_NAME` defaults to `Mrgee`
- `PLUGIN_MANUFACTURER_CODE` defaults to `MrgE`
- `FORMATS` defaults to `VST3 AU Standalone`
- `JSFX_ASSETS` accepts files or directories to bundle beside the script at runtime

Per-target MIDI role is declared at the CMake call site rather than hidden in shared code. Optional flags are:

- `NEEDS_MIDI_INPUT`
- `NEEDS_MIDI_OUTPUT`
- `IS_MIDI_EFFECT`
- `IS_SYNTH`

The helper:

- embeds the JSFX script and optional companion assets for that plugin target
- generates target-local JSFX bundle metadata
- builds a dedicated JUCE plugin target
- links `ysfx` when `MRGEE_USE_YSFX=ON`
- lets each exported plugin declare whether it is an audio effect, MIDI effect, synth, or hybrid audio+MIDI processor

Example:

```cmake
mrgee_add_jsfx_plugin(
    TARGET MrgeeMidiEcho
    PRODUCT_NAME "Mrgee MIDI Echo"
    PLUGIN_CODE MdE1
    JSFX_FILE assets/jsfx/MidiEcho.jsfx
    NEEDS_MIDI_INPUT
    NEEDS_MIDI_OUTPUT
    IS_MIDI_EFFECT
)
```

When this repo is the top-level project, `MRGEE_BUILD_EXAMPLES` defaults to `ON` and builds the shipped example and smoke tools. When imported by another project, it defaults to `OFF`, leaving only the public `mrgee_add_jsfx_plugin(...)` function available unless explicitly enabled.

At runtime, the bundled JSFX script is written under a target-specific temp `Effects/` directory. Assets whose logical path starts with `Data/` are written to the sibling `Data/` directory; other assets preserve their logical path under `Effects/`, which supports normal JSFX import and data lookup conventions.

## JUCE UI Workflow

The intended split of responsibilities is:

- JSFX defines DSP behavior and the raw slider model
- `ysfx` executes the bundled JSFX and reports slider metadata
- JUCE turns that metadata into a product UI

For a new plugin target, the practical UI flow should be:

1. Bundle one JSFX project into one plugin target.
2. Read slider descriptors from the bundled script/runtime in `JsfxHost`.
3. Create APVTS parameters from those descriptors in `PluginProcessor`.
4. Decide which controls should stay generic and which should get custom treatment in the editor.

In this repo today:

- enum sliders render as `ComboBox`
- continuous sliders render as rotary `Slider`
- parameter attachments are owned by the editor
- status text comes from `JsfxHost`

That is the baseline, not the final design ceiling.

For production-quality per-plugin UI, the expected pattern is:

- keep the parameter IDs stable and derived from the bundled JSFX slider layout
- keep DSP control ownership in the processor/APVTS layer
- use JUCE components only as views over those parameters
- replace generic controls selectively with branded components, grouped layouts, metering, response plots, or explanatory text

In other words:

- `ysfx` should not dictate the final UI
- the JSFX slider set is the control contract
- JUCE is where the plugin becomes a product

Recommended implementation steps for a new plugin:

1. Get the bundled JSFX compiling and rendering correctly through `ysfx`.
2. Lock the exported parameter list and IDs.
3. Build a minimal dynamic editor to prove the surface.
4. Replace the generic layout with a plugin-specific JUCE design once the parameter contract is stable.

Files to start from:

- [src/JsfxHost.cpp](/Users/gareth/src/mrgee-fx/src/JsfxHost.cpp)
- [src/PluginProcessor.cpp](/Users/gareth/src/mrgee-fx/src/PluginProcessor.cpp)
- [src/PluginEditor.cpp](/Users/gareth/src/mrgee-fx/src/PluginEditor.cpp)

## UI Design Tooling

There is no modern JUCE-first visual designer you should rely on for polished FX products.

Recommended workflow:

1. Mock the product UI in Figma, Penpot, or Sketch.
2. Define the parameter contract from the bundled JSFX.
3. Implement the real editor in JUCE `Component` code.
4. Attach controls to APVTS parameters.
5. Add custom drawing, metering, plots, and branded interaction on top.

Practical guidance:

- Use the current dynamic editor as a bootstrap only.
- Move quickly from generic controls to plugin-specific components.
- Build reusable JUCE widgets for knobs, switches, segmented selectors, meters, keyboards, step editors, and plots.
- Keep layout and styling in JUCE, not in JSFX metadata.
- Treat JSFX slider names and ranges as the control contract, not as the final visual design.

For a “nice UI,” the code structure you usually want is:

- `PluginProcessor`
  owns parameter/state definition
- `PluginEditor`
  owns layout and composition
- custom JUCE components
  own the look/feel and interaction details

That lets you ship a clean product UI without changing the underlying JSFX DSP contract.

## MIDI Processing Considerations

If you want to export custom MIDI JSFX as plugins other people can use, the main architectural points are:

- The plugin target must declare the right MIDI capabilities in JUCE.
- The host path must preserve MIDI input, MIDI output, and sample-accurate event offsets.
- The UI should make event behavior obvious, especially for generators, arps, harmonizers, remappers, and channel tools.

Important implementation considerations:

- Decide whether the plugin is:
  - MIDI effect only
  - instrument with MIDI input
  - audio effect that also transforms MIDI
- Make the per-plugin JUCE flags in `mrgee_add_jsfx_plugin(...)` match that decision.
- Translate between JUCE `MidiBuffer` events and the `ysfx` MIDI APIs consistently.
- Preserve event timing offsets within the processing block.
- Be explicit about pass-through behavior for notes, CC, pitch bend, aftertouch, transport, and panic/all-notes-off handling.
- Test zero-output and dense-output cases, especially for generators and echo/repeater style effects.

In the current runtime path:

- incoming JUCE MIDI is forwarded into `ysfx` before block processing
- outgoing `ysfx` MIDI is copied back into JUCE after block processing
- sample offsets are preserved within the block rather than collapsed to block boundaries

For product behavior, also decide:

- whether incoming MIDI is transformed or passed through unchanged by default
- whether generated MIDI is merged with input or replaces it
- whether the effect depends on host tempo/transport
- how state restore interacts with held notes or latched/sequenced patterns

Verification you should expect for MIDI plugins:

- note-in to note-out behavior
- CC pass-through or transformation behavior
- timing offset preservation inside a block
- transport/tempo-dependent behavior
- state round-trip without stuck-note regressions

Licensing note for MIT-distributed JSFX projects:

- MIT is a good fit for redistributing your own JSFX logic.
- Keep the JSFX project license text in the repo and, if you want clean downstream reuse, include it alongside the bundled plugin source/assets too.
- If a plugin bundles third-party JSFX or borrowed tables/data, verify those assets are also compatible with MIT redistribution.

## Runtime Policy

- `MRGEE_USE_YSFX=ON` is the real product path.
  `ysfx` loads and runs the bundled JSFX, and JUCE surfaces the plugin UI and automation.
- `MRGEE_USE_YSFX=OFF` is a degraded developer/build path.
  The plugin still exposes the bundled metadata so the JUCE parameter/UI surface can be inspected, but audio bypasses.

## Bootstrap

```bash
./scripts/bootstrap_deps.sh
```

The repo expects:

- `third_party/JUCE`
- `third_party/ysfx` with submodules initialized recursively

This repo includes minimal local `third_party/ysfx` CMake fixes so it can be embedded as a subdirectory cleanly.

## Verification

Manual path:

### `MRGEE_USE_YSFX=OFF`

```bash
cmake -S . -B build-check-off -G Ninja -DMRGEE_USE_YSFX=OFF
cmake --build build-check-off
./build-check-off/mrgee_jsfx_smoke_artefacts/mrgee_jsfx_smoke
./build-check-off/mrgee_jsfx_asset_smoke_artefacts/mrgee_jsfx_asset_smoke
cmake -S tests/external_consumer -B build-external-consumer -G Ninja
cmake --build build-external-consumer
```

### `MRGEE_USE_YSFX=ON`

```bash
cmake -S . -B build-check-on -G Ninja -DMRGEE_USE_YSFX=ON
cmake --build build-check-on
./build-check-on/mrgee_jsfx_smoke_artefacts/RelWithDebInfo/mrgee_jsfx_smoke
./build-check-on/mrgee_jsfx_asset_smoke_artefacts/RelWithDebInfo/mrgee_jsfx_asset_smoke
./build-check-on/mrgee_midi_bridge_smoke_artefacts/RelWithDebInfo/mrgee_midi_bridge_smoke
./build-check-on/mrgee_vst3_smoke_artefacts/RelWithDebInfo/mrgee_vst3_smoke \
  "./build-check-on/MrgeeSwitchableFilter_artefacts/RelWithDebInfo/VST3/Mrgee Switchable Filter.vst3"
```

Shortcut:

```bash
./scripts/run_local_verification.sh
```

Smoke targets:

- `mrgee_jsfx_smoke`
  Verifies the direct JUCE/processor/runtime surface for the dedicated plugin.
- `mrgee_jsfx_asset_smoke`
  Verifies JSFX companion assets are materialized under `Effects/` and sibling `Data/`.
- `mrgee_midi_bridge_smoke`
  Verifies JUCE-to-`ysfx` MIDI input/output bridging and per-block event offset preservation.
- `mrgee_vst3_smoke`
  Loads the built VST3 in a clean process and verifies parameter presence, audio behavior, and state restore.

## Ralph Handoff

- `TASKS.md` is the task board and execution log
- `PROMPT.md` is the prompt handed to Codex
- `scripts/run_ralph_handoff.sh` runs Codex non-interactively via `codex exec` with full-access flags

If you resume work, start with `TASKS.md`.
