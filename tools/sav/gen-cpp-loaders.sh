#!/bin/bash
set -eo pipefail

sav="$(dirname "$0")"
tools="$sav/.."
root="$tools/.."

out_dir="$root/src/sav"

source "$sav/luarocks-env.sh"
export LUA_PATH="$sav/?.lua;$LUA_PATH"

lua                           \
  "$sav/gen-cpp-loaders.lua" \
  "$sav/sav-structure.json"  \
  "$out_dir"