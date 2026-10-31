#!/bin/bash
set -e
# This script is meant for what you just need to compile a couple
# of files and so can do it on multiple platforms simultaneously.

export DSICILIA_NINJA_STATUS_PRINT_MODE=multiline
export DSICILIA_NINJA_REFORMAT_MODE=pretty

concurrency=

export CCACHE_DEPEND=true

if printenv | grep REMOTE; then
  export CCACHE_PREFIX="$HOME/dev/redist/src/builder.sh"
  export CCACHE_PREFIX_CPP=xxx
  concurrency="$("$HOME/dev/redist/src/available-concurrency.sh")"
  if [[ -n "$concurrency" ]]; then
    concurrency="-j$concurrency"
  fi
fi

go() {
  local d="$1"
  cd "$d"
  # No quotes around concurrency so that if it is empty then it
  # won't be a parameter.
  ninja all $concurrency
}

folders="$(ls -d .builds/*/)"

for folder in $folders; do
  test -d "$folder" || continue
  test -L "$folder" && continue
  [[ "$folder" =~ coverage ]] && continue
  [[ "$folder" =~ current  ]] && continue
  # echo "folder: $folder"
  go "$folder" &
done

wait