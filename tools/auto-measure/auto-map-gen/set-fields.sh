#!/bin/bash
set -eo pipefail

cd "$(dirname "$0")"

files="$(find . -name '*.SAV.*')"

for f in $files; do
  echo "$f"
  ~/dev/revolution-now/tools/sav/conversion/lambda-edit-in-place.sh "$f" make-map-visible.lambda.lua
done