# mrgee-fx

Turn a JSFX script into the start of a dedicated JUCE VST3/AU plugin.

`mrgee-fx` is meant to be imported by another CMake project. You bring a `.jsfx` file, call one CMake function, and get a plugin target with:

- the JSFX script embedded in the plugin binary
- optional JSFX companion files/directories bundled with it
- stable JUCE parameters derived from JSFX sliders as `slider1`, `slider2`, ...
- a generic JUCE editor generated from the slider metadata
- `ysfx` runtime execution
- VST3, AU, and Standalone targets by default

It is a bootstrap for product plugins, not a generic script-runner plugin.

## Quick Start

Create a new plugin repo with this shape:

```text
acme-tape-delay/
  CMakeLists.txt
  jsfx/
    tape-delay.jsfx
    lib/
    Data/
```

Add a `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.24)

project(acme_tape_delay VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# ysfx runtime. Fetch this before mrgee-fx so the `ysfx` CMake target exists.
set(YSFX_PLUGIN OFF CACHE BOOL "" FORCE)
set(YSFX_TESTS OFF CACHE BOOL "" FORCE)
set(YSFX_TOOLS OFF CACHE BOOL "" FORCE)
set(YSFX_SKIP_CHECKSUM ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    ysfx
    GIT_REPOSITORY https://github.com/jpcima/ysfx.git
    GIT_TAG 8077347ccf4115567aed81400281dca57acbb0cc
    GIT_SUBMODULES_RECURSE TRUE
)

FetchContent_MakeAvailable(ysfx)

FetchContent_Declare(
    mrgee_fx
    GIT_REPOSITORY https://github.com/gmcnicol/mrgee-fx.git
    GIT_TAG main
)

FetchContent_MakeAvailable(mrgee_fx)

mrgee_add_jsfx_plugin(
    TARGET AcmeTapeDelay
    PRODUCT_NAME "Acme Tape Delay"
    PLUGIN_CODE TdL1
    JSFX_FILE "${CMAKE_CURRENT_SOURCE_DIR}/jsfx/tape-delay.jsfx"
    JSFX_ASSETS
        "${CMAKE_CURRENT_SOURCE_DIR}/jsfx/lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/jsfx/Data"
)
```

Build it:

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

`ysfx` is mandatory. Fetch it before `mrgee-fx`; otherwise configuration fails.

On macOS, the default formats produce artefacts under paths like:

```text
build/AcmeTapeDelay_artefacts/VST3/Acme Tape Delay.vst3
build/AcmeTapeDelay_artefacts/AU/Acme Tape Delay.component
build/AcmeTapeDelay_artefacts/Standalone/Acme Tape Delay.app
```

For multi-config generators such as Xcode, the configuration name appears in the artefact path.

## Your JSFX File

Slider declarations become plugin parameters. This JSFX:

```js
desc:Acme Tape Delay

slider1:250<1,2000,1>Delay
slider2:35<0,95,1>Feedback
slider3:50<0,100,1>Mix

@sample
spl0 = spl0;
spl1 = spl1;
```

creates stable parameter IDs:

```text
slider1
slider2
slider3
```

Those IDs are intentionally based on the JSFX slider layout, not display names. Rename display labels carefully, but treat slider order and numbering as the automation/state contract once a plugin has shipped.

## Companion Assets

Use `JSFX_ASSETS` for imports, helper scripts, samples, lookup tables, images, or data files needed by the JSFX project:

```cmake
mrgee_add_jsfx_plugin(
    TARGET AcmeSampler
    PRODUCT_NAME "Acme Sampler"
    PLUGIN_CODE SmP1
    JSFX_FILE "${CMAKE_CURRENT_SOURCE_DIR}/jsfx/sampler.jsfx"
    JSFX_ASSETS
        "${CMAKE_CURRENT_SOURCE_DIR}/jsfx/lib"
        "${CMAKE_CURRENT_SOURCE_DIR}/jsfx/Data"
)
```

At runtime, `mrgee-fx` writes the bundle into a target-specific temp directory:

- the main script is written under `Effects/`
- assets under a logical `Data/` path are written to sibling `Data/`
- other assets are written under `Effects/` with relative paths preserved

That layout matches normal JSFX import and data lookup expectations.

## Plugin Metadata

Required arguments:

- `TARGET`
- `PRODUCT_NAME`
- `PLUGIN_CODE`
- `JSFX_FILE`

Optional arguments:

- `COMPANY_NAME`, default `Mrgee`
- `PLUGIN_MANUFACTURER_CODE`, default `MrgE`
- `FORMATS`, default `VST3 AU Standalone`
- `JSFX_ASSETS`, files or directories to bundle beside the script
- `COPY_PLUGIN_AFTER_BUILD`, optional JUCE copy-to-plugin-folder behavior

Example with explicit metadata:

```cmake
mrgee_add_jsfx_plugin(
    TARGET AcmeFilter
    PRODUCT_NAME "Acme Filter"
    COMPANY_NAME "Acme Audio"
    PLUGIN_MANUFACTURER_CODE Acme
    PLUGIN_CODE FlT1
    FORMATS VST3 AU
    JSFX_FILE "${CMAKE_CURRENT_SOURCE_DIR}/jsfx/filter.jsfx"
)
```

JUCE plugin codes are four-character identifiers. Pick stable values before distributing builds.

## MIDI And Instruments

Declare the plugin role at the CMake call site:

```cmake
mrgee_add_jsfx_plugin(
    TARGET AcmeMidiEcho
    PRODUCT_NAME "Acme MIDI Echo"
    PLUGIN_CODE MdE1
    JSFX_FILE "${CMAKE_CURRENT_SOURCE_DIR}/jsfx/midi-echo.jsfx"
    NEEDS_MIDI_INPUT
    NEEDS_MIDI_OUTPUT
    IS_MIDI_EFFECT
)
```

Available role flags:

- `NEEDS_MIDI_INPUT`
- `NEEDS_MIDI_OUTPUT`
- `IS_MIDI_EFFECT`
- `IS_SYNTH`

The runtime path forwards JUCE MIDI buffers into `ysfx`, receives MIDI output back from `ysfx`, and preserves event offsets inside each processing block.

## Runtime

`ysfx` is always required and always linked:

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

`mrgee-fx` does not support building without `ysfx`.

## What You Get First

The first generated plugin is deliberately plain:

- dynamic parameters from JSFX sliders
- generic controls for enum and continuous sliders
- APVTS state save/restore
- bundled JSFX loading through `ysfx`
- basic status text from the runtime

That gets the DSP into a host quickly. From there, make it a real product:

1. Lock the JSFX slider list and parameter IDs.
2. Confirm the VST3/AU loads in your target hosts.
3. Replace the generic editor with a plugin-specific JUCE UI.
4. Add any metering, response plots, preset handling, and branded controls.
5. Keep JUCE responsible for product UX; keep JSFX responsible for DSP behavior.

## Alpha Testing

Unsigned builds are fine for early testers, but macOS and Windows will show trust warnings. Use [docs/alpha-tester-install.md](/Users/gareth/src/mrgee-fx/docs/alpha-tester-install.md) as the tester-facing install guide for unsigned macOS and Windows alpha builds.

## Import Behavior

When `mrgee-fx` is imported by another project, examples and smoke tools are off by default. Only the public `mrgee_add_jsfx_plugin(...)` helper is exposed.

When `mrgee-fx` is configured as the top-level project, `MRGEE_BUILD_EXAMPLES` defaults to `ON` and builds the included `MrgeeSwitchableFilter` example plus smoke tools.

## Dependencies

This repo expects JUCE and ysfx to be available in one of these ways:

- already provided by the parent CMake project
- present under `third_party/JUCE` and `third_party/ysfx` in this repo
- JUCE fetched automatically when `MRGEE_FETCH_JUCE=ON`

For target projects, prefer the Quick Start `FetchContent` pattern: fetch `ysfx` first, then fetch `mrgee-fx`.

For local development on this repo:

```bash
./scripts/bootstrap_deps.sh
```

## Verifying This Repo

Run:

```bash
cmake -S tests/external_consumer -B build-external-consumer -G Ninja
cmake --build build-external-consumer
cmake -S . -B build-check -G Ninja
cmake --build build-check
./build-check/mrgee_jsfx_smoke_artefacts/RelWithDebInfo/mrgee_jsfx_smoke
./build-check/mrgee_jsfx_asset_smoke_artefacts/RelWithDebInfo/mrgee_jsfx_asset_smoke
./build-check/mrgee_midi_bridge_smoke_artefacts/RelWithDebInfo/mrgee_midi_bridge_smoke
./build-check/mrgee_vst3_smoke_artefacts/RelWithDebInfo/mrgee_vst3_smoke \
  "./build-check/MrgeeSwitchableFilter_artefacts/RelWithDebInfo/VST3/Mrgee Switchable Filter.vst3"
```

Or run:

```bash
./scripts/run_local_verification.sh
```
