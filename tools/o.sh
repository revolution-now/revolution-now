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
  echo "    $0 src/rds/rdsc/parser.cpp"
  echo "    $0 test/cargo-test.cpp"
  echo
  exit 1
}

die() {
  echo "$@" >&2
  exit 1
}

path=$1

file="$(basename "$path")"

file_dir="$(dirname "$path")"

dir="$file_dir"
while true; do
  cmake="$dir/CMakeLists.txt"
  [[ -e $cmake ]] && break
  [[ $dir == . ]] && \
    die "cannot locate CMakeLists.txt file for file $file."
  dir="$(dirname "$dir")"
  cmake="$dir/CMakeLists.txt"
done

# We still need the folder structure under the folder containing
# the CMakeLists.txt because that folder structure is preserved
# in the build directory.
sub_folder=${file_dir/$dir}

# Hack for getting the CMake target name in the source folder,
# since we need that to form the path to the .o file and hence
# the ninja build rule. This assumes that each CMakeLists.txt
# file contains a statement like:
#
#   add_rn_library(
#     <target-name>
#     ...
#   )
#
# from which the <target-name> is extracted. If that fails, then
# try for `add_executable` which should cover the unit testing
# folder.
find_target() {
  local previous_line=$1
  sed -rn "/$previous_line/{n;p}" | sed -r 's/ *(.*)$/\1/'
}

target="$(cat "$cmake" | find_target add_rn_library)"
[[ -z "$target" ]] && \
  target="$(cat "$cmake" | find_target add_executable)"

obj="$dir/CMakeFiles/$target.dir$sub_folder/$file.o"

cd .builds/current
[[ -f "env-vars.mk" ]] && eval "$(cat env-vars.mk | sed 's/ = /=/')"
export DSICILIA_NINJA_STATUS_PRINT_MODE=singleline
export DSICILIA_NINJA_REFORMAT_MODE=pretty
ninja "$obj"