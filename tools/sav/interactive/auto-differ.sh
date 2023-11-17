#!/bin/bash
set -eo pipefail
# set -x

this="$(dirname "$0")"
export this

sav_path=$1
echo "sav_path: $sav_path"
[[ -e "$sav_path" ]]

conv() {
  local sav_path="$1"
  # Convert the latest version fo the SAV file to JSON and rotate
  # the latest/previous symlinks.
  echo "running $this/auto-sav-converter.sh $sav_path..."
  "$this/auto-sav-converter.sh" "$sav_path"

  local sav_file="$(basename "$sav_path")"
  echo "sav_file: $sav_file"
  [[ "$sav_file" =~ ^COLONY[0-9][0-9]\.SAV ]]

  local sav_dir="$(dirname "$sav_path")"
  echo "sav_dir: $sav_dir"
  [[ -d "$sav_dir" ]]

  local out_dir="$sav_dir/$sav_file.dir"
  echo "out_dir: $out_dir"
  mkdir -p "$out_dir"

  local left="$out_dir/previous"
  echo "left: $left"
  [[ -e "$left" ]]

  local right="$out_dir/latest"
  echo "right: $right"
  [[ -e "$right" ]]

  diff -u5 "$left" "$right" | nvim -R -
}

export -f conv

echo "$sav_path" | entr bash -c "conv $sav_path"