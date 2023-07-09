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
local tables = require( 'util.tables' )
local U = require( 'test.unit' )

-----------------------------------------------------------------
-- Freeze global access.
-----------------------------------------------------------------
-- Declare all globals used.
local pairs = pairs

-- No reading or writing of globals from here on.
local _ENV = nil

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local ASSERT_EQ = U.ASSERT_EQ
local ASSERT_EQ_APPROX = U.ASSERT_EQ_APPROX

-----------------------------------------------------------------
-- Test setup.
-----------------------------------------------------------------
local Test = U.new_test_pack()

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function validate_weights( weights, expected )
  ASSERT_EQ( tables.table_size( weights ),
             tables.table_size( expected ),
             'lengths of result and expected result' )
  for type, weight in pairs( weights ) do
    ASSERT_EQ_APPROX( weight, expected[type],
                      'weights for type ' .. type )
  end
end

-----------------------------------------------------------------
-- Test cases.
-----------------------------------------------------------------
function Test.test_dry_weights()
  local weights, expected

  weights = W.dry_weights_for_row( 100, 0 )
  expected = { grassland=.03, plains=.35, prairie=.25,
               tundra=.37 }
  validate_weights( weights, expected )

  weights = W.dry_weights_for_row( 100, 99 )
  expected = { grassland=.03, plains=.35, prairie=.25,
               tundra=.37 }
  validate_weights( weights, expected )

  weights = W.dry_weights_for_row( 100, 49 )
  expected = {
    grassland=.20,
    plains=.05,
    prairie=.15,
    savannah=.45,
    desert=.15,
  }
  validate_weights( weights, expected )

  weights = W.dry_weights_for_row( 100, 10 )
  expected = { grassland=.04, plains=.35, prairie=.30,
               tundra=.31 }
  validate_weights( weights, expected )

  weights = W.dry_weights_for_row( 100, 25 )
  expected = {
    desert=.15,
    grassland=.15,
    plains=.30,
    prairie=.30,
    savannah=.00,
    tundra=.10,
  }
  validate_weights( weights, expected )

  weights = W.dry_weights_for_row( 100, 24 )
  expected = {
    desert=.135,
    grassland=.14,
    plains=.305,
    prairie=.305,
    tundra=.115,
  }
  validate_weights( weights, expected )

  weights = W.dry_weights_for_row( 100, 40 )
  expected = {
    desert=.175,
    grassland=.20,
    plains=.075,
    prairie=.20,
    savannah=.30,
    tundra=.05,
  }
  validate_weights( weights, expected )
end

function Test.test_wet_weights()
  local weights, expected

  weights = W.wet_weights_for_row( 100, 0 )
  expected = { none=1.0, marsh=0.0, swamp=0.0 }
  validate_weights( weights, expected )

  weights = W.wet_weights_for_row( 100, 99 )
  expected = { none=1.0, marsh=0.0, swamp=0.0 }
  validate_weights( weights, expected )

  weights = W.wet_weights_for_row( 100, 49 )
  expected = { none=.7, marsh=.15, swamp=.15 }
  validate_weights( weights, expected )

  weights = W.wet_weights_for_row( 100, 10 )
  expected = { none=1.0, marsh=0.0, swamp=0.0 }
  validate_weights( weights, expected )

  weights = W.wet_weights_for_row( 100, 20 )
  expected = { none=.95, marsh=.025, swamp=.025 }
  validate_weights( weights, expected )

  weights = W.wet_weights_for_row( 100, 30 )
  expected = { none=.85, marsh=.075, swamp=.075 }
  validate_weights( weights, expected )
end

return Test
