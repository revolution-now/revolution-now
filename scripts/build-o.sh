#!/bin/bash
set -e
set -o pipefail

(( $# != 1 )) && {
  echo "Usage: build-o.sh path/to/file.cpp"
  echo
  echo "  examples:"
  echo
  echo "    build-o.sh src/cargo.cpp"
  echo "    build-o.sh src/rnl/rnlc/parser.cpp"
  echo "    build-o.sh test/cargo.cpp"
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
ninja "$obj"