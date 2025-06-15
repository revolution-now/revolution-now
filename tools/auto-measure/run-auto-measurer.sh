#!/bin/bash
# This one is for testing random effects.
set -e
# set -x

die() { echo "fatal: $*" 1>&2; exit 1; }
usage() { die "usage: $0 <run-dir>"; }

name=$1
[[ -n "$name" ]] || usage

this="$(realpath "$(dirname "$0")")"
cd "$this"

tools="$(realpath "$this/../")"
[[ -d "$tools" ]]

export LUA_PATH="$LUA_PATH;$tools/?.lua;$tools/sav/conversion/?.lua"

[[ -d "$name" ]]
lua auto-measurer.lua "$name"