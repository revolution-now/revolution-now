--[[ ------------------------------------------------------------
|
| runner.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2022-05-13.
|
| Description: Top-level runner for lua tests.
|
--]] ------------------------------------------------------------
local M = {}

local U = require( 'test.unit' )

local files = { 'map-gen.classic.resource-dist-test' }

local function bar()
  print( '-----------------------------------------------' )
end

local function run_test_file( quiet, file )
  local tests = require( file )
  if not quiet then
    bar()
    print( 'Test: ' .. file )
    bar()
  end
  U.runner( quiet, tests )
end

local function main( quiet )
  for _, file in ipairs( files ) do run_test_file( quiet, file ) end
end

return main