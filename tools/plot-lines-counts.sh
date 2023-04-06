#!/bin/bash
set -e
set -o pipefail

initial=6a8f5905e115d64c20395109bf891b32dca45ad6
range="$initial..master"

while read -r rev; do
  git checkout --quiet "$rev"
  num_lines=$(cloc --quiet --csv --csv-delimiter=' ' --sum-one src | grep -i 'C++' | awk '{total += $NF} END {print total}')
  date=$(git log -1 --format=%cd --date=unix)
  echo "$date,$num_lines"
  #git submodule update --init || {
  #  echo "Commit $rev failed submodule update."
  #  exit 1
  #}
done < <(git rev-list "$range")

git checkout master
