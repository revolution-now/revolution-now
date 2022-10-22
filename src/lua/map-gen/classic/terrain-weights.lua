--[[ ------------------------------------------------------------
|
| terrain-weights.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2022-10-10.
|
| Description: Ground terrain weights for the classic map gen.
|
--]] ------------------------------------------------------------
local M = {}

-----------------------------------------------------------------
-- Imports
-----------------------------------------------------------------
local limits = require( 'util.limits' )
local freeze = require( 'util.freeze' )

-----------------------------------------------------------------
-- Aliases/Globals
-----------------------------------------------------------------
local _ENV = freeze.globals( _ENV )

local format = assert( string.format )
local floor = assert( math.floor )

-----------------------------------------------------------------
-- Weights
-----------------------------------------------------------------
-- Here we are attempting to model the OG's map gen algorithm
-- with respect to selecting ground terrain types as a function
-- of location on the map. It is not clear exactly how the OG
-- does this, but we will model it with the following.
--
-- Each ground terrain type has a weight that depends on which
-- 10% horizontal slice of the world we are in. The world is sym-
-- metric about the equator, so there are really only five dis-
-- tinct slices that we need to define.
--
--                   +------------------------+
--                   |       segment 0        |
--                   +------------------------+
--                   |       segment 1        |
--                   +------------------------+
--                   |       segment 2        |
--                   +------------------------+
--                   |       segment 3        |
--                   +------------------------+
--                   |       segment 4        |
--                   +------------------------+
--                   |       segment 4        |
--                   +------------------------+
--                   |       segment 3        |
--                   +------------------------+
--                   |       segment 2        |
--                   +------------------------+
--                   |       segment 1        |
--                   +------------------------+
--                   |       segment 0        |
--                   +------------------------+
--
-- The weights in each segment must add to 100.
--
-- First we have the "dry weights," which are the weights for all
-- terrain types apart from swamp and marsh, which need to go
-- next to water. The first pass will assign each land tile one
-- of the dry ground types. Then two subsequent pass will assign
-- the swamp/marsh to eligible tiles, overwriting whatever was
-- there previously. This three-pass approach is done because 1)
-- the wet types need to go near water, and two wet types can
-- also go next to other wet types even if they are not immedi-
-- ately next to an ocean or river. And so if we were to make a
-- single pass we wouldn't be able to capture (2) in a symmetric
-- way, since we can traverse the map in only one direction. Also
-- this multi-pass approach eliminates the need to have different
-- sets of weights for tiles that are eligible for wet types and
-- those that are not.
--
-- LuaFormatter off
local dry_weights = {
  [0] = {
    grassland =  3,
    plains    =  35,
    prairie   =  25,
    tundra    =  37,
  },

  [1] = {
    grassland =  5,
    plains    =  35,
    prairie   =  35,
    tundra    =  25,
  },

  [2] = {
    desert    =  15,
    grassland =  15,
    plains    =  30,
    prairie   =  30,
    tundra    =  10,
  },

  [3] = {
    desert    =  20,
    grassland =  20,
    plains    =  10,
    prairie   =  25,
    savannah  =  15,
    tundra    =  10,
  },

  [4] = {
    desert    =  15,
    grassland =  20,
    plains    =   5,
    prairie   =  15,
    savannah  =  45,
  },
}
-- LuaFormatter on

-- LuaFormatter off
local wet_weights = {
  [0] = {
    none  = 100,
    marsh =   0,
    swamp =   0,
  },

  [1] = {
    none  = 100,
    marsh =   0,
    swamp =   0,
  },

  [2] = {
    none  =  90,
    marsh =   5,
    swamp =   5,
  },

  [3] = {
    none  =  80,
    marsh =  10,
    swamp =  10,
  },

  [4] = {
    none  =  70,
    marsh =  15,
    swamp =  15,
  },
}
-- LuaFormatter on

local function check_weights( weights )
  for i = 0, 4 do
    local sum = 0
    local group = dry_weights[i]
    for _, v in pairs( group ) do sum = sum + v end
    assert( sum == 100, format(
                'weights for segment %d do not sum to 100.', i ) )
  end
end

check_weights( dry_weights )
check_weights( wet_weights )

local function weights_for_row( map_height, weights, row )
  if map_height % 2 == 1 and row == map_height // 2 then
    -- This happens if we have an odd map height and the row in
    -- questing is the center row, which will cause the code fur-
    -- ther below to not work right, so we'll just use the same
    -- weights as for the previous row.
    row = row - 1
  end
  if row >= map_height // 2 then row = (map_height - row - 1) end
  assert( row < map_height // 2, format(
              '%d is not less than %d.', row, map_height // 2 ) )
  local fractional_slice = (row / (map_height // 2)) * 5.0
  assert( fractional_slice < 5.0 )
  local slice_idx = floor( fractional_slice )
  local fractional_part = fractional_slice - slice_idx
  local adjacent_slice_idx
  if fractional_part < .5 then
    adjacent_slice_idx = slice_idx - 1
  else
    adjacent_slice_idx = slice_idx + 1
  end
  adjacent_slice_idx = limits.clamp( adjacent_slice_idx, 0, 4 )
  assert( adjacent_slice_idx >= 0 )
  assert( adjacent_slice_idx <= 4 )
  local unique_types = {}
  for type, _ in pairs( weights[slice_idx] ) do
    unique_types[type] = true
  end
  for type, _ in pairs( weights[adjacent_slice_idx] ) do
    unique_types[type] = true
  end
  local total = 0
  -- Weight the two slices according to how close the row is to
  -- each of them. If the row is in the center of a slice, then
  -- that slice will get nearly all of the weight. The closer it
  -- moves to the border with the adjacent slice, the more weight
  -- that will get. If it is right on the border then the two
  -- slices each get 0.5.
  local weight1 = 1.0 - math.abs( fractional_part - .5 )
  local weight2 = 1.0 - weight1
  assert( weight1 >= 0.0 )
  assert( weight2 >= 0.0 )
  assert( weight1 <= 1.0 )
  assert( weight2 <= 1.0 )
  local linear_combo_weights = {}
  for type, _ in pairs( unique_types ) do
    local slice1 = weights[slice_idx][type] or 0
    local slice2 = weights[adjacent_slice_idx][type] or 0
    linear_combo_weights[type] =
        weight1 * slice1 + weight2 * slice2
    total = total + linear_combo_weights[type]
  end
  -- Normalize the weights so that they sum to 1.
  for type, weight in pairs( linear_combo_weights ) do
    linear_combo_weights[type] =
        linear_combo_weights[type] / total
  end
  return linear_combo_weights
end

function M.dry_weights_for_row( map_height, row )
  return weights_for_row( map_height, dry_weights, row )
end

function M.wet_weights_for_row( map_height, row )
  return weights_for_row( map_height, wet_weights, row )
end

-- The weights need to add up to 1.0.
function M.select_from_weights( weights )
  local cut = math.random()
  local total = 0.0
  for type, weight in pairs( weights ) do
    total = total + weight
    if total > cut then return type end
  end
  error( 'should not be here.' )
end

return M
