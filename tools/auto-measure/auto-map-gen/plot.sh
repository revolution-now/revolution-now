#!/bin/bash
set -e

config="$1"
[[ -n "$config" ]]

./accumulate-lambda.sh "$config" map-analysis/ground-types.lua
gnuplot -p plots/$config.gnuplot