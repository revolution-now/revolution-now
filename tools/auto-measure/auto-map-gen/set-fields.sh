#!/bin/bash
set -eo pipefail

cd "$(dirname "$0")"

config_dir=.
# config_dir=./gamegen/config/bbbb

edit() {
  local f="$1"
  [[ -z "$f" ]] && return
  echo "$f"
  ~/dev/revolution-now/tools/sav/conversion/lambda-edit-in-place.sh "$f" make-map-visible.lambda.lua
}

go() {
  while true; do
    [[ -z "$1" ]] && break

    shift; edit "$1" &
    shift; edit "$1" &
    shift; edit "$1" &
    shift; edit "$1" &
    shift; edit "$1" &
    shift; edit "$1" &
    shift; edit "$1" &
    shift; edit "$1" &
    shift; edit "$1" &
    shift; edit "$1" &
    shift; edit "$1" &
    shift; edit "$1" &
    shift; edit "$1" &
    shift; edit "$1" &
    shift; edit "$1" &
    shift; edit "$1" &
    shift; edit "$1" &
    shift; edit "$1" &
    shift; edit "$1" &
    shift; edit "$1" &

    wait
  done
}

files="$(find $config_dir -name '*.SAV.*')"

go _ $files