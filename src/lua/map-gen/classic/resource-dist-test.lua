--[[ ------------------------------------------------------------
|
| resource-dist-test.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2022-05-13.
|
| Description:  Unit tests for classic map resource distribution.
|
--]] ------------------------------------------------------------
local dist = require( 'map-gen.classic.resource-dist' )
local data = require( 'map-gen.classic.resource-dist-test-data' )

local U = require( 'test.unit' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local ASSERT_EQ = U.ASSERT_EQ

local Test = U.new_test_pack()

local rep = string.rep
local format = string.format

-----------------------------------------------------------------
-- Constants.
-----------------------------------------------------------------
-- Map size in original game.
local MAP_SIZE = { w=56, h=70 }

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function bar( w ) print( rep( '-', w ) ) end

-- This is suitable for debugging failed tests and for generating
-- test outputs for new test cases.
local function print_coords( label, coords )
  print( format( '%s:', label ) )
  bar( 65 )
  for i = 1, #coords do
    local x_comma = tostring( coords[i].x ) .. ','
    io.write(
        format( '{ x=%-3s y=%-2s },', x_comma, coords[i].y ) )
    if i % 4 == 0 then
      io.write( '\n' )
    else
      io.write( ' ' )
    end
  end
  if #coords % 4 ~= 0 then io.write( '\n' ) end
  bar( 65 )
end

local function validate_coords( coords, expected )
  ASSERT_EQ( #coords, #expected,
             'lengths of result and expected result' )
  for i = 1, #coords do
    ASSERT_EQ( coords[i].x, expected[i].x,
               'x coordinates at index ' .. i )
    ASSERT_EQ( coords[i].y, expected[i].y,
               'y coordinates at index ' .. i )
  end
end

local function validate_or_print( coords, expected )
  local success, msg = pcall( validate_coords, coords, expected )
  if not success then
    print_coords( 'expected', expected )
    print_coords( 'actual', coords )
  end
  assert( success, msg )
end

-----------------------------------------------------------------
-- Test cases.
-----------------------------------------------------------------
-- In the OG there is only a single seed used for both prime re-
-- sources and LCR in a given game. But for the purposes of these
-- unit tests we don't need to use the same one.

function Test.lost_city_rumors()
  local seed = 3 -- arbitrary
  local coords = dist.compute_lost_city_rumors( MAP_SIZE, seed )
  validate_or_print( coords, data.EXPECTED_LOST_CITY_RUMORS )
end

function Test.prime_ground_resources()
  local seed = 2 -- arbitrary
  local coords = dist.compute_prime_ground_resources( MAP_SIZE,
                                                      seed )
  validate_or_print( coords, data.EXPECTED_PRIME_GROUND_RESOURCES )
end

function Test.prime_forest_resources()
  local seed = 1 -- arbitrary
  local coords = dist.compute_prime_forest_resources( MAP_SIZE,
                                                      seed )
  validate_or_print( coords, data.EXPECTED_PRIME_FOREST_RESOURCES )
end

return Test
