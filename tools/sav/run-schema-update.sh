#!/bin/bash
set -eo pipefail

this="$(dirname "$0")"

$this/schema/update-structure-file.sh
$this/conversion/gen-cpp-loaders.sh
$this/conversion/regen-test-data.sh