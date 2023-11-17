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

out="${2:-$sav.json}"
[[ -n "$out" ]] || die "out file is empty."

source "$this/luarocks-env.sh"
export LUA_PATH="$this/?.lua;$LUA_PATH"

lua                            \
  "$this/binary-to-json.lua"   \
  "$schema/sav-structure.json" \
  "$sav"                       \
  "$out"