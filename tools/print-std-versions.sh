#!/bin/bash
set -e

cat .builds/current/build.ninja | sed -rn 's/.*-std=(c\+\+..).*/\1/p' | sort | uniq -c | sort -V
