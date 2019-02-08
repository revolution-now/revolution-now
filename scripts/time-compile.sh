#!/bin/bash
# 1:24 to compile menu.cpp
set -e

[[ -f src/menu.cpp ]] || exit 1

touch src/menu.cpp

ccache --clear

cd .builds/clang-4.2.1-libc++-debug/src

time make menu.o
