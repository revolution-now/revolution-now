--[[ ------------------------------------------------------------
|
| terrain-weights-test.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2022-10-10.
|
| Description:  Unit tests for terrain ground type weights.
|
--]] ------------------------------------------------------------
local W = require( 'map-gen.classic.terrain-weights' )

local U = require( 'test.unit' )
local ASSERT_EQ = U.ASSERT_EQ

local Test = U.new_test_pack()

local function validate_weights( weights, expected )
  ASSERT_EQ( #weights, #expected,
             'lengths of result and expected result' )
  for i = 1, #weights do
    ASSERT_EQ( weights[i].type, expected[i].type,
               'types at index ' .. i )
    ASSERT_EQ( weights[i].value, expected[i].value,
               'values at index ' .. i )
  end
end

function Test.test_dry_weights()
  -- ASSERT_EQ( 3, 4, '3 and 4' ) --
end

function Test.test_wet_weights()
  -- ASSERT_EQ( 3, 4, '3 and 4' ) --
end

function Test.test_select_from_weights()
  -- ASSERT_EQ( 3, 4, '3 and 4' ) --
end

return Test
