#!/bin/bash
set -eo pipefail

die() {
  echo "error: $*" 1>&2
  exit 1
}

this="$(dirname "$0")"

out="$1"
[[ -n "$out" ]] || die "out directory is empty."

source "$this/luarocks-env.sh"
export LUA_PATH="$this/?.lua;$LUA_PATH"

lua                           \
  "$this/gen-cpp-loaders.lua" \
  "$this/sav-structure.json"  \
  "$out"