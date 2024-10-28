#!/bin/bash
set -e

./.builds/current/test/unittest --durations=yes \
    | grep '^[0-9]' \
    | sort -r \
    | head -n10