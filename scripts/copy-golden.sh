#!/bin/bash
set -e

cp .builds/current/src/rnl/testing.hpp test/data/rnl-testing-golden.hpp
cp .builds/current/src/fb/testing_generated.h test/data/fb-testing-golden.h