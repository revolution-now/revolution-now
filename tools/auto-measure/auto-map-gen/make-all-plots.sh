#!/bin/bash
set -e

this="$(dirname "$0")"
sav="$(realpath "$this/../../sav")"

limit="$1"
[[ -n "$limit" ]] || {
  echo 'must specify sav file count as first argument.' >&2
  exit 1
}

export LUA_PATH="$sav/?.lua;$LUA_PATH"

declare -A ran_lambdas

run() {
  local lambda="$1"
  local config="$2"
  ran_lambdas["$lambda"]=1
  ./accumulate-lambda.sh "$limit" "$config" "$lambda/empirical.lua" &
}

collect() {
  local lambda="$1.empirical"
  lua -e "require( '$lambda' ).collect()"
}

# biomes.
# run biomes bbtt
# run biomes bbtm
# run biomes bbtb
# run biomes bbmt
# run biomes bbmm
# run biomes bbmb
# run biomes bbbt
# run biomes bbbm
# run biomes bbbb
# run biomes tmmm
# run biomes bmmm
# run biomes mtmm
# run biomes mbmm
# run biomes mmmm
# run biomes new

# land density.
# run landmmmm -density
# run landtmmm -density
# run landbmmm -density
# run landmtmm -density
# run landmbmm -density
# run landttmm -density
# run landbbmm -density
# run landtbmm -density
# run landbtmm -density
# run landnew  -density

# lakes.
# run lakes ttmm
# run lakes tmmm
# run lakes tbmm
# run lakes mtmm
# run lakes mmmm
# run lakes mbmm
# run lakes btmm
# run lakes bmmm
# run lakes bbmm
# run lakes new

# rivers.
# run rivers bbbb
# run rivers bbbm
# run rivers bbmb
# run rivers bbmm
# run rivers bbmt
# run rivers bbtm
# run rivers bbtt
# run rivers bmmm
# run rivers mbmb
# run rivers mbmm
# run rivers mbmt
# run rivers mmmb
# run rivers mmmm
# run rivers mmmt
# run rivers mtmb
# run rivers mtmm
# run rivers mtmt
# run rivers tmmm
# run rivers ttmb
# run rivers ttmm
# run rivers ttmt
# run rivers new

# overlays.
# run overlays mmmm
# run overlays tmmm
# run overlays bmmm
# run overlays mtmm
# run overlays mbmm
# run overlays bbmm
# run overlays bbmt
# run overlays bbmb
# run overlays bbtm
# run overlays bbbm
# run overlays bbtt
# run overlays bbbb
# run overlays new

wait

for lambda in "${!ran_lambdas[@]}"; do
  echo "collecting: $lambda"
  collect "$lambda"
done
# If we need to force a collect.
# collect overlays