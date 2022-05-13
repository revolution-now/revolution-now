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
local function col1_distribution_algo( config, coord )
  local ROTATION = 12
  -- Note: this modulus seems to be optional.
  local idx = coord.y + config.y_offset % config.board_size
  -- Note: this modulus seems to be optional.
  local rotations = ((idx // 4) + config.shifts[idx % 4 + 1]) %
                        (config.board_size // 4)
  local column = coord.x - config.x_offset + ROTATION * rotations
  return config.resources[column % config.board_size] ~= nil
end

local function distribute( config )
  local res = {}
  for y = 0, config.world_size.h - 1 do
    for x = 0, config.world_size.w - 1 do
      local coord = { x=x, y=y }
      if col1_distribution_algo( config, coord ) then
        table.insert( res, coord )
      end
    end
  end
  return res
end

-- This algorithm matches the original game's precisely. It takes
-- the world size (in tiles) as a parameter and a random offset.
function M.compute_lost_city_rumors( size, y_offset )
  local config = {
    world_size=size,
    shifts={ 0, 5, 10, 15 },
    resources={
      [17 * 0]=1, --
      [17 * 1]=1, --
      [17 * 2]=1, --
      [17 * 3]=1 --
    },
    board_size=128,
    x_offset=-5,
    y_offset=y_offset
  }
  return distribute( config )
end

-- This algorithm matches the original game's precisely. It takes
-- the world size (in tiles) as a parameter and a random offset.
local function compute_prime_resources( size, y_offset, x_offset )
  local config = {
    world_size=size,
    shifts={ 0, 4, 9, 12 },
    resources={
      [0 + 17 * 0]=1, --
      [0 + 17 * 1]=1, --
      [0 + 17 * 2]=1, --
      [0 + 17 * 3 - 4]=1, --
      [7 + 17 * 0]=1, --
      [7 + 17 * 1]=1, --
      [7 + 17 * 2]=1, --
      [7 + 17 * 3]=1 --
    },
    board_size=64,
    x_offset=x_offset,
    y_offset=y_offset
  }
  return distribute( config )
end

function M.compute_prime_ground_resources( size, y_offset )
  local x_offset = 0
  return compute_prime_resources( size, y_offset, x_offset )
end

function M.compute_prime_forest_resources( size, y_offset )
  -- This offset of 4 is chosen so that the forest resources
  -- never fall on the same squares as the non-forested re-
  -- sources.
  local x_offset = 4
  return compute_prime_resources( size, y_offset, x_offset )
end

return M
