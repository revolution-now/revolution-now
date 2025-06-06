#!/bin/bash
set -eo pipefail

die() {
  echo "error: $*" 1>&2
  exit 1
}

this="$(dirname "$0")"
sav="$(realpath "$this/../")"
schema="$(realpath "$sav/schema")"

sav="$1"
[[ -f "$sav" ]] || die "must specify input file as first argument to $0."

lambda="$2"
[[ -n "$lambda" ]] || die "lambda file argument is empty."
[[ -e "$lambda" ]] || die "lambda file '$lambda' not found."

export LUA_PATH="$this/?.lua;$LUA_PATH"

lua                            \
  "$this/lambda-on-binary.lua" \
  "$schema/sav-structure.json" \
  "$sav"                       \
  "$lambda"