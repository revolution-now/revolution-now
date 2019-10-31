#!/bin/bash
# This script is to be run before each incremental build and will
# manually invalidate any flatbuffers schema files that need to
# be recompiled due to one of their included dependencies having
# changed. Unfortunately, the CMake custom commands that are cur-
# rently being used to compile them do not know about their de-
# pendencies.
set -e
set -o pipefail

cd src/fb

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
  for fb in *.fbs; do
    includes=$(cat $fb | sed -n 's/^include.*"\(.*\)".*/\1/p')
    for inc in $includes; do
      if is_newer_than $inc $fb; then
        finished=0
        echo "Invalidating $fb"
        touch $fb
      fi
    done
  done
  (( finished )) && break;
done
