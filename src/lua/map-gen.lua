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

-----------------------------------------------------------------
-- Constants
-----------------------------------------------------------------
local WORLD_SIZE = { w=58, h=72 }

-----------------------------------------------------------------
-- Utils
-----------------------------------------------------------------
local function debug_log( msg )
  -- io.write( msg )
end

-- Enforces that n is in [min, max].
local function clamp( n, min, max )
  if n < min then return min end
  if n > max then return max end
  return n
end

local function append( tbl, elem ) tbl[#tbl + 1] = elem end

-----------------------------------------------------------------
-- Random Numbers
-----------------------------------------------------------------
local function random_elem( tbl, len )
  local idx = math.random( 1, len )
  local j = 1
  for key, elem in pairs( tbl ) do
    if j == idx then return key, elem end
    j = j + 1
  end
end

local function random_bool()
  if math.random( 1, 2 ) == 1 then
    return true
  else
    return false
  end
end

local function random_list_elem( lst )
  return lst[math.random( 1, #lst )]
end

local function random_point_in_rect( rect )
  local size = { w=rect.w, h=rect.h }
  local x = math.random( 0, rect.w - 1 ) + rect.x
  local y = math.random( 0, rect.h - 1 ) + rect.y
  return { x=x, y=y }
end

local function random_direction()
  local x = math.random( 0, 1 )
  local y = math.random( 0, 1 )
  return { x=x, y=y }
end

-----------------------------------------------------------------
-- Coordinate Map
-----------------------------------------------------------------
local function square_key( square )
  return square.y * 10000 + square.x
end

-----------------------------------------------------------------
-- Algorithms
-----------------------------------------------------------------
-- This will call the function on each square of the map, passing
-- in the coordinate and the square object which the function may
-- use. Note that the coordinates are zero based.
local function on_all( f )
  local size = map_gen.world_size()
  for y = 0, size.h - 1 do
    for x = 0, size.w - 1 do --
      local coord = { x=x, y=y }
      local square = map_gen.at( coord )
      f( coord, square )
    end
  end
end

-----------------------------------------------------------------
-- Unit Placement
-----------------------------------------------------------------
function M.initial_ship_pos()
  local size = map_gen.world_size()
  return { y=size.h / 2, x=size.w - 3 }
end

-----------------------------------------------------------------
-- Terrain Modification
-----------------------------------------------------------------
local function set_land( coord )
  local square = map_gen.at( coord )
  square.surface = e.surface.land
  square.ground = random_list_elem{
    e.ground_terrain.plains, e.ground_terrain.grassland,
    e.ground_terrain.prairie, e.ground_terrain.marsh
  }
end

local function set_water( coord )
  local square = map_gen.at( coord )
  square.surface = e.surface.water
  square.ground = e.ground_terrain.arctic
  square.sea_lane = false
end

local function is_square_water( square )
  return square.surface == e.surface.water
end

local function set_arctic( coord )
  local square = map_gen.at( coord )
  square.surface = e.surface.land
  square.ground = e.ground_terrain.arctic
end

local function is_sea_lane( coord )
  local square = map_gen.at( coord )
  return square.sea_lane
end

local function set_sea_lane( coord )
  set_water( coord )
  local square = map_gen.at( coord )
  square.sea_lane = true
end

-- This will create a new empty map set all squares to water.
local function reset_terrain()
  map_gen.reset_terrain( WORLD_SIZE )
  on_all( function( coord ) set_water( coord ) end )
end

-----------------------------------------------------------------
-- Square Surroundings
-----------------------------------------------------------------
local function surrounding_squares_7x7( square )
  local possible = {}
  for y = square.y - 3, square.y + 3 do
    for x = square.x - 3, square.x + 3 do
      if x ~= square.x or y ~= square.y then
        append( possible, { x=x, y=y } )
      end
    end
  end
  return possible
end

-- This will give the tiles along the right edge of the 7x7 block
-- of tiles centered on `square`.
local function surrounding_squares_7x7_right_edge( square )
  local possible = {}
  for y = square.y - 3, square.y + 3 do
    append( possible, { x=square.x + 3, y=y } )
  end
  return possible
end

local function surrounding_squares_diagonal( square )
  local possible = {
    { x=square.x - 1, y=square.y - 1 }, --
    { x=square.x + 1, y=square.y - 1 }, --
    { x=square.x - 1, y=square.y + 1 }, --
    { x=square.x + 1, y=square.y + 1 } --
  }
  return possible
end

local function surrounding_squares_cardinal( square )
  local possible = {
    { x=square.x + 0, y=square.y - 1 }, --
    { x=square.x - 1, y=square.y + 0 }, --
    { x=square.x + 1, y=square.y + 0 }, --
    { x=square.x + 0, y=square.y + 1 } --
  }
  return possible
end

local function filter_existing_squares( squares )
  local exists = {}
  local size = map_gen.world_size()
  local function square_exists( square )
    return
        square.x >= 0 and square.y >= 0 and square.x < size.w and
            square.y < size.h
  end
  for _, val in ipairs( squares ) do
    if square_exists( val ) then append( exists, val ) end
  end
  return exists
end

-----------------------------------------------------------------
-- Continent Generation
-----------------------------------------------------------------
local function generate_continent( seed_square, area )
  local square = seed_square
  local border_squares = {}
  border_squares_len = 0
  set_land( square )
  for i = 1, area - 1 do
    local surrounding_n
    if math.random( 1, 3 ) > 1 then
      surrounding_n = surrounding_squares_cardinal
    else
      surrounding_n = surrounding_squares_diagonal
    end
    local surrounding = filter_existing_squares(
                            surrounding_n( square ) )
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

-----------------------------------------------------------------
-- Forest Generation
-----------------------------------------------------------------
local function forest_cover()
  local size = map_gen.world_size()
  on_all( function( coord )
    local square = map_gen.at( coord )
    if square.surface == e.surface.land then
      if square.ground ~= e.ground_terrain.arctic then
        if math.random( 1, 4 ) <= 3 then
          square.overlay = e.land_overlay.forest
        end
      end
    end
  end )
end

-----------------------------------------------------------------
-- Map Edges
-----------------------------------------------------------------
-- Will clear a frame around the edge of the map to make sure
-- that land doesn't get too close to the map edge and we still
-- have room for sea lane squares.
local function clear_buffer_area( buffer_size )
  local size = map_gen.world_size()
  on_all( function( coord )
    local y = coord.y
    local x = coord.x
    if y < buffer_size or y > size.h - buffer_size or x <
        buffer_size or x > size.w - buffer_size then
      set_water{ x=x, y=y }
      -- set_sea_lane{ x=x, y=y }
    end
  end )
end

local function create_arctic_along_row( y )
  local size = map_gen.world_size()
  -- Note that we don't include the edges.
  for x = 1, size.w - 2 do
    if math.random( 1, 2 ) == 1 then set_arctic{ x=x, y=y } end
  end
end

local function create_arctic()
  local size = map_gen.world_size()
  create_arctic_along_row( 0 )
  create_arctic_along_row( size.h - 1 )
end

-----------------------------------------------------------------
-- Sea Lane Generation
-----------------------------------------------------------------
local function create_sea_lanes()
  local size = map_gen.world_size()

  -- First set all water tiles to sea lane.
  on_all( function( coord, square )
    if is_square_water( square ) then square.sea_lane = true end
  end )

  -- Now find all land squares and make sure that there are no
  -- sea lane squares in their vicinity (7x7 square). And for
  -- each of those water squares, clear all sea lane squares
  -- along the entire row to the left of it until the map edge.
  on_all( function( coord )
    local square = map_gen.at( coord )
    if square.surface == e.surface.land then
      local block_edge = surrounding_squares_7x7_right_edge(
                             coord )
      block_edge = filter_existing_squares( block_edge )
      for _, s in ipairs( block_edge ) do
        for x = 0, s.x do
          local coord = { x=x, y=s.y }
          local square = map_gen.at( coord )
          if square.sea_lane then set_water( coord ) end
        end
      end
    end
  end )

  -- Clear out any sea lane along the three rows at the top of
  -- the map and the bottom of the map (not including the arctic
  -- rows).
  for y = 0, 3 do
    for x = 0, size.w - 1 do
      local coord = { x=x, y=y }
      local square = map_gen.at( coord )
      if square.sea_lane then set_water( coord ) end
    end
  end
  for y = size.h - 4, size.h - 1 do
    for x = 0, size.w - 1 do
      local coord = { x=x, y=y }
      local square = map_gen.at( coord )
      if square.sea_lane then set_water( coord ) end
    end
  end

  -- At this point, some rows (that contain no land tiles) will
  -- be all sea lane. So we will start at the center of the map
  -- and move upward (downward) to find them and we will set
  -- their sea lane width (i.e., the width on the right side of
  -- the map) to what it was below (above) that row.
  --
  -- Run through all rows and find the row that is not entirely
  -- sea lane that is closest to the center of the map.
  local closest_row = 0 -- row zero has been cleared of sea lane.
  local y_mid = size.h / 2
  for y = 0, size.h - 1 do
    if not is_sea_lane{ x=0, y=y } then
      if math.abs( y - y_mid ) < math.abs( closest_row - y_mid ) then
        -- We've found a non-sea-lane row that is closer to the
        -- middle then what we've found so far.
        closest_row = y
      end
    end
  end
  debug_log( 'starting row: ' .. tostring( closest_row ) .. '\n' )
  -- Now get the sea lane width where we are starting.
  local sea_lane_width = function( y )
    local width = 0
    for x = size.w - 1, 0, -1 do
      if not is_sea_lane{ x=x, y=y } then break end
      width = width + 1
    end
    return width
  end
  local curr_sea_lane_width = sea_lane_width( closest_row )
  debug_log( 'curr width: ' .. tostring( curr_sea_lane_width ) ..
                 '\n' )
  -- Now start at the row that we found and go upward.
  for y = closest_row - 1, 0, -1 do
    if not is_sea_lane{ x=0, y=y } then
      curr_sea_lane_width = sea_lane_width( y )
    else
      -- Clear the sea lane and make it have the width of the row
      -- below it.
      for x = 0, size.w - 1 - curr_sea_lane_width do
        map_gen.at{ x=x, y=y }.sea_lane = false
      end
    end
  end
  -- Now start at the row that we found and go downward.
  for y = closest_row + 1, size.h - 1 do
    if not is_sea_lane{ x=0, y=y } then
      curr_sea_lane_width = sea_lane_width( y )
    else
      -- Clear the sea lane and make it have the width of the row
      -- below it.
      for x = 0, size.w - 1 - curr_sea_lane_width do
        map_gen.at{ x=x, y=y }.sea_lane = false
      end
    end
  end

  -- Finally put one tile of sea lane on the left edge and one on
  -- the right edge (it will be missing on the left edge, and may
  -- be missing on the right edge at this point).
  for y = 0, size.h - 1 do set_sea_lane{ x=0, y=y } end
  for y = 0, size.h - 1 do set_sea_lane{ x=size.w - 1, y=y } end
end

-----------------------------------------------------------------
-- Resource Generation
-----------------------------------------------------------------
local function distribute_prime_ground_resources()
  local shifts = { 0, 4, 9, 12 }
  local resources = {
    [0]=true,
    [7]=true,
    [17]=true,
    [24]=true,
    [34]=true,
    [41]=true,
    [47]=true,
    [58]=true
  }
  local const_offset = 0

  local has_resource = function( coord )
    local y = coord.y + 1
    local x = coord.x + 1
    local idx = (y + const_offset) % 64
    local shift = shifts[idx % 4 + 1]
    local lookup = (idx // 4 + shift) % 16
    local rotation = 12 * lookup
    local resource_idx = (x + rotation) % 64
    return resources[resource_idx] ~= nil
  end

  on_all( function( coord, square )
    if has_resource( coord ) then
      -- FIXME: forest is temporary.
      square.overlay = e.land_overlay.forest
    end
  end )
end

-----------------------------------------------------------------
-- Map Generator
-----------------------------------------------------------------
function M.generate()
  reset_terrain()
  local size = map_gen.world_size()

  -- local buffer = 10
  -- local initial_square = { x=size.w - buffer * 2, y=size.h / 2 }
  -- local initial_area = math.random( 50, 200 )
  -- generate_continent( initial_square, initial_area )
  -- for i = 1, 2 do
  --   local square = random_point_in_rect(
  --                      {
  --         x=buffer,
  --         y=buffer,
  --         w=size.w - buffer * 2,
  --         h=size.h - buffer * 2
  --       } )
  --   local area = math.random( 10, 300 )
  --   generate_continent( square, area )
  -- end
  -- clear_buffer_area( buffer / 2 )
  -- -- Need to do this before creating fish resources.
  -- create_sea_lanes()
  -- create_arctic()
  --
  -- forest_cover()

  on_all( function( coord, square )
    square.surface = e.surface.land
    square.ground = e.ground_terrain.plains
  end )

  distribute_prime_ground_resources()
end

return M
