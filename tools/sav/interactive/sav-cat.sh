#!/bin/bash
# This script should be bound to the fish shell command `sav-cat`
# to allow quickly dumping SAV files by converting them to json
# tmp files.
set -eo pipefail
# set -x

interactive="$(dirname "$0")"
conv="$(realpath "$interactive/../conversion")"

log() {
  # Since this is a "cat" script, we don't want to send anything
  # to stdout other than the stuff we're catting.
  : echo "$*" 1>&2
}

sav_path="$1"
log "sav_path: $sav_path"
[[ -e "$sav_path" ]]

sav_file="$(basename "$sav_path")"
log "sav_file: $sav_file"
[[ "$sav_file" =~ .*\.SAV* ]]

tmp_file="/tmp/$sav_file.json"
log "tmp_file: $tmp_file"
rm -f "$tmp_file"

log "converting $sav_file to JSON file $tmp_file..."
$conv/binary-to-json.sh "$sav_file" "$tmp_file"
[[ -e "$tmp_file" ]]

cat "$tmp_file"