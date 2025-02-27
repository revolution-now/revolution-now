#!/bin/bash
# ---------------------------------------------------------------
# Generate clang code coverage.
# ---------------------------------------------------------------
# Runs the full end-to-end configure/build/run/coverage process
# required to get an html coverage report.
#
# Usage:
#
#   build-and-run-coverage-report.sh <target> [<targets>...]
#
# or use the fish function `coverage`, e.g.:
#
#   coverage src/gfx/spread-algo.cpp
#
# Must specify at least one target (i.e., a file or folder to
# limit the coverage data), Otherwise it would produce coverage
# for the entire source folder which is too big. But you can pro-
# vide multiple files/folders if desired.
#
# ---------------------------------------------------------------
# Shell options.
# ---------------------------------------------------------------
set -e

# ---------------------------------------------------------------
# Imports.
# ---------------------------------------------------------------
source ~/dev/utilities/bashlib/util.sh

# ---------------------------------------------------------------
# Constants.
# ---------------------------------------------------------------
HTML_OUT=.builds/current/test/unittest.coverage.html
HTML_URL="file://$(realpath "$HTML_OUT")"

# ---------------------------------------------------------------
# Args.
# ---------------------------------------------------------------
# This can be either a file to emit coverage for, or a folder.
targets="$@"

# ---------------------------------------------------------------
# Validation.
# ---------------------------------------------------------------
usage() { die "usage: $0 <target> [<targets>...]"; }

[[ -z "$targets" ]] && {
  error "Must specify files or folders as arguments to limit the" \
        "coverage data.  By default it will produce coverage for" \
        "the entire source folder which is too big."
  usage
}

# ---------------------------------------------------------------
# Step: Configure.
# ---------------------------------------------------------------
log "step: configure"
cmc --clang --libstdcxx --lld --coverage

# ---------------------------------------------------------------
# Step: Build.
# ---------------------------------------------------------------
log "step: build"
make all

# ---------------------------------------------------------------
# Step: Run test/generate coverage data.
# ---------------------------------------------------------------
log "step: run tests"
make test

# ---------------------------------------------------------------
# Step: Merge coverage data and generate html.
# ---------------------------------------------------------------
log "step: generate report"
# NOTE: no quotes around targets.
./tools/clang-coverage-report.sh "$HTML_OUT" $targets

# ---------------------------------------------------------------
# Step: Open report.
# ---------------------------------------------------------------
log "step: open report"
log "coverage HTML file: $HTML_OUT"

echo
echo "Opening coverage HTML in browser. Select a line and use"
echo "the L, B, R keys keys (with and without shift) to navigate."
echo "See generated source code to see what they do."

# These extra args to xclip were apparently needed to get it to
# work within a bash script:
#
#   https://unix.stackexchange.com/questions/589556/
#     piping-to-xclip-doesnt-survive-bash-script-termination
#
echo "$HTML_URL" | xclip -sel p -sel s -sel c
xdg-open "$HTML_URL"
