#!/bin/bash
set -eo pipefail

ninja_log=".builds/current/.ninja_log"

[[ -e "$ninja_log" ]] || {
  echo "$ninja_log does not exist.  You must first run a build."
  exit 1
}

script='
  NF >= 2 && $1 ~ /^[0-9]+$/ {
    printf "%10.3f  %s\n", ($2-$1)/1000, $4
  }
'

printf "%10s  %s\n" seconds name
echo "-------------------------------------------------------------------------"
awk "$script" .builds/current/.ninja_log \
  | sort -nr \
  | head -50 \
  || true