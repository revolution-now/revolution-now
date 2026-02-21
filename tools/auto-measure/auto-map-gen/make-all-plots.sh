#!/bin/bash
set -e

lambda=biomes

this="$(dirname "$0")"
sav="$(realpath "$this/../../sav")"

# export LUA_PATH="$this/?.lua;$sav/?.lua;$LUA_PATH"
export LUA_PATH="$sav/?.lua;$LUA_PATH"

go() {
  local config="$1"
  ./accumulate-lambda.sh "$config" "map-analysis/$lambda.lua"
}

collect() {
  lua -e "require( 'map-analysis/$lambda' ).collect()"
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

collect