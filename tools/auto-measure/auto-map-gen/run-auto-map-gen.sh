#!/bin/bash
# This one is for testing non-random effects.
set -e
# set -x

die() { echo "fatal: $*" 1>&2; exit 1; }

this="$(realpath "$(dirname "$0")")"
cd "$this"

tools="$(realpath "$this/../../")"
[[ -d "$tools" ]]

export LUA_PATH="$LUA_PATH;$tools/?.lua;$tools/sav/conversion/?.lua;$tools/auto-measure/?.lua"

lua auto-map-gen.lua #"$name"