#!/bin/bash
set -e

go() {
  local config="$1"
  ./accumulate-lambda.sh "$config" map-analysis/ground-types.lua
}

go bbtt &
go bbtm &
go bbtb &
go bbmt &
go bbmm &
go bbmb &
go bbbt &
go bbbm &
go bbbb &
wait