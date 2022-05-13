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

function Test.lost_city_rumors()
  local size = { w=56, h=70 }
  local const_offset = 109
  local coords = dist.compute_lost_city_rumors( size,
                                                const_offset )
  local expected = data.EXPECTED_LOST_CITY_RUMORS
  ASSERT_EQ( #coords, #expected,
             'lengths of result and expected result' )
  for i = 1, #coords do
    ASSERT_EQ( coords[i].x, expected[i].x,
               'x coordinates at index ' .. i )
    ASSERT_EQ( coords[i].y, expected[i].y,
               'y coordinates at index ' .. i )
  end
end

function Test.prime_ground_resources()
  local size = { w=56, h=70 }
  local const_offset = 1
  local coords = dist.compute_prime_ground_resources( size,
                                                      const_offset )
  local expected = data.EXPECTED_PRIME_GROUND_RESOURCES
  ASSERT_EQ( #coords, #expected,
             'lengths of result and expected result' )
  for i = 1, #coords do
    ASSERT_EQ( coords[i].x, expected[i].x,
               'x coordinates at index ' .. i )
    ASSERT_EQ( coords[i].y, expected[i].y,
               'y coordinates at index ' .. i )
  end
end

return Test
