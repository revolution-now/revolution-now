#!/bin/bash
# This script should be bound to the fish shell command
# `sav-diff` to allow quickly diffing two SAV files by converting
# them to json tmp files.
set -eo pipefail
# set -x

interactive="$(dirname "$0")"
conv="$(realpath "$interactive/../conversion")"

log() {
  # Since this is a "cat" script, we don't want to send anything
  # to stdout other than the stuff we're catting.
  : echo "$*" 1>&2
}

l_sav_path="$1"
log "l_sav_path: $l_sav_path"
[[ -e "$l_sav_path" ]]

r_sav_path="$2"
log "r_sav_path: $r_sav_path"
[[ -e "$r_sav_path" ]]

l_sav_file="$(basename "$l_sav_path")"
log "l_sav_file: $l_sav_file"
[[ "$l_sav_file" =~ .*\.SAV* ]]

r_sav_file="$(basename "$r_sav_path")"
log "r_sav_file: $r_sav_file"
[[ "$r_sav_file" =~ .*\.SAV* ]]

l_tmp_file="/tmp/$l_sav_file.json"
log "l_tmp_file: $l_tmp_file"
rm -f "$l_tmp_file"

r_tmp_file="/tmp/$r_sav_file.json"
log "r_tmp_file: $r_tmp_file"
rm -f "$r_tmp_file"

log "converting $l_sav_file to JSON file $l_tmp_file..."
$conv/binary-to-json.sh "$l_sav_file" "$l_tmp_file"
[[ -e "$l_tmp_file" ]]

log "converting $r_sav_file to JSON file $r_tmp_file..."
$conv/binary-to-json.sh "$r_sav_file" "$r_tmp_file"
[[ -e "$r_tmp_file" ]]

log "diffing $l_tmp_file vs $r_tmp_file..."

diff -u5 "$l_tmp_file" "$r_tmp_file" | nvim -c 'set syntax=diff' -R -