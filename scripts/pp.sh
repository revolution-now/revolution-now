#!/bin/bash
set -e

clear

/usr/local/google/home/dsicilia/dev/tools/gcc-current/bin/g++-8-2-0  -E -DBACKWARD_HAS_BACKTRACE=0 -DBACKWARD_HAS_BACKTRACE_SYMBOL=0 -DBACKWARD_HAS_BFD=0 -DBACKWARD_HAS_DW=1 -DBACKWARD_HAS_DWARF=0 -DBACKWARD_HAS_UNWIND=1 -DBETTER_ENUMS_STRICT_CONVERSION=1 -DFMT_STRING_ALIAS=1 -DSPDLOG_FMT_EXTERNAL -D__CLANG_SUPPORT_DYN_ANNOTATION__ -I/usr/include/SDL2 -I/usr/local/google/home/dsicilia/dev/rn/extern/abseil-cpp -I/usr/local/google/home/dsicilia/dev/rn/extern/base-util/src/include -I/usr/local/google/home/dsicilia/dev/rn/extern/better-enums -I/usr/local/google/home/dsicilia/dev/rn/extern/fmt/include -I/usr/local/google/home/dsicilia/dev/rn/extern/observer-ptr/include -I/usr/local/google/home/dsicilia/dev/rn/extern/scelta/include -I/usr/local/google/home/dsicilia/dev/rn/extern/spdlog/include -I/usr/local/google/home/dsicilia/dev/rn/extern/libucl/include -I/usr/local/google/home/dsicilia/dev/rn/extern/backward-cpp   -fdiagnostics-color=always -g   -Wall -Wextra -pthread -std=c++17 -o adt.pp.cpp -c /usr/local/google/home/dsicilia/dev/rn/src/adt.cpp

cat adt.pp.cpp | sed '0,/marker_start/d; /marker_end/,$d' >adt.pp.fmt.cpp

clang-format -i ./adt.pp.fmt.cpp

vimcat ./adt.pp.fmt.cpp
