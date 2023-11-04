#!/bin/bash
set -eo pipefail

die() {
  echo "error: $*" 1>&2
  exit 1
}

this="$(dirname "$0")"

json="$1"
[[ -f "$json" ]] || die "must specify input file as first argument to $0."
[[ "$json" =~ \.json$ ]] || die "input file should end in .json"

out="${2:-"${json%.json}"}"
[[ -n "$out" ]] || die "out file is empty."
[[ "$out" =~ \.SAV$ ]] || die "output file should end in .SAV"

source "$this/luarocks-env.sh"
export LUA_PATH="$this/?.lua;$LUA_PATH"

lua                          \
  "$this/json-to-binary.lua" \
  "$this/sav-structure.json" \
  "$json"                    \
  "$out"