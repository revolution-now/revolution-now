#!/bin/bash
# This script should be bound to the fish shell command `sav-lambda`.
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
[[ "$sav_file" =~ .*\.SAV$ ]]

lambda_name="$2"
log "lambda_name: $lambda_name"

lambda_file="$interactive/lambda/$lambda_name.lua"
log "lambda_file: $lambda_file"
# NOTE: don't check if the lambda file exists here; instead let
# the below script check it because it will create a better error
# message.

$conv/lambda-on-binary.sh "$sav_file" "$lambda_file"