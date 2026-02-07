#!/bin/bash
# Example usage:
#
#   $ cycle.sh gamegen/config/bbmm
#
set -eo pipefail

tools=~/dev/revolution-now/tools

lambda=$tools/auto-measure/auto-map-gen/map-analysis/land-form.lua

cd "$(dirname "$0")"

dir="$1"
[[ -n "$dir" ]] || {
  echo "usage: $0 <folder-with-savs>" 1>&2
  exit 1
}
[[ -d "$dir" ]]

sav=~/games/colonization/data/MPS/COLONIZE/COLONY00.SAV
[[ -L "$sav" || ! -f "$sav" ]] || {
  echo "$sav must be either a symlink or not exist." 1>&2
  exit 1
}

real_sav="$(basename "$(realpath "$sav")")"

list="$(find "$dir" -name 'COLONY00.SAV.*' -printf '%f\n' | sort)"

on_linked() {
  lua load-first-sav.lua
  "$tools/sav/conversion/lambda-on-binary.sh" "$sav" "$lambda"
}

run() {
  for f in $list; do
    if [[ ! -L "$sav" ]]; then
      echo "linking to $f"
      local dst="$(pwd)/$dir/$f"
      [[ -f "$dst" ]]
      ln -s "$dst" "$sav"
      on_linked
      exit
    fi
    if [[ "$(basename "$f")" == "$real_sav" ]]; then
      rm "$sav"
    fi
  done
}

run
echo "list finished. rerunning from beginning."
rm -f "$sav"
run