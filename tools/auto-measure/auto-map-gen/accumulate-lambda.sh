#!/bin/bash
set -e

lambda="$1"
[[ -n "$lambda" ]]
[[ -f "$lambda" ]]

tools=~/dev/revolution-now/tools
config=bbmm
gamegen=~/dev/revolution-now/tools/auto-measure/auto-map-gen/gamegen/config/$config

files="$(find "$gamegen" -name "COLONY*.*")"

"$tools/sav/conversion/lambda-on-binaries.sh" "$lambda" $files