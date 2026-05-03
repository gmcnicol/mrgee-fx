# Alpha Tester Install Guide

Use this guide when sending unsigned alpha builds to testers.

Unsigned builds are useful for early feedback, but macOS and Windows will treat them as untrusted software. That is expected. Do not present unsigned builds as production releases.

## Package Shape

Package each alpha clearly:

```text
AcmeTapeDelay-alpha-001-mac.zip
AcmeTapeDelay-alpha-001-win64.zip
```

Include a short note beside the plugin files:

```text
README-ALPHA.txt
Acme Tape Delay.vst3
Acme Tape Delay.component
Acme Tape Delay.app
```

The AU component and standalone app are macOS-only. Windows testers normally only need the VST3 bundle.

## Warning Text

Include this text in `README-ALPHA.txt`:

```text
This is an unsigned alpha build for testing only.

macOS and Windows may warn that the developer cannot be verified.
That is expected for this alpha. Only install this build if you received it
directly from us and are comfortable testing pre-release audio software.
```

## macOS

Install paths:

```text
VST3:
~/Library/Audio/Plug-Ins/VST3/

AU:
~/Library/Audio/Plug-Ins/Components/
```

Tester steps:

1. Quit the DAW.
2. Unzip the alpha build.
3. Copy `Acme Tape Delay.vst3` to `~/Library/Audio/Plug-Ins/VST3/`.
4. Copy `Acme Tape Delay.component` to `~/Library/Audio/Plug-Ins/Components/` if testing AU.
5. Remove quarantine if macOS blocks the plugin:

```bash
xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/VST3/Acme Tape Delay.vst3"
xattr -dr com.apple.quarantine "$HOME/Library/Audio/Plug-Ins/Components/Acme Tape Delay.component"
```

6. Reopen the DAW and rescan plugins.

For the standalone app:

```bash
xattr -dr com.apple.quarantine "/path/to/Acme Tape Delay.app"
```

If macOS still blocks the app, right-click it and choose **Open**.

## Logic Pro AU Reset

If Logic does not show the AU, reset the Audio Unit registrar:

```bash
killall -9 AudioComponentRegistrar
```

Then reopen Logic and check Plugin Manager.

## Windows

Install path:

```text
C:\Program Files\Common Files\VST3\
```

Tester steps:

1. Quit the DAW.
2. Unzip the alpha build.
3. Copy `Acme Tape Delay.vst3` to `C:\Program Files\Common Files\VST3\`.
4. If Windows shows a warning when opening a standalone app or installer, click **More info**, then **Run anyway**.
5. If the zip or extracted plugin is blocked, unblock it before rescanning.

Unblock the zip before extracting:

```powershell
Unblock-File ".\AcmeTapeDelay-alpha-001-win64.zip"
```

Or unblock an extracted VST3 bundle:

```powershell
Get-ChildItem "C:\Program Files\Common Files\VST3\Acme Tape Delay.vst3" -Recurse | Unblock-File
```

Then reopen the DAW and rescan plugins.

## What Testers Should Report

Ask testers for:

- OS version
- DAW name and version
- plugin format tested: VST3 or AU
- whether the plugin appeared in the DAW
- whether scan or load failed
- screenshots of warnings or errors
- crash logs if available
- short audio/MIDI behavior notes
- exact alpha build filename

## Release Builds

For normal users, unsigned builds are not enough. Public macOS releases should be Developer ID signed and notarized. Public Windows releases should be Authenticode signed.

