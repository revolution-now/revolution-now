#!/bin/bash
set -e
set -o pipefail

start="$1"

die() {
  echo "$@" >&2
  exit 1
}

dot_gen="$(realpath scripts/header-dependency-gen.lua)"

[[ ! -z "$start" ]] || \
  die "first argument must be starting file"

cd $(dirname $start)

stem="$(basename $start)"

out_prefix=/tmp/$stem-dependencies

$dot_gen $(basename $start) \
  | tee $out_prefix.dot     \
  | dot -Tpng -o$out_prefix.png

xdg-open $out_prefix.png