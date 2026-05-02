#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TASKS_FILE="$ROOT_DIR/TASKS.md"
PROMPT_FILE="$ROOT_DIR/PROMPT.md"
CODEX_BIN="${CODEX_BIN:-codex}"
MAX_ITERATIONS="${RALPH_MAX_ITERATIONS:-5}"

fail() {
  printf 'run_ralph_handoff: %s\n' "$*" >&2
  exit 1
}

require_file() {
  local path="$1"
  [[ -f "$path" ]] || fail "required file not found: $path"
}

validate_iterations() {
  [[ "$MAX_ITERATIONS" =~ ^[1-9][0-9]*$ ]] || fail "RALPH_MAX_ITERATIONS must be a positive integer"
}

require_file "$TASKS_FILE"
require_file "$PROMPT_FILE"
validate_iterations
command -v "$CODEX_BIN" >/dev/null 2>&1 || fail "codex executable not found: $CODEX_BIN"

cd "$ROOT_DIR"

PROMPT_TEXT="$(cat "$PROMPT_FILE")"
PROMPT_TEXT+=$'\n\n'
PROMPT_TEXT+="Use TASKS.md at $TASKS_FILE as the task board and update it as you work."

run_codex_once() {
  "$CODEX_BIN" exec \
    --dangerously-bypass-approvals-and-sandbox \
    -C "$ROOT_DIR" \
    "$PROMPT_TEXT"
}

for (( iteration = 1; iteration <= MAX_ITERATIONS; iteration++ )); do
  printf 'run_ralph_handoff: starting non-interactive Codex iteration %d/%d via exec\n' "$iteration" "$MAX_ITERATIONS"

  if run_codex_once; then
    printf 'run_ralph_handoff: Codex completed successfully on iteration %d/%d\n' "$iteration" "$MAX_ITERATIONS"
    exit 0
  fi

  if (( iteration == MAX_ITERATIONS )); then
    fail "Codex failed after $MAX_ITERATIONS iterations"
  fi

  printf 'run_ralph_handoff: Codex exited non-zero on iteration %d/%d, retrying\n' "$iteration" "$MAX_ITERATIONS" >&2
done
