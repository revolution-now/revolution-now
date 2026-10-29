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
-- lost city rumors. It matches the output of the original game's
-- algorithm precisely. This algorithm was actually inherited by
-- the OG from the Civilization 1 game which also used it to
-- place its goody huts and resources. It was likely chosen to
-- provide even a sufficiently random but even distribution in a
-- way that can be computed very efficiently given a map square.
-- The latter is important because the original game does not
-- store or precompute the locations of these resources; it needs
-- to recompute them on each query of a tile.
local function hashed_lattice_recurring_placement( config, coord )
  local x = coord.x + 1
  local y = coord.y + 1
  local cell = assert( config.cell )
  local cell_x = x // cell
  local cell_y = y // cell
  local cell_idx_x = x % cell
  local cell_idx_y = y % cell
  local hash = config.seed + cell_x * 17 + cell_y * 19 +
                   config.shift
  -- This needs to be non-negative because our modulo won't re-
  -- ally work right if it is negative, and in the OG it is not
  -- supposed to be negative.
  assert( hash >= 0 )
  hash = hash % assert( config.cycle )
  local tile_idx_1 = cell_idx_x * cell + cell_idx_y
  local tile_idx_2 = nil
  if config.extra then tile_idx_2 = tile_idx_1 ~ 10 end
  return tile_idx_1 == hash or tile_idx_2 == hash
end

local function distribute( config )
  local res = {}
  for y = 0, config.world_size.h - 1 do
    for x = 0, config.world_size.w - 1 do
      local coord = { x=x, y=y }
      if hashed_lattice_recurring_placement( config, coord ) then
        table.insert( res, coord )
      end
    end
  end
  return res
end

function M.compute_lost_city_rumors( world_size, seed )
  return distribute{
    world_size=world_size,
    seed=seed,
    cell=4,
    cycle=32,
    shift=8,
    extra=false,
  }
end

local function compute_prime_resources( world_size, seed, shift )
  return distribute{
    world_size=world_size,
    seed=seed,
    cell=4,
    cycle=16,
    shift=shift,
    extra=true,
  }
end

function M.compute_prime_forest_resources( world_size, seed )
  -- The OG shifts the ground and forest resources relative to
  -- each other so that they don't overlap. A shift of -1 here is
  -- a shift to the right of 1 cell (4 tiles).
  local shift = -1
  return compute_prime_resources( world_size, seed, shift )
end

function M.compute_prime_ground_resources( world_size, seed )
  local shift = 0
  return compute_prime_resources( world_size, seed, shift )
end

return M
