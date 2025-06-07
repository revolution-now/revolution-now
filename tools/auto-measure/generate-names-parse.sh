#!/bin/bash
set -e

cd "$(dirname "$0")"

NAMES=~/games/colonization/data/MPS/COLONIZE/NAMES.TXT

OUT=names.txt.json

cmd="print( require( 'moon.printer' ).to_json_oneline( require( 'lib.names' ).parse( '$NAMES' ) ) )"

# We need to remove the logging that the parser may emit.
lua -e "$cmd"           \
  | grep -v 'NAMES.TXT' \
  | jq                  \
  | tee "$OUT"          \
  | batcat --paging=never --style=plain -ljson

git diff -- "$OUT"