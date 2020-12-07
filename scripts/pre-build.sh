#!/bin/bash
set -e
set -o pipefail

bash scripts/fbs-invalidator.sh
# bash scripts/rnl-invalidator.sh
