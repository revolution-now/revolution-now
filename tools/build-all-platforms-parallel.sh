#!/bin/bash
set -e
# This script is meant for what you just need to compile a couple
# of files and so can do it on multiple platforms simultaneously.

export DSICILIA_NINJA_STATUS_PRINT_MODE=multiline
export DSICILIA_NINJA_REFORMAT_MODE=pretty

go() {
  local d="$1"
  cd "$d"
  ninja all
}

folders="$(ls -d .builds/*)"

for folder in $folders; do
  test -d "$folder" || continue
  test -L "$folder" && continue
  [[ "$folder" =~ coverage ]] && continue
  # echo "folder: $folder"
  go "$folder" &
done

wait