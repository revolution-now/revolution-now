#!/bin/bash
line_counts() {
  for f in *.txt; do
    echo "$f: $(cat "$f" | wc -l)"
  done
}

line_counts          \
  | sort -nr --key=2 \
  | column -t
