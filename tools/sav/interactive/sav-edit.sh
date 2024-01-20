#!/bin/bash
# This script should be bound to the fish shell command
# `sav-edit` to allow quickly editing SAV files in vim by tem-
# porarily converting them to json.
set -eo pipefail
# set -x

interactive="$(dirname "$0")"
conv="$(realpath "$interactive/../conversion")"

sav_path="$1"
echo "sav_path: $sav_path"
[[ -e "$sav_path" ]]

sav_file="$(basename "$sav_path")"
echo "sav_file: $sav_file"
[[ "$sav_file" =~ .*\.SAV$ ]]

tmp_file="/tmp/$sav_file.json"
echo "tmp_file: $tmp_file"
rm -f "$tmp_file"

echo "converting $sav_path to JSON file $tmp_file..."
$conv/binary-to-json.sh "$sav_path" "$tmp_file"
[[ -e "$tmp_file" ]]

echo "backing up $tmp_file to $tmp_file.copy..."
cp "$tmp_file" "$tmp_file.copy"
[[ -e "$tmp_file.copy" ]]

nvim "$tmp_file"

# Only save if the file was edited.
if ! diff "$tmp_file" "$tmp_file.copy" >/dev/null; then
  echo "converting JSON file $tmp_file to $sav_path..."
  $conv/json-to-binary.sh "$tmp_file" "$sav_path"
fi

echo "finished."