#!/bin/bash
set -e

n="${1:-30}"

./.builds/current/test/unittest --durations=yes \
    | grep '^[0-9]' \
    | sort -r \
    | head -n"$n"