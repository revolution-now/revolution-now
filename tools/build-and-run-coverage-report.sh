#!/bin/bash
# ---------------------------------------------------------------
# Generate clang code coverage.
# ---------------------------------------------------------------
# Runs the full end-to-end configure/build/run/coverage process
# required to get an html coverage report.
#
# Usage:
#
#   build-and-run-coverage-report.sh <target>
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

# ---------------------------------------------------------------
# Args.
# ---------------------------------------------------------------
# This is a single file to emit coverage for.
target="$1"

# ---------------------------------------------------------------
# Validation.
# ---------------------------------------------------------------
usage() { die "usage: $0 <target>"; }

[[ -z "$target" ]] && {
  error "Must specify a file to limit the coverage data. " \
        "By default it will produce coverage for the entire " \
        "source folder which is too big."
  usage
}

[[ -e "$target" ]] || die "target '$target' does not exist."

# ---------------------------------------------------------------
# Step: Configure.
# ---------------------------------------------------------------
log "step: configure"
cmc --clang --libstdcxx --lld --coverage --cached
# Must be evaluated after we change to the --coverage config.
HTML_URL="file://$(realpath "$HTML_OUT")"

# ---------------------------------------------------------------
# Step: Do a full build.
# ---------------------------------------------------------------
# The build generally will inherit the REMOTE= flag that is
# present in the environment. However, in the event that it is,
# we want to tell it to build the target file locally; see com-
# ment below for why we need to do that.
export REDIST_FILE_LOCAL="$target"
make all

# ---------------------------------------------------------------
# Step: Rebuild target locally if needed.
# ---------------------------------------------------------------
# In the event that the target was build remotely on the build
# farm (either because REMOTE= is set in the environment cur-
# rently and that TU was just built, or if it had been built re-
# motely previously and the object file is still up to date) the
# coverage output will not show any coverage data because line
# numbers in the coverage data will pertain in the line numbers
# in the preprocessed output that was actually compiled (the
# #line directives it contains apparently are only relevant for
# diagnostics). In that case, the user of this script may need to
# force it to be rebuilt locally.
if [[ -n "$RECOMPILE_TARGET_LOCAL" ]]; then
  log "step: forcing target to recompile locally."
  # First we need to force ninja to rebuild it.
  touch "$target"
  # Next we need to force ccache to rebuild it.
  export CCACHE_RECACHE=1
  # This should rebuild only the target TU locally.
  env --unset=REMOTE make all
fi

# ---------------------------------------------------------------
# Step: Run test/generate coverage data.
# ---------------------------------------------------------------
log "step: run tests"
make test

# ---------------------------------------------------------------
# Step: Merge coverage data and generate html.
# ---------------------------------------------------------------
log "step: generate report"
./tools/clang-coverage-report.sh "$HTML_OUT" "$target"
HTML_REAL="$(realpath "$HTML_OUT")"
[[ -f "$HTML_REAL" ]] || die "failed to generate $HTML_REAL."

# ---------------------------------------------------------------
# Step: Open report.
# ---------------------------------------------------------------
log "step: open report"
log "coverage HTML file: $HTML_OUT"
# These extra args to xclip were apparently needed to get it to
# work within a bash script:
#
#   https://unix.stackexchange.com/questions/589556/
#     piping-to-xclip-doesnt-survive-bash-script-termination
#
echo "$HTML_REAL" | xclip -sel p -sel s -sel c
echo
echo "Copied report URL to clipboard, now paste in browser."
echo "Select a line and use the L, B, R keys keys (with and"
echo "without shift) to navigate. See generated source code to"
echo "see what they do."
echo
echo "NOTE: if html file show the source code but lacks coverage"
echo "data then it is possible that the TU was built on the build"
echo "farm.  Rerunning once with RECOMPILE_TARGET_LOCAL=1 should"
echo "fix it from there on."

# ---------------------------------------------------------------
# Step: Configure.
# ---------------------------------------------------------------
log "step: restore configuration"
./tools/set-default-configuration.sh
