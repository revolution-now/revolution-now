#!/bin/bash
set -e

tools=~/dev/revolution-now/tools

limit="$1"
[[ -n "$limit" ]]

config="$2"
gamegen=$tools/auto-measure/auto-map-gen/gamegen/config/$config
[[ -n "$gamegen" ]]
[[ -d "$gamegen" ]]
echo config=$config

lambda="$3"
[[ -n "$lambda" ]]
[[ -f "$lambda" ]]

files="$(find "$gamegen" -name "COLONY*.*" | head "-n$limit")"

# for file in $files; do echo "$file"; done

"$tools/sav/conversion/lambda-on-binaries.sh" "$config" "$lambda" $files