#!/bin/bash
set -e

cp .builds/current/src/rds/testing.hpp test/data/rds-testing-golden.hpp
cp .builds/current/src/fb/testing_generated.h test/data/fb-testing-golden.h