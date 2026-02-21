#!/bin/bash
set -eo pipefail

cd "$(dirname "$0")"

NAMES=~/games/colonization/data/MPS/COLONIZE/NAMES.TXT

OUT=names.txt.json

cmd="print( require( 'moon.json' ).print( require( 'lib.names' ).parse( '$NAMES' ) ) )"

# We need to remove the logging that the parser may emit.
lua -e "$cmd"           \
  | grep -v 'NAMES.TXT' \
  | jq                  \
  | tee "$OUT"          \
  | batcat --paging=never --style=plain -ljson

git diff -- "$OUT"