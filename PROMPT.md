Refactor this repo as a dedicated JSFX-to-plugin export workspace, not a generic JSFX runner.

## Product direction
- One JUCE plugin target per bundled JSFX project
- `ysfx` is the embedded JSFX runtime
- JUCE owns the product UI and plugin packaging
- Do not reintroduce generic bridge/runner framing

## Operating rules
1. Use `TASKS.md` as the authoritative task board.
2. Update `TASKS.md` as you work.
3. Prefer direct validation over assumptions.
4. Keep vendored `third_party/ysfx` edits minimal and documented.
5. Do not revert unrelated user changes.

## Verification shortcut
- `./scripts/run_local_verification.sh`
