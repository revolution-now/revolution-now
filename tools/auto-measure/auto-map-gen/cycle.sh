#!/bin/bash
# Example usage:
#
#   $ cycle.sh gamegen/config/bbmm
#
set -eo pipefail
cd "$(dirname "$0")"

dir="$1"
[[ -n "$dir" ]] || {
  echo "usage: $0 <folder-with-savs>" 1>&2
  exit 1
}
[[ -d "$dir" ]]

sav=~/games/colonization/data/MPS/COLONIZE/COLONY00.SAV
[[ -L "$sav" || ! -f "$sav" ]]

real_sav="$(basename "$(realpath "$sav")")"

list="$(find "$dir" -name 'COLONY00.SAV.*' -printf '%f\n' | sort)"

run() {
  for f in $list; do
    if [[ ! -L "$sav" ]]; then
      echo "linking to $f"
      local dst="$(pwd)/$dir/$f"
      [[ -f "$dst" ]]
      ln -s "$dst" "$sav"
      exit
    fi
    if [[ "$(basename "$f")" == "$real_sav" ]]; then
      rm "$sav"
    fi
  done
}

run
# echo "list finished. rerunning from beginning."
rm -f "$sav"
run