#!/bin/bash
set -e

lambda=rivers

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

# lakes.
# go ttmm &
# go tmmm &
# go tbmm &
# go mtmm &
# go mmmm &
# go mbmm &
# go btmm &
# go bmmm &
# go bbmm &
# go new  &

# rivers.
# go bbbb &
# go bbbm &
# go bbmb &
# go bbmm &
# go bbmt &
# go bbtm &
# go bbtt &
# go bmmm &
# go mbmb &
# go mbmm &
# go mbmt &
# go mmmb &
# go mmmm &
# go mmmt &
# go mtmb &
# go mtmm &
# go mtmt &
# go tmmm &
# go ttmb &
# go ttmm &
# go ttmt &
# go new  &

wait

collect