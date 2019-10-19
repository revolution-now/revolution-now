#!/bin/bash
set -e

[[ -z "$1" ]] && {
  echo 'first parameter is stem.'
  exit 1
}

stem=$1

[[ -z "$2" ]] && looking_for_stem=a1b2c3 \
              || looking_for_stem=$2

get_flag() {
  local flag=$1
  cat .builds/current/src/CMakeFiles/rn.dir/flags.make \
    | grep "^$flag"                                    \
    | sed -r 's/^[^ ]+ = (.*)/\1/'
}

get_flag_ninja() {
  local flag=$1
  local chunk=$(cat .builds/current/build.ninja | grep ".*rn\.dir.*$stem\.cpp\.o:" -A6)
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

(( ninja )) && escape_pwd="../../src/" \
            || escape_pwd=$(pwd)/src/
escape_pwd="${escape_pwd//\//\\/}"

(( ninja )) && pushd .builds/current &>/dev/null

esc=$(printf '\033')
RED="\033[91m"
NORMAL="\033[0m"
BOLD="\033[1m"

$HOME/dev/tools/llvm-current/bin/clang++ \
    -H                                   \
    -E                                   \
    $CXX_DEFINES                         \
    $CXX_INCLUDES                        \
    $CXX_FLAGS                           \
    -o $HOME/dev/rn/$stem.pp.cpp         \
    -c $HOME/dev/rn/src/$stem.cpp        \
    2>&1                                 \
    | sed -rn "s/$escape_pwd//p"         \
    | sed -r "s/$looking_for_stem\.hpp/${esc}[91m${esc}[1m$looking_for_stem.hpp${esc}[0m/g"

(( ninja )) && popd &>/dev/null

rm $stem.pp.cpp
