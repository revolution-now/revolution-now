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

function M.initial_ship_pos()
  local size = map_gen.world_size()
  return { y=size.h / 2, x=size.w - 3 }
end

local function set_land( coord )
  local square = map_gen.at( coord )
  square.surface = e.surface.land
  square.ground = math.random( e.ground_terrain.desert,
                               e.ground_terrain.tundra )
end

local function set_water( coord )
  local square = map_gen.at( coord )
  square.surface = e.surface.water
  square.ground = e.ground_terrain.arctic
  square.sea_lane = false
end

local function set_sea_lane( coord )
  set_water( coord )
  local square = map_gen.at( coord )
  square.sea_lane = true
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

local function random_bool()
  if math.random( 1, 2 ) == 1 then
    return true
  else
    return false
  end
end

local function surrounding_squares_5x5( square )
  local possible = {
    -- Cardinal
    { x=square.x + 0, y=square.y - 1 }, --
    { x=square.x - 1, y=square.y + 0 }, --
    { x=square.x + 1, y=square.y + 0 }, --
    { x=square.x + 0, y=square.y + 1 }, --
    -- Diagonal
    { x=square.x - 1, y=square.y - 1 }, --
    { x=square.x + 1, y=square.y - 1 }, --
    { x=square.x - 1, y=square.y + 1 }, --
    { x=square.x + 1, y=square.y + 1 }, --
    -- 2 up
    { x=square.x - 1, y=square.y - 2 }, --
    { x=square.x + 0, y=square.y - 2 }, --
    { x=square.x + 1, y=square.y - 2 }, --
    -- 2 down
    { x=square.x - 1, y=square.y + 2 }, --
    { x=square.x + 0, y=square.y + 2 }, --
    { x=square.x + 1, y=square.y + 2 }, --
    -- 2 left
    { x=square.x - 2, y=square.y - 1 }, --
    { x=square.x - 2, y=square.y + 0 }, --
    { x=square.x - 2, y=square.y + 1 }, --
    -- 2 right
    { x=square.x + 2, y=square.y - 1 }, --
    { x=square.x + 2, y=square.y + 0 }, --
    { x=square.x + 2, y=square.y + 1 }, --
    -- 4 corners
    { x=square.x - 2, y=square.y - 2 }, --
    { x=square.x + 2, y=square.y - 2 }, --
    { x=square.x - 2, y=square.y + 2 }, --
    { x=square.x + 2, y=square.y + 2 } --
  }
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

-- Will clear a frame around the edge of the map to make sure
-- that land doesn't get too close to the map edge and we still
-- have room for sea lane squares.
local function clear_buffer_area( buffer_size )
  local size = map_gen.world_size()
  for y = 0, size.h - 1 do
    for x = 0, size.w - 1 do --
      if y < buffer_size or y > size.h - buffer_size or x <
          buffer_size or x > size.w - buffer_size then
        set_water{ x=x, y=y }
        -- set_sea_lane{ x=x, y=y }
      end
    end
  end
end

local function land_edges_on_row( y )
  local width = map_gen.world_size().w
  local left, right
  for x = 0, width - 1 do
    if map_gen.at( { x=x, y=y } ).surface == e.surface.land then
      left = x
      break
    end
  end
  for x = 0, width - 1 do
    local reversed = width - x - 1
    if map_gen.at( { x=reversed, y=y } ).surface ==
        e.surface.land then
      right = reversed
      break
    end
  end
  if left then assert( right ) end
  if right then assert( left ) end
  return left, right
end

-- Enforces that n is in [min, max].
local function clamp( n, min, max )
  if n < min then return min end
  if n > max then return max end
  return n
end

local function create_sea_lanes( max_width )
  local size = map_gen.world_size()
  local sea_lane_width = 0

  local sea_lane_buffer_max = 8
  local sea_lane_buffer = 5
  local sea_lane_buffer_min = 3

  local sea_lane_width_max = max_width
  local sea_lane_width_min = 1

  -- First the left side of the map.
  local left, right
  for y = 0, size.h - 1 do
    left, right = land_edges_on_row( y )
    if left then break end
  end
  sea_lane_width = left - sea_lane_buffer
  sea_lane_width = clamp( sea_lane_width, sea_lane_width_min,
                          sea_lane_width_max )
  for y = 0, size.h - 1 do
    left = land_edges_on_row( y )
    if left then
      if math.abs( left - sea_lane_width ) > sea_lane_buffer_max or
          math.abs( left - sea_lane_width ) < sea_lane_buffer_min then
        sea_lane_width = left - sea_lane_buffer
        sea_lane_width = clamp( sea_lane_width,
                                sea_lane_width_min,
                                sea_lane_width_max )
      end
    end
    for x = 0, sea_lane_width do set_sea_lane{ x=x, y=y } end
  end

  -- Now the right side of the map.
  local left, right
  for y = 0, size.h - 1 do
    left, right = land_edges_on_row( y )
    if right then break end
  end
  sea_lane_width = size.w - right
  sea_lane_width = clamp( sea_lane_width, sea_lane_width_min,
                          sea_lane_width_max )
  for y = 0, size.h - 1 do
    left, right = land_edges_on_row( y )
    if right then
      if math.abs( size.w - right - sea_lane_width ) >
          sea_lane_buffer_max or
          math.abs( size.w - right - sea_lane_width ) <
          sea_lane_buffer_min then
        sea_lane_width = size.w - right - sea_lane_buffer
        sea_lane_width = clamp( sea_lane_width,
                                sea_lane_width_min,
                                sea_lane_width_max )
      end
    end
    for x = size.w - sea_lane_width - 1, size.w - 1 do
      set_sea_lane{ x=x, y=y }
    end
  end

  -- Now find all land squares and make sure that there are no
  -- sea lane squares in their vicinity.
  for y = 0, size.h - 1 do
    for x = 0, size.w - 1 do
      local square = map_gen.at{ x=x, y=y }
      if square.surface == e.surface.land then
        local surrounding = surrounding_squares_5x5{ x=x, y=y }
        surrounding = filter_existing_squares( surrounding )
        for _, s in ipairs( surrounding ) do
          local square = map_gen.at( s )
          if square.sea_lane then set_water( s ) end
        end
      end
    end
  end
end

function M.generate()
  reset_terrain()
  local size = map_gen.world_size()
  local buffer = 10
  local initial_square = { x=size.w-buffer*2, y=size.h/2 }
  local initial_area = math.random( 50, 200 )
  generate_continent( initial_square, initial_area )
  for i = 1, 2 do
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
  clear_buffer_area( buffer / 2 )
  create_sea_lanes( buffer )
end

return M
