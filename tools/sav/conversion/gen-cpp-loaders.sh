#!/bin/bash
set -eo pipefail

this="$(dirname "$0")"
sav="$(realpath "$this/../")"
schema="$(realpath "$sav/schema")"
conv="$(realpath "$sav/conversion")"
tools="$sav/.."
root="$tools/.."

out_dir="$root/src/sav"

source "$this/luarocks-env.sh"
export LUA_PATH="$conv/?.lua;$LUA_PATH"

lua                             \
  "$this/gen-cpp-loaders.lua"   \
  "$schema/sav-structure.json"  \
  "$out_dir"