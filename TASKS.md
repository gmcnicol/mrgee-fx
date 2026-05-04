# JSFX Per-Plugin Export Board

## Status Schema
- `todo`: not started yet
- `in_progress`: actively being worked
- `blocked`: cannot proceed until a concrete blocker is removed
- `done`: completed and verified

## Objective
Use `ysfx` as the JSFX runtime library for dedicated plugin exports, not as a generic script runner. The repo should produce one branded JUCE plugin target per bundled JSFX project, with JUCE handling the UI and `ysfx` handling DSP execution.

## Current Repo State
- Board status: `in_progress`
- Runtime model:
  - bundled JSFX asset embedded per plugin target
  - `ysfx` is the real DSP path when enabled
  - `ysfx` is a hard runtime requirement
- Primary example plugin:
  - bundled asset: [assets/jsfx/SwitchableFilter.jsfx](/Users/gareth/src/mrgee-fx/assets/jsfx/SwitchableFilter.jsfx)
  - exported product target should be a dedicated switchable-filter plugin, not a generic bridge/runner
- Notes:
  - `2026-04-18`: objective reset after user clarification: the repo should present `ysfx` as a per-plugin export engine with custom JUCE UI, not as a script-runner product.

## Task 1: Refactor build shape to per-plugin targets
- Status: `done`
- Goal: make the top-level build describe dedicated plugin exports rather than a generic JSFX bridge
- Scope:
  - replace generic target framing with a per-plugin helper or equivalent structure
  - rename the shipped example target/product away from `Bridge`
  - keep the repo ready for additional plugin targets later
- Files to inspect/edit:
  - [CMakeLists.txt](/Users/gareth/src/mrgee-fx/CMakeLists.txt)
  - [README.md](/Users/gareth/src/mrgee-fx/README.md)
- Done when:
  - the example plugin builds as a dedicated switchable-filter export target
  - CMake structure makes it obvious how a second JSFX plugin would be added
- Notes:
  - `2026-04-18`: started refactor pass from generic-host framing to dedicated per-plugin target layout.
  - `2026-04-18`: replaced the top-level single bridge declaration with a reusable `mrgee_add_ysfx_plugin(...)` helper and renamed the shipped example target to `MrgeeSwitchableFilter`.
  - `2026-04-18`: verified the renamed dedicated target with configure/build smoke checks.

## Task 2: Remove script-runner branding from product/UI/docs
- Status: `done`
- Goal: eliminate user-facing language that implies the shipped plugin is a generic JSFX loader
- Scope:
  - rename product strings and editor copy
  - update docs to describe bundled-per-plugin export architecture
  - keep `ysfx` positioned as internal runtime machinery for dedicated plugin products
- Files to inspect/edit:
  - [src/PluginEditor.cpp](/Users/gareth/src/mrgee-fx/src/PluginEditor.cpp)
  - [README.md](/Users/gareth/src/mrgee-fx/README.md)
  - [PROMPT.md](/Users/gareth/src/mrgee-fx/PROMPT.md)
- Done when:
  - no primary user-facing string calls the shipped example a bridge or script runner
- Notes:
  - `2026-04-18`: started user-facing cleanup and doc rewrite; README now also needs explicit guidance on how JUCE should own the per-plugin UI layer over bundled JSFX parameters.
  - `2026-04-18`: renamed the shipped editor/product copy to `Mrgee Switchable Filter` and rewrote the README/PROMPT framing around dedicated per-plugin export rather than a generic bridge.
  - `2026-04-18`: added a README section describing how JUCE should own the UI layer over the JSFX slider contract for a dedicated plugin product.

## Task 3: Keep verification aligned with the dedicated-plugin model
- Status: `done`
- Goal: preserve the existing proof points while updating names and paths to the dedicated plugin target
- Scope:
  - direct smoke test still verifies the in-process JUCE/ysfx/plugin surface
  - VST3 smoke host still verifies the built artifact in a clean process
  - scripts/docs point at the dedicated plugin artefact names
- Files to inspect/edit:
  - [tools/jsfx_smoke.cpp](/Users/gareth/src/mrgee-fx/tools/jsfx_smoke.cpp)
  - [tools/vst3_smoke.cpp](/Users/gareth/src/mrgee-fx/tools/vst3_smoke.cpp)
  - [scripts/run_local_verification.sh](/Users/gareth/src/mrgee-fx/scripts/run_local_verification.sh)
- Done when:
  - both smoke tools still pass against the renamed dedicated plugin target
- Notes:
  - `2026-04-18`: updated smoke-tool references and verification paths to the renamed `MrgeeSwitchableFilter` artefacts.
  - `2026-04-18`: verified direct and hosted checks with `./build-check-off/mrgee_jsfx_smoke_artefacts/mrgee_jsfx_smoke`, `./build-check-on/mrgee_jsfx_smoke_artefacts/RelWithDebInfo/mrgee_jsfx_smoke`, and `./build-check-on/mrgee_vst3_smoke_artefacts/RelWithDebInfo/mrgee_vst3_smoke "./build-check-on/MrgeeSwitchableFilter_artefacts/RelWithDebInfo/VST3/Mrgee Switchable Filter.vst3"`.

## Task 4: Rewrite the Ralph handoff launcher for non-interactive full-access Codex
- Status: `done`
- Goal: make the handoff launcher always run Codex in a non-interactive full-access mode
- Scope:
  - remove interactive/yolo branching
  - use `codex exec` with full-access flags
  - keep retry behavior straightforward
- Files to inspect/edit:
  - [scripts/run_ralph_handoff.sh](/Users/gareth/src/mrgee-fx/scripts/run_ralph_handoff.sh)
  - [PROMPT.md](/Users/gareth/src/mrgee-fx/PROMPT.md)
- Done when:
  - the launcher is clearly non-interactive and grants Codex full access by default
- Notes:
  - `2026-04-18`: replaced the launcher’s interactive mode branching with a fixed `codex exec --dangerously-bypass-approvals-and-sandbox -C "$ROOT_DIR"` call so Ralph always runs Codex non-interactively with full access.

## Task 5: Expand docs for JUCE UI workflow and MIDI-export considerations
- Status: `done`
- Goal: document how to design polished JUCE UIs for bundled JSFX plugins and call out the main MIDI-processing/export considerations
- Scope:
  - add practical JUCE UI workflow guidance
  - add recommended design/mockup tooling guidance
  - add MIDI effect/export considerations for JSFX projects intended for redistribution
- Files to inspect/edit:
  - [README.md](/Users/gareth/src/mrgee-fx/README.md)
- Done when:
  - README explains how to approach custom JUCE UI design for these plugins
  - README includes a concise section on MIDI-processing concerns for JSFX-to-plugin exports
- Notes:
  - `2026-04-18`: added README guidance covering external mockup tools, JUCE implementation workflow, custom control strategy, APVTS attachment patterns, and MIDI-processing/export considerations for MIT-licensed JSFX plugins.

## Task 6: Add first-class MIDI support to the project
- Status: `done`
- Goal: make MIDI a core capability of the JSFX-to-plugin architecture rather than a documentation-only consideration
- Scope:
  - bridge JUCE `MidiBuffer` events into and out of `ysfx` during processing
  - make per-plugin CMake declarations able to describe audio effects, MIDI effects, or hybrid plugins
  - keep verification and docs aligned with the new MIDI-capable architecture
- Files to inspect/edit:
  - [src/JsfxHost.h](/Users/gareth/src/mrgee-fx/src/JsfxHost.h)
  - [src/JsfxHost.cpp](/Users/gareth/src/mrgee-fx/src/JsfxHost.cpp)
  - [src/PluginProcessor.cpp](/Users/gareth/src/mrgee-fx/src/PluginProcessor.cpp)
  - [CMakeLists.txt](/Users/gareth/src/mrgee-fx/CMakeLists.txt)
  - [README.md](/Users/gareth/src/mrgee-fx/README.md)
- Done when:
  - the runtime path no longer ignores MIDI
  - plugin targets can declare MIDI capabilities explicitly
  - the repo documents and verifies the MIDI-capable architecture honestly
- Notes:
  - `2026-04-18`: added after user requested first-class MIDI support for exporting custom MIT-licensed MIDI JSFX as plugins.
  - `2026-04-18`: implementation started: refactor the per-plugin CMake helper to declare MIDI capabilities explicitly, bridge JUCE `MidiBuffer` events into/out of `ysfx` with block-offset preservation, and add direct verification for the MIDI path instead of leaving MIDI as a docs-only concern.
  - `2026-04-18`: completed by adding explicit per-target MIDI role flags to `mrgee_add_ysfx_plugin(...)`, moving JUCE-to-`ysfx` MIDI translation into a shared bridge utility used by `JsfxHost`, and adding `mrgee_midi_bridge_smoke` to verify MIDI event round-trip and offset preservation through `ysfx`.
  - `2026-04-18`: verified with `./scripts/run_local_verification.sh`, including `./build-check-on/mrgee_midi_bridge_smoke_artefacts/RelWithDebInfo/mrgee_midi_bridge_smoke`.

## Final Acceptance Criteria
- Status: `done`
- The repo presents the shipped example as a dedicated switchable-filter plugin target, not a generic JSFX bridge.
- `README.md` explains the architecture as “one JUCE plugin per bundled JSFX project.”
- `scripts/run_ralph_handoff.sh` runs Codex non-interactively with full access.
- `ysfx` remains required for configure/build.
- The direct smoke probe still passes.
- The headless VST3 smoke host still passes against the dedicated plugin artifact.
- Notes:
  - set this section to `done` only after all tasks above are either `done`, or a remaining `blocked` task has a documented external blocker and agreed scope cut
  - `2026-04-18`: satisfied with configure/build and JSFX/VST3 smoke checks.
  - `2026-04-18`: re-verified after Task 6 with `./scripts/run_local_verification.sh`, including the new `mrgee_midi_bridge_smoke` proof point for JUCE-to-`ysfx` MIDI bridging.
