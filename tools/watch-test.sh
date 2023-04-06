#!/bin/bash
set -o pipefail

build_threads() {
  local threads=4
  if [[ "$(uname)" == Linux ]]; then
    threads=$(nproc --all)
  elif [[ "$(uname)" == Darwin ]]; then
    threads=$(sysctl -n hw.ncpu)
  fi
  echo "$threads"
}
export -f build_threads

do_build() {
  clear
  make test -j$(build_threads)
}
export -f do_build

fd '.*' test | entr bash -c 'do_build'
