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
local ASSERT_EQ = U.ASSERT_EQ

local Test = U.new_test_pack()

-- Map size in original game.
local MAP_SIZE = { w=56, h=70 }

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

function Test.lost_city_rumors()
  local offset = 109 -- arbitrary
  local coords =
      dist.compute_lost_city_rumors( MAP_SIZE, offset )
  validate_coords( coords, data.EXPECTED_LOST_CITY_RUMORS )
end

function Test.prime_ground_resources()
  local offset = 1 -- arbitrary
  local coords = dist.compute_prime_ground_resources( MAP_SIZE,
                                                      offset )
  validate_coords( coords, data.EXPECTED_PRIME_GROUND_RESOURCES )
end

function Test.prime_forest_resources()
  local offset = 1 -- arbitrary
  local coords = dist.compute_prime_forest_resources( MAP_SIZE,
                                                      offset )
  validate_coords( coords, data.EXPECTED_PRIME_FOREST_RESOURCES )
end

return Test
