#!/bin/bash
set -eo pipefail
# set -x

die() {
  echo "error: $*" 1>&2
  exit 1
}

this="$(dirname "$0")"
root="$(realpath "$this/../../")"
cd "$root"

### From here on, all paths are relative to root.

classic_saves="test/data/saves/classic"
sav="tools/sav"

[[ -d "$classic_saves" ]]

parse() {
  local in_file="$1"
  local out_file="$2"
  [[ -f "$in_file" ]]
  "$sav/binary-to-json.sh" "$in_file" "$out_file"
}

convert_file() {
  local SAV="$1"
  [[ -f "$SAV" ]]
  echo "converting SAV: $SAV"
  local dir="$(dirname "$SAV")"
  [[ -d "$dir" ]]
  local json_dir="$dir/json"
  [[ -d "$json_dir" ]]
  local basename="$(basename "$SAV")"
  local out_file="$json_dir/$basename.json"
  parse "$SAV" "$out_file"
}

convert_batch() {
  local batch="$1"
  echo "processing batch: $batch"
  [[ -d "$batch" ]]
  local SAVs="$(ls $batch/*.SAV)"
  for SAV in $SAVs; do
    convert_file "$SAV"
  done
}

main() {
  batches="$(ls $classic_saves/* -d)"
  for batch in $batches; do
    convert_batch "$batch"
  done
}

main
