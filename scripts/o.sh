#!/bin/bash
set -e
set -o pipefail

[[ -d .builds/current ]] || {
  echo "Must be run from root of rn directory."
  exit 1
}

(( $# != 1 )) && {
  echo "Usage: $0 path/to/file.cpp"
  echo
  echo "  examples:"
  echo
  echo "    $0 src/cargo.cpp"
  echo "    $0 src/rnl/rnlc/parser.cpp"
  echo "    $0 test/cargo.cpp"
  echo
  exit 1
}

path=$1

file="$(basename "$path")"

dir="$(dirname "$path")"

cmake="$dir/CMakeLists.txt"

# Hack for getting the CMake target name in the source folder,
# since we need that to form the path to the .o file and hence
# the ninja build rule. This assumes that each CMakeLists.txt
# file contains a line that says:
#
#   set_warning_options( <target> )
#
# from which the target name is extracted.
target="$(cat "$cmake" | sed -rn 's/set_warning_options\( (.*) \)/\1/p')"

obj="$dir/CMakeFiles/$target.dir/$file.o"

cd .builds/current
export DSICILIA_NINJA_STATUS_PRINT_MODE=singleline
export DSICILIA_NINJA_REFORMAT_MODE=pretty
ninja "$obj"