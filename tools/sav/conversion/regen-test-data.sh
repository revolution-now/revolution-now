#!/bin/bash
set -eo pipefail
# set -x

die() {
  echo "error: $*" 1>&2
  exit 1
}

this="$(dirname "$0")"
root="$(realpath "$this/../../../")"
cd "$root"

### From here on, all paths are relative to root.

classic_saves="test/data/saves/classic"
sav="tools/sav"
conversion="tools/sav/conversion"

[[ -d "$classic_saves" ]]

sav_to_json() {
  local SAV="$1"
  [[ -f "$SAV" ]]
  local dir="$(dirname "$SAV")"
  [[ -d "$dir" ]]
  local json_dir="$dir/json"
  [[ -d "$json_dir" ]]
  local basename="$(basename "$SAV")"
  local out_file="$json_dir/$basename.json"
  "$conversion/binary-to-json.sh" "$SAV" "$out_file"
}

json_to_sav() {
  local json="$1"
  [[ -f "$json" ]]
  local dir="$(dirname "$json")"
  [[ -d "$dir" ]]
  local sav_dir="${dir%/json}"
  [[ -d "$sav_dir" ]]
  local basename="$(basename "$json")"
  local out_file="$sav_dir/${basename%.json}"
  # We require this one exist because this scripts first gener-
  # ates the json, then goes back to the SAV, so the json should
  # exist.
  [[ -e "$out_file" ]]
  "$conversion/json-to-binary.sh" "$json" "$out_file"
}

convert_batch_to_json() {
  local batch="$1"
  echo "processing batch [-> json]: $batch"
  [[ -d "$batch" ]]
  local SAVs="$(ls $batch/*.SAV)"
  for SAV in $SAVs; do
    sav_to_json "$SAV"
  done
}

convert_batch_to_binary() {
  local batch="$1"
  echo "processing batch [-> binary]: $batch"
  [[ -d "$batch" ]]
  local jsons="$(ls $batch/json/*.json)"
  for json in $jsons; do
    json_to_sav "$json"
  done
}

main() {
  batches="
    $classic_saves/1990s
    $classic_saves/dutch-viceroy-playthrough
  "
  for batch in $batches; do
    convert_batch_to_json "$batch"
    convert_batch_to_binary "$batch"
  done
}

main
