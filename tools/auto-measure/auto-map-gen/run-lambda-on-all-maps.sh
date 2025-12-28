#!/bin/bash
set -e

lambda="$1"
[[ -n "$lambda" ]]
[[ -f "$lambda" ]]

tools=~/dev/revolution-now/tools
config= #ttmm
gamegen=~/dev/revolution-now/tools/auto-measure/auto-map-gen/gamegen/config/$config

for f in $(find "$gamegen" -name "COLONY*.*"); do
  echo "------------------------------------------------------"
  echo "running: $f"
  echo "------------------------------------------------------"
  "$tools/sav/conversion/lambda-on-binary.sh" "$f" "$lambda"
done