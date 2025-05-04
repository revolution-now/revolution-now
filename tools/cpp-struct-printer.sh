#!/bin/bash
set -e

ops='
  s/[a-z]+:://g
  s/\[/{/g
  s/\]/}/g
  s/[a-zA-Z0-9_]+\{/{/g
  s/\{([a-zA-Z_])/{.\1/g
  s/,([a-zA-Z_])/,.\1/g
'

tr -d '\n' | sed -r "$ops"
echo \;
