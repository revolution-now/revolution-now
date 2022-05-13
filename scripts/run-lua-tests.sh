#!/bin/bash
set -eo pipefail

cd src/lua

# May need to use the lua compiled by the game here.
lua -e 'local runner = require( "test.runner" ); runner()'