#!/bin/bash
set -e

# Shows all tests that run at least .1 seconds.
./.builds/current/test/unittest --min-duration=.099 \
    | grep '^[0-9]' \
    | sort -r