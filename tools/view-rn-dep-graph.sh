#!/bin/bash
set -e
set -o pipefail

die() {
  echo "$@" >&2
  exit 1
}

[[ -d .builds/current/graphviz ]] || \
  die 'You must run cmc with the --graphviz option.'

which dot &>/dev/null || \
  die 'You must apt install graphviz.'

dot='.builds/current/graphviz/dependencies.dot.rn'

[[ -e $dot ]] || \
  die "Cannot find $dot."

out='/tmp/rn-graphviz-output.svg'

cat $dot | dot -Tsvg > $out

xdg-open $out