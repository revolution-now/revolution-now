#!/bin/bash
set -eo pipefail

die() {
  echo "error: $*" 1>&2
  exit 1
}

this="$(dirname "$0")"

cd "$this"

sav="$1"
[[ -f "$sav" ]] || die "must specify input file as first argument to $0."

out="${2:-$sav.json}"
[[ -n "$out" ]] || die "out file is empty."

./with-luarocks-env.sh lua \
    save-parser.lua        \
    sav-structure.json     \
    "$sav"                 \
    "$out"