#!/bin/bash
set -o pipefail

do_build() {
  clear
  make test
}
export -f do_build

[[ -z "$fd" ]] && fd="$(which fd 2>/dev/null)"
[[ -z "$fd" ]] && fd="$(which fdfind 2>/dev/null)"
[[ -n "$fd" ]]

$fd '.*' test | entr bash -c 'do_build'
