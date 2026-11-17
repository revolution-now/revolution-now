#!/bin/bash
set -e
# This script is meant for what you just need to compile a couple
# of files and so can do it on multiple platforms simultaneously.

source ~/dev/utilities/bashlib/util.sh

export DSICILIA_NINJA_STATUS_PRINT_MODE=multiline
export DSICILIA_NINJA_REFORMAT_MODE=pretty

concurrency=

export CCACHE_DEPEND=true

if [[ -v REMOTE ]]; then
  export CCACHE_PREFIX="$HOME/dev/redist/src/builder.sh"
  export CCACHE_PREFIX_CPP=xxx
  concurrency="-j$($HOME/dev/redist/src/available-concurrency.sh)" || {
    printf '\r\033[2K\033[1m\033[31merror\033[00m: no available build farm workers'
    exit 1
  }
fi

go() {
  local d="$1"
  cd "$d"
  env_vars=env-vars.sh
  [[ -f "$env_vars" ]] || die "missing env-vars.sh: $d"
  source "$env_vars"
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