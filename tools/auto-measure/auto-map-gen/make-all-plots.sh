#!/bin/bash
set -e

lambda=land-density

go() {
  local config="$1"
  ./accumulate-lambda.sh "$config" "map-analysis/$lambda.lua"
}

# biomes.
# go bbtt &
# go bbtm &
# go bbtb &
# go bbmt &
# go bbmm &
# go bbmb &
# go bbbt &
# go bbbm &
# go bbbb &
# go new  &

# land density.
# go mmmm &
# go tmmm &
# go bmmm &
# go mtmm &
# go mbmm &
# go ttmm &
# go bbmm &
# go tbmm &
# go btmm &
# go new  &

wait