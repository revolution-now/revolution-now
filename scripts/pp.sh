#!/bin/bash
set -e

[[ -z "$1" ]] && {
  echo 'first parameter is stem.'
  exit 1
}

stem=$1

CXX_FLAGS="-Wno-unused-command-line-argument -fcolor-diagnostics -g   -Wall -Wextra -pthread -std=c++2a"

CXX_DEFINES="-DBACKWARD_HAS_BACKTRACE=0 -DBACKWARD_HAS_BACKTRACE_SYMBOL=0 -DBACKWARD_HAS_BFD=0 -DBACKWARD_HAS_DW=1 -DBACKWARD_HAS_DWARF=0 -DBACKWARD_HAS_UNWIND=1 -DBETTER_ENUMS_STRICT_CONVERSION=1 -DFMT_STRING_ALIAS=1 -DSPDLOG_FMT_EXTERNAL -D__CLANG_SUPPORT_DYN_ANNOTATION__"

CXX_INCLUDES="-I$HOME/dev/rn -I/usr/include/SDL2 -I$HOME/dev/rn/extern/abseil-cpp -I$HOME/dev/rn/extern/base-util/src/include -I$HOME/dev/rn/extern/better-enums -I$HOME/dev/rn/extern/expected-lite/include -I$HOME/dev/rn/extern/fmt/include -I$HOME/dev/rn/extern/observer-ptr/include -I$HOME/dev/rn/extern/range-v3/include -I$HOME/dev/rn/extern/rtmidi -I$HOME/dev/rn/extern/scelta/include -I$HOME/dev/rn/extern/spdlog/include -I$HOME/dev/rn/extern/libucl/include -I$HOME/dev/rn/extern/backward-cpp"

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
