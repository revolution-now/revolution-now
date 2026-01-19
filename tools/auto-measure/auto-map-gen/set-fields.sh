#!/bin/bash
set -eo pipefail

cd "$(dirname "$0")"

# config_dir=.
config_dir=./gamegen/config/bbbm

files="$(find $config_dir -name '*.SAV.*')"

for f in $files; do
  echo "$f"
  ~/dev/revolution-now/tools/sav/conversion/lambda-edit-in-place.sh "$f" make-map-visible.lambda.lua
done