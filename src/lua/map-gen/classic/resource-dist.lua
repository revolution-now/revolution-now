--[[ ------------------------------------------------------------
|
| resource-dist.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2022-05-13.
|
| Description: Resource distribution algorithms mirroring those
|              from the original game.
|
--]] ------------------------------------------------------------
local M = {}

-- This function, together with the appropriate config numbers,
-- represents the object distribution algorithm used by the orig-
-- inal game to distribute land resources, forest resources, and
-- lost city rumors. The numbers chosen in the config (which must
-- be precisely selected to replicate the patterns in the orig-
-- inal game) were likely chosen to provide even distribution
-- (i.e., without the clustering that is characteristic of a
-- purely random approach) but with some level of randomness in a
-- way that can be computed very efficiently given a map square.
-- The efficiency is likely important because the original game
-- does not seem to store or precompute the locations of these
-- resources, since there are no bits in the map save files for
-- them, and so it probably computes them on the fly whenever it
-- needs to know what is on a square.
local function OG_distribution_algo( config, coord )
  local ROTATION = 12
  local C = config
  local X = coord.x + C.x_offset
  local Y = coord.y + C.y_offset
  -- WARNING: this Lua modulo is not the same as the C/C++ modulo
  -- when negative numbers are involved.
  local idx = (Y - C.seed * 20) % C.board_size
  assert( #C.shifts == 4 )
  local rotations = ((idx // 4) + C.shifts[idx % 4 + 1]) %
                        (C.board_size // 4)
  local column = X + ROTATION * rotations
  return C.resources[column % C.board_size] ~= nil
end

local function distribute( config )
  local res = {}
  for y = 0, config.world_size.h - 1 do
    for x = 0, config.world_size.w - 1 do
      local coord = { x=x, y=y }
      if OG_distribution_algo( config, coord ) then
        table.insert( res, coord )
      end
    end
  end
  return res
end

-- This algorithm matches the original game's precisely. It takes
-- the world size (in tiles) as a parameter and the one-byte seed
-- used by the OG, which must be an integer.
function M.compute_lost_city_rumors( world_size, seed )
  return distribute{
    world_size=world_size,
    board_size=128,
    seed=seed,
    x_offset=5,
    y_offset=53,
    shifts={ 0, 5, 10, 15 },
    resources={
      -- LuaFormatter off
      [17*0]=1,
      [17*1]=1,
      [17*2]=1,
      [17*3]=1,
      -- LuaFormatter on
    },
  }
end

-- This algorithm matches the original game's precisely. It takes
-- the world size (in tiles) as a parameter, the one-byte seed
-- used by the OG (must be an integer), and a horizontal shift.
local function compute_prime_resources(world_size, seed, x_offset )
  return distribute{
    world_size=world_size,
    board_size=64,
    seed=seed,
    x_offset=x_offset,
    y_offset=10,
    shifts={ 0, 4, 9, 12 },
    resources={
      -- LuaFormatter off
      [0 + 17*0    ]=1,
      [0 + 17*1    ]=1,
      [0 + 17*2    ]=1,
      [0 + 17*3 - 4]=1,
      [7 + 17*0    ]=1,
      [7 + 17*1    ]=1,
      [7 + 17*2    ]=1,
      [7 + 17*3    ]=1,
      -- LuaFormatter on
    },
  }
end

function M.compute_prime_ground_resources( world_size, seed )
  local x_offset = 0
  return compute_prime_resources( world_size, seed, x_offset )
end

function M.compute_prime_forest_resources( world_size, seed )
  -- This offset of 4 (used by the OG) is chosen so that the
  -- forest resources never fall on the same squares as the
  -- non-forested resources.
  local x_offset = -4
  return compute_prime_resources( world_size, seed, x_offset )
end

return M
