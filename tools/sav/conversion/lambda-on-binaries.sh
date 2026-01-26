#!/bin/bash
set -eo pipefail

die() {
  echo "error: $*" 1>&2
  exit 1
}

this="$(dirname "$0")"
sav="$(realpath "$this/../")"
schema="$(realpath "$sav/schema")"

label="$1"
[[ -n "$label" ]] || die "label argument is empty."
shift

lambda="$1"
[[ -n "$lambda" ]] || die "lambda file argument is empty."
[[ -e "$lambda" ]] || die "lambda file '$lambda' not found."
shift

export LUA_PATH="$this/?.lua;$sav/?.lua;$LUA_PATH"

lua                              \
  "$this/lambda-on-binaries.lua" \
  "$schema/sav-structure.json"   \
  "$label"                       \
  "$lambda"                      \
  "$@"