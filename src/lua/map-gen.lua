--[[ ------------------------------------------------------------
|
| map-gen.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2022-04-25.
|
| Description: Some routines for random map generation.
|
--]] ------------------------------------------------------------
local M = {}

local function foo()
  -- local function.
end

local function set_land( coord )
  local square = map_gen.at( coord )
  square.surface = e.surface.land
  square.ground = e.ground_terrain.grassland
end

local function set_water( coord )
  local square = map_gen.at( coord )
  square.surface = e.surface.water
  square.ground = e.ground_terrain.arctic
  square.sea_lane = false
end

local function random_square()
  local size = map_gen.world_size()
  local x = math.random( 0, size.w - 1 )
  local y = math.random( 0, size.h - 1 )
  return { x=x, y=y }
end

local function reset_terrain()
  local size = map_gen.world_size()
  for y = 0, size.h - 1 do
    for x = 0, size.w - 1 do --
      set_water{ x=x, y=y }
    end
  end
end

function M.generate()
  reset_terrain()
  set_land( random_square() )
end

return M
