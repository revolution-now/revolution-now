#!/bin/bash
this="$(dirname "$0")"

line_counts() {
  for f in $this/*.txt; do
    echo "$f: $(cat "$f" | wc -l)"
  done
}

line_counts          \
  | sort -nr --key=2 \
  | column -t
