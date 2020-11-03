#!/bin/bash
# This script is to be run before each incremental build and will
# manually invalidate any rnls definition files that need to be
# recompiled due to one of their included dependencies having
# changed. Unfortunately, the CMake custom commands that are cur-
# rently being used to compile them do not know about their de-
# pendencies.
set -e
set -o pipefail

cd src/rnl

is_newer_than() {
  local a=$1
  local b=$2
  local count=$(find . -maxdepth 1 -name $a -newer $b | wc -l)
  # We seem to need this branching on OSX.
  if (( count != 0 )); then
    return 0
  else
    return 1
  fi
}

while true; do
  finished=1
  for rnl in *.rnl; do
    includes=$(cat $rnl | sed -n 's/^import[ ]\+\(.*\);/\1/p')
    for inc in $includes; do
      if is_newer_than $inc $rnl; then
        finished=0
        echo "Invalidating $rnl"
        touch $rnl
      fi
    done
  done
  (( finished )) && break;
done
