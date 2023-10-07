#!/bin/bash
# This script takes a module name and runs the unit tests from
# only that module. It is assumed that the module name will ap-
# pear in square brackets in the name of each test case in that
# module.
set -e

module=$1

[[ -n "$module" ]] || {
  echo "usage: $0 <module-name>"
  exit 1
}

make unittest

./.builds/current/test/unittest "*\[$module\]*"