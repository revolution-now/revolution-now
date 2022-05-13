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

local function populate_coords_with( f, size )
  local res = {}
  for y = 0, size.h - 1 do
    for x = 0, size.w - 1 do
      local coord = { x=x, y=y }
      if f( coord ) then table.insert( res, coord ) end
    end
  end
  return res
end

-- This algorithm matches the original game's precisely. It takes
-- the world size (in tiles) as a parameter and a random offset.
function M.compute_lost_city_rumors( size, offset )
  local shifts = { 0, 5, 10, 15 }
  local resources = {
    [0 + 17 * 0]=true,
    [0 + 17 * 1]=true,
    [0 + 17 * 2]=true,
    [0 + 17 * 3]=true
  }
  local board = 128
  local const_shift = -5
  local ROTATION = 12

  local has_rumor = function( coord )
    local idx = coord.y + offset -- % board
    local rotations = ((idx // 4) + shifts[idx % 4 + 1]) % 32
    local column = coord.x - const_shift + ROTATION * rotations
    return resources[column % board] ~= nil
  end

  return populate_coords_with( has_rumor, size )
end

-- This algorithm matches the original game's precisely. It takes
-- the world size (in tiles) as a parameter and a random offset.
function M.compute_prime_ground_resources( size, offset )
  local shifts = { 0, 4, 9, 12 }
  local resources = {
    [0 + 17 * 0]=true,
    [7 + 17 * 0]=true,
    [0 + 17 * 1]=true,
    [7 + 17 * 1]=true,
    [0 + 17 * 2]=true,
    [7 + 17 * 2]=true,
    [0 + 17 * 3 - 4]=true,
    [7 + 17 * 3]=true
  }
  local board = 64
  local const_shift = 0
  local ROTATION = 12

  local has_resource = function( coord )
    local idx = coord.y + offset -- % board
    local rotations = ((idx // 4) + shifts[idx % 4 + 1]) % 16
    local column = coord.x - const_shift + ROTATION * rotations
    return resources[column % board] ~= nil
  end

  return populate_coords_with( has_resource, size )
end

return M
