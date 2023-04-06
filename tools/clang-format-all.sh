#!/bin/bash
set -eo pipefail

source ~/dev/utilities/bashlib/util.sh

cd_to_this "$0" # script folder
cd ..           # project root folder
[[ -d src/ ]]   # sanity check

files() {
  fdfind '\.?pp$' src/ exe/ test/ \
    --type=f                      \
    --exclude='test/data'
}

clang-format -i $(files)