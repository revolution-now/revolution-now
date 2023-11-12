#!/bin/bash
set -eo pipefail

this="$(dirname "$0")"

$this/update-structure-file.sh
$this/gen-cpp-loaders.sh
$this/regen-test-data.sh