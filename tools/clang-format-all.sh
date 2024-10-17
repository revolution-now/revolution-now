#!/bin/bash
set -eo pipefail

source ~/dev/utilities/bashlib/util.sh

cd_to_this "$0" # script folder
cd ..           # project root folder
[[ -d src/ ]]   # sanity check

files() {
  fdfind '\.?pp$' src/ exe/ test/     \
    --type=f                          \
    --exclude='test/data'             \
    --exclude='sav-struct.*'
}

erase_current_line() {
  echo -ne "                                                  \r"
}

# Will check that the files are formatted and exit with an error
# code if not.
check() {
  local out=/tmp/clang-format-tmp-output.txt
  for file in $(files); do
    echo -ne "checking: $file                                \r"
    clang-format --fail-on-incomplete-format "$file" &>"$out"
    if ! diff -u "$file" "$out" &>/dev/null; then
      erase_current_line
      diff --color=always -u "$file" "$out"
      echo
      echo "unformatted file: $file"
    fi
  done
  erase_current_line
  echo "all source files already formatted."
}

# Actually format the files (in place).
format() {
  clang-format -i $(files)
}

check
#format