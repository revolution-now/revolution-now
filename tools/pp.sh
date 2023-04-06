#!/bin/bash
set -e

[[ -z "$1" ]] && {
  echo 'first parameter is stem path without src/, e.g. cargo, base/to-str'
  exit 1
}

stem_path=$1
stem="$(basename "$1")"

get_flag() {
  local flag=$1
  cat .builds/current/src/CMakeFiles/rn.dir/flags.make \
    | grep "^$flag"                                    \
    | sed -r 's/^[^ ]+ = (.*)/\1/'
}

get_flag_ninja() {
  local flag=$1
  local chunk=$(cat .builds/current/build.ninja | grep ".*rn.*\.dir.*$stem\.cpp\.o:" -A6)
  echo "$chunk" | sed -n "s/^  $flag = \(.*\)/\1/p"
}

[[ "$(realpath .builds/current)" =~ ninja ]] && ninja=1 || ninja=0

if (( ninja )); then
  CXX_FLAGS=$(get_flag_ninja FLAGS)
  CXX_DEFINES=$(get_flag_ninja DEFINES)
  CXX_INCLUDES=$(get_flag_ninja INCLUDES)
else
  CXX_FLAGS=$(get_flag CXX_FLAGS)
  CXX_DEFINES=$(get_flag CXX_DEFINES)
  CXX_INCLUDES=$(get_flag CXX_INCLUDES)
fi

clear

(( ninja )) && pushd .builds/current &>/dev/null

$HOME/dev/tools/llvm-current/bin/clang++    \
  -E                                        \
  $CXX_DEFINES                              \
  $CXX_INCLUDES                             \
  $CXX_FLAGS                                \
  -o $HOME/dev/revolution-now/$stem.pp.cpp  \
  -c $HOME/dev/revolution-now/src/$stem_path.cpp

(( ninja )) && popd &>/dev/null

cat $stem.pp.cpp | sed '0,/marker_start/d; /marker_end/,$d' \
                 | grep -v marker_start                     \
                 | grep -v marker_end                       \
                   >$stem.pp.fmt.cpp

clang-format -i ./$stem.pp.fmt.cpp

bat --style=plain ./$stem.pp.fmt.cpp

rm $stem.pp.cpp
rm $stem.pp.fmt.cpp
