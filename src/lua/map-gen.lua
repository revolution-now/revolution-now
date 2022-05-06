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

local WORLD_SIZE = { w=58, h=72 }

local function foo()
  -- local function.
end

local function set_land( coord )
  local square = map_gen.at( coord )
  square.surface = e.surface.land
  square.ground = e.ground_terrain.plains
end

local function set_water( coord )
  local square = map_gen.at( coord )
  square.surface = e.surface.water
  square.ground = e.ground_terrain.arctic
  square.sea_lane = false
end

local function random_point_in_rect( rect )
  local size = { w=rect.w, h=rect.h }
  local x = math.random( 0, rect.w - 1 ) + rect.x
  local y = math.random( 0, rect.h - 1 ) + rect.y
  return { x=x, y=y }
end

local function random_square()
  local size = map_gen.world_size()
  return random_point_in_rect( { x=0, y=0, w=size.w, h=size.h } )
end

local function random_direction()
  local x = math.random( 0, 1 )
  local y = math.random( 0, 1 )
  return { x=x, y=y }
end

local function reset_terrain()
  map_gen.reset_terrain( WORLD_SIZE )
  local size = map_gen.world_size()
  for y = 0, size.h - 1 do
    for x = 0, size.w - 1 do --
      set_water{ x=x, y=y }
    end
  end
end

local function square_key( square )
  return square.y * 10000 + square.x
end

local function append( tbl, elem ) tbl[#tbl + 1] = elem end

local function random_elem( tbl, len )
  local idx = math.random( 1, len )
  local j = 1
  for key, elem in pairs( tbl ) do
    if j == idx then return key, elem end
    j = j + 1
  end
end

local function surrounding_squares( square )
  local possible = {
    { x=square.x - 1, y=square.y - 1 }, --
    { x=square.x + 0, y=square.y - 1 }, --
    { x=square.x + 1, y=square.y - 1 }, --
    { x=square.x - 1, y=square.y + 0 }, --
    { x=square.x + 1, y=square.y + 0 }, --
    { x=square.x - 1, y=square.y + 1 }, --
    { x=square.x + 0, y=square.y + 1 }, --
    { x=square.x + 1, y=square.y + 1 } --
  }
  local exists = {}
  local size = map_gen.world_size()
  local function square_exists( square )
    return
        square.x >= 0 and square.y >= 0 and square.x < size.w and
            square.y < size.h
  end
  for _, val in ipairs( possible ) do
    if square_exists( val ) then append( exists, val ) end
  end
  return exists
end

local function generate_continent( seed_square, area )
  local square = seed_square
  local border_squares = {}
  border_squares_len = 0
  set_land( square )
  for i = 1, area - 1 do
    local surrounding = surrounding_squares( square )
    for _, s in ipairs( surrounding ) do
      if map_gen.at( s ).surface == e.surface.water then
        local key = square_key( s )
        if border_squares[key] == nil then
          border_squares[key] = s
          border_squares_len = border_squares_len + 1
        end
      end
    end
    if border_squares_len == 0 then
      -- We've run out of space to grow.  This can happen
      -- e.g. if we started inside an enclosed lake inside
      -- another continent.
      return
    end
    key = random_elem( border_squares, border_squares_len )
    square = border_squares[key]
    border_squares[key] = nil
    border_squares_len = border_squares_len - 1
    set_land( square )
  end
end

local function forest_cover()
  local size = map_gen.world_size()
  for y = 0, size.h - 1 do
    for x = 0, size.w - 1 do
      local square = map_gen.at( { x=x, y=y } )
      if square.surface == e.surface.land then
        if math.random( 1, 4 ) <= 3 then
          square.overlay = e.land_overlay.forest
        end
      end
    end
  end
end

function M.generate()
  reset_terrain()
  local size = map_gen.world_size()
  local buffer = 10
  for i = 1, 3 do
    local square = random_point_in_rect(
                       {
          x=buffer,
          y=buffer,
          w=size.w - buffer * 2,
          h=size.h - buffer * 2
        } )
    local area = math.random( 10, 300 )
    generate_continent( square, area )
  end
  forest_cover()
end

return M
