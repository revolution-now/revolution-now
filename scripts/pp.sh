#!/bin/bash
set -e

[[ -z "$1" ]] && {
  echo 'first parameter is stem.'
  exit 1
}

stem=$1

get_flag() {
  local flag=$1
  cat .builds/current/src/CMakeFiles/rn.dir/flags.make \
    | grep "^$flag"                                    \
    | sed -r 's/^[^ ]+ = (.*)/\1/'
}

CXX_FLAGS=$(get_flag CXX_FLAGS)
CXX_DEFINES=$(get_flag CXX_DEFINES)
CXX_INCLUDES=$(get_flag CXX_INCLUDES)

clear

$HOME/dev/tools/llvm-current/bin/clang++ \
  -E                                     \
  $CXX_DEFINES                           \
  $CXX_INCLUDES                          \
  $CXX_FLAGS                             \
  -o $stem.pp.cpp                        \
  -c $HOME/dev/rn/src/$stem.cpp

cat $stem.pp.cpp | sed '0,/marker_start/d; /marker_end/,$d' \
                 | grep -v marker_start                     \
                 | grep -v marker_end                       \
                   >$stem.pp.fmt.cpp

clang-format -i ./$stem.pp.fmt.cpp

vimcat ./$stem.pp.fmt.cpp

rm $stem.pp.cpp
rm $stem.pp.fmt.cpp
