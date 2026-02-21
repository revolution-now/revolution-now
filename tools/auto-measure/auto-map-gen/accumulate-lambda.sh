#!/bin/bash
set -e

tools=~/dev/revolution-now/tools

config="$1"
gamegen=$tools/auto-measure/auto-map-gen/gamegen/config/$config
[[ -n "$gamegen" ]]
[[ -d "$gamegen" ]]
echo config=$config

lambda="$2"
[[ -n "$lambda" ]]
[[ -f "$lambda" ]]

files="$(find "$gamegen" -name "COLONY*.*" | head -n2000)"

"$tools/sav/conversion/lambda-on-binaries.sh" "$config" "$lambda" $files