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
-- Imports
-----------------------------------------------------------------
local dist = require( 'map-gen.classic.resource-dist' )
local partition = require( 'map-gen.classic.land-partition' )
local weights = require( 'map-gen.classic.terrain-weights' )
local timer = require( 'util.timer' )

-----------------------------------------------------------------
-- aliases
-----------------------------------------------------------------
local min = math.min
local max = math.max

-----------------------------------------------------------------
-- Constants
-----------------------------------------------------------------
-- The maps in the original game are 58x72, but the tiles on the
-- edges are not visible, so effectively we have 56x70.
local CLASSIC_WORLD_SIZE = { w=56, h=70 }

-----------------------------------------------------------------
-- Options
-----------------------------------------------------------------
-- TODO: move this into a dedicated options module that can be
-- shared.
local function secure_options( tbl )
  assert( tbl )
  return setmetatable( tbl, {
    __index=function( tbl, key )
      error( 'options key ' .. key .. ' does not exist.' )
    end
  } )
end

function M.default_options()
  return secure_options{
    world_size=CLASSIC_WORLD_SIZE,
    type='normal',
    -- The original game seems to have a land density of about
    -- 25% on normal map generation settings. However we will put
    -- the average slightly lower because it tends to end up
    -- slightly higher than the target.
    land_density=.17 + math.random() * .1,
    remove_Xs=false,
    brush='rand',
    -- This is the probability that, given a land square, we will
    -- start creating a river from it.
    river_density=.10,
    -- Probability that each land square will have mountain-
    -- s/hills on it.
    hills_density=.05,
    -- If the map height is not at least this tall then the
    -- arctic rows at the top and bottom of the map will be
    -- omitted to save space. That said, the top and bottom proto
    -- squares will still be artic.
    min_map_height_for_arctic=10,
    mountains_density=.1,
    -- Probability that a tile that has been chosen to have a
    -- mountain on it will be the start of a mountain range.
    mountains_range_density=.1,
    -- This is the probability that a given river segment will be
    -- a major river. This should really be smaller than .5 be-
    -- cause major rivers give a production bonus over minor
    -- rivers.
    major_river_fraction=.15,
    native_tribes={
      'inca', 'aztec', 'apache', 'sioux', 'tupi', 'arawak',
      'cherokee', 'iroquois'
    }
  }
end

-----------------------------------------------------------------
-- Utils
-----------------------------------------------------------------
local function debug_log( fmt, ... )
  -- io.write( string.format( fmt .. '\n', ... ) )
end

local function append( tbl, elem ) tbl[#tbl + 1] = elem end

local function switch( b, t, f )
  if b then
    return t
  else
    return f
  end
end

-- Take a percent (where 1.0 is 100%) and format it like nn.n%
local function percent( x )
  return tostring( math.floor( x * 1000 ) / 10 ) .. '%'
end

local function round( x ) return math.floor( x + 0.5 ) end

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

local function random_bool( p )
  p = p or 0.5
  return math.random() < p
end

local function random_choice( p, l, r )
  if math.random() <= p then
    return l
  else
    return r
  end
end

local function random_list_elem( lst )
  return lst[math.random( 1, #lst )]
end

-- Mutates the list.
local function shuffle( lst )
  for i = 2, #lst do
    local elem = lst[i - 1]
    local other_idx = math.random( i, #lst )
    local other = lst[other_idx]
    lst[i - 1] = other
    lst[other_idx] = elem
  end
end

local function random_point_in_rect( rect )
  local size = { w=rect.w, h=rect.h }
  local x = math.random( 0, rect.w - 1 ) + rect.x
  local y = math.random( 0, rect.h - 1 ) + rect.y
  return { x=x, y=y }
end

local function random_cardinal_direction()
  return random_list_elem{
    { w=-1, h=0 }, { w=1, h=0 }, { w=0, h=-1 }, { w=0, h=1 }
  }
end

-----------------------------------------------------------------
-- Basic map access.
-----------------------------------------------------------------
local function world_size() return ROOT.terrain:size() end

local function square_exists( square )
  local size = world_size()
  return
      square.x >= 0 and square.y >= 0 and square.x < size.w and
          square.y < size.h
end

-- The square must exist.
local function square_at( coord )
  assert( square_exists( coord ),
          string.format( 'square {x=%d,y=%d} does not exist.',
                         coord.x, coord.y ) )
  return ROOT.terrain:square_at( coord )
end

-----------------------------------------------------------------
-- Algorithms
-----------------------------------------------------------------
-- This will call the function on each square of the map, passing
-- in the coordinate and the square object which the function may
-- use. Note that the coordinates are zero based.
local function on_all( f )
  local size = world_size()
  for y = 0, size.h - 1 do
    for x = 0, size.w - 1 do --
      local coord = { x=x, y=y }
      local square = square_at( coord )
      f( coord, square )
    end
  end
end

-----------------------------------------------------------------
-- Unit Placement
-----------------------------------------------------------------
local function initial_ships_pos_for_row( y )
  local size = world_size()
  assert( size.w > 0 and size.h > 0 )
  local x = size.w - 1
  while square_at{ x=x, y=y }.sea_lane do
    x = x - 1
    if not square_exists{ x=x, y=y } then
      return { x=size.w - 1, y=y }
    end
  end
  return x + 1
end

-- TODO: Move this into new-game.lua.
function M.initial_ships_pos()
  local size = world_size()
  local quintile = size.h // 5
  local y = size.h / 2
  local quintiles = {
    quintile, quintile * 2, quintile * 3, quintile * 4
  }
  shuffle( quintiles )
  local spanish_y = quintiles[1]
  local dutch_y = quintiles[2]
  local french_y = quintiles[3]
  local english_y = quintiles[4]
  local dutch_x = initial_ships_pos_for_row( dutch_y )
  local french_x = initial_ships_pos_for_row( french_y )
  local english_x = initial_ships_pos_for_row( english_y )
  local spanish_x = initial_ships_pos_for_row( spanish_y )
  return {
    dutch={ x=dutch_x, y=dutch_y },
    french={ x=french_x, y=french_y },
    english={ x=english_x, y=english_y },
    spanish={ x=spanish_x, y=spanish_y }
  }
end

-----------------------------------------------------------------
-- Terrain Manipulation
-----------------------------------------------------------------
local function is_on_map_edge( world_size, coord )
  return
      coord.x == 0 or coord.y == 0 or coord.x == world_size.w - 1 or
          coord.y == world_size.h - 1
end

local function set_land( coord )
  local square = square_at( coord )
  square.surface = 'land'
  square.ground = 'grassland'
end

local function set_water( coord )
  local square = square_at( coord )
  square.surface = 'water'
  square.ground = 'arctic'
  square.sea_lane = false
end

-- This will create a new empty map set all squares to water.
local function reset_terrain( options )
  ROOT.terrain:reset( options.world_size )
  -- FIXME: needed?
  on_all( set_water )
end

local function is_square_water( square )
  return square.surface == 'water'
end

local function set_square_arctic( square )
  square.surface = 'land'
  square.ground = 'arctic'
end

local function set_arctic( coord )
  local square = square_at( coord )
  set_square_arctic( square )
end

local function is_arctic_square( square )
  return square.surface == 'land' and square.ground == 'arctic'
end

local function is_sea_lane( coord )
  local square = square_at( coord )
  return square.sea_lane
end

local function set_square_sea_lane( square )
  square.surface = 'water'
  square.sea_lane = true
end

local function set_sea_lane( coord )
  local square = square_at( coord )
  set_square_sea_lane( square )
end

local function square_has_river( square )
  return square.river ~= nil
end

local function square_is_water_source( square )
  return square_has_river( square ) or is_square_water( square )
end

local function square_is_indirect_water_source( square )
  return square_is_water_source( square ) or square.ground ==
             'marsh' or square.ground == 'swamp'
end

-- row is zero-based.
local function row_has_land( row )
  local size = world_size()
  for x = 0, size.w - 1 do
    local square = square_at{ x=x, y=row }
    if square.surface == 'land' then return true end
  end
  return false
end

local function is_land( square ) return square.surface == 'land' end

local function is_coord_land( coord )
  return square_at( coord ).surface == 'land'
end

local function is_water( square ) return
    square.surface == 'water' end

local function right_most_land_square_in_row( row )
  local size = world_size()
  for x = size.w - 1, 0, -1 do
    local coord = { x=x, y=row }
    local square = square_at( coord )
    if square.surface == 'land' then return coord end
  end
  return nil
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

local function surrounding_squares_3x3( square )
  local possible = {}
  for y = square.y - 1, square.y + 1 do
    for x = square.x - 1, square.x + 1 do
      if x ~= square.x or y ~= square.y then
        append( possible, { x=x, y=y } )
      end
    end
  end
  return possible
end

local function generate_large_tribe_owned_land()
  local _ = false
  local X = true
  local map = {
    _, _, X, X, X, _, _, --
    _, X, X, X, X, X, _, --
    X, X, X, X, X, X, X, --
    X, X, X, X, X, X, X, --
    X, X, X, X, X, X, X, --
    _, X, X, X, X, X, _, --
    _, _, X, X, X, _, _ --
  }
  assert( #map == 7 * 7 )
  return map
end
-- Generate/cache this once for speed.
local LARGE_TRIBE_OWNED_LAND_MAP =
    generate_large_tribe_owned_land()

local function surrounding_squares_tribe_owned( tribe, coord )
  assert( tribe )
  -- FIXME: dwelling radius is specified in the config files,
  -- should not be duplicated here.
  if tribe == 'aztec' or tribe == 'inca' then
    local possible = {}
    for h = -3, 3 do
      for w = -3, 3 do
        if w ~= 0 or h ~= 0 then
          local on = LARGE_TRIBE_OWNED_LAND_MAP[(h + 3) * 7 +
                         (w + 3) + 1] -- +1 for lua.
          assert( on ~= nil ) -- must be true or false.
          if on then
            append( possible, { x=coord.x + w, y=coord.y + h } )
          end
        end
      end
    end
    return possible
  else
    return surrounding_squares_3x3( coord )
  end
end

local function surrounding_squares_5x5( square )
  local possible = {}
  for y = square.y - 2, square.y + 2 do
    for x = square.x - 2, square.x + 2 do
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

local function filter_on_map( coords )
  local res = {}
  for _, coord in ipairs( coords ) do
    if square_exists( coord ) then table.insert( res, coord ) end
  end
  return res
end

local function filter( lst, predicate )
  local res = {}
  for _, elem in ipairs( lst ) do
    local retain = predicate( elem )
    assert( retain == true or retain == false )
    if retain then table.insert( res, elem ) end
  end
  return res
end

-----------------------------------------------------------------
-- Hills/Mountains Generation
-----------------------------------------------------------------
local function create_hills( options )
  local size = world_size()
  on_all( function( coord )
    local square = square_at( coord )
    if square.surface == 'land' then
      -- Make sure there are no hills/mountains on this tile.
      if square.ground ~= 'arctic' and square.overlay == nil then
        if math.random() <= options.hills_density then
          square.overlay = 'hills'
        end
      end
    end
  end )
end

local function can_receive_mountain( square )
  return
      square.surface == 'land' and square.ground ~= 'arctic' and
          square.overlay == nil
end

local function create_mountain_range( options, coord )
  local range_length = math.random( 1, 10 )
  local curr = coord
  for i = 1, range_length do
    if square_exists( curr ) then
      local square = square_at( curr )
      if can_receive_mountain( square ) then
        square.overlay = 'mountains'
      end
    end
    local d = random_cardinal_direction()
    curr.x = curr.x + d.w
    curr.y = curr.y + d.h
  end
end

local function create_mountains( options )
  local size = world_size()
  on_all( function( coord )
    local square = square_at( coord )
    if can_receive_mountain( square ) then
      if math.random() <= options.mountains_density then
        if math.random() <= options.mountains_range_density then
          create_mountain_range( options, coord )
        else
          square.overlay = 'mountains'
        end
      end
    end
  end )
end

-----------------------------------------------------------------
-- Forest Generation
-----------------------------------------------------------------
-- TODO: tweak the density of forest to match the original game.
local function forest_cover()
  local size = world_size()
  on_all( function( coord )
    local square = square_at( coord )
    if square.surface == 'land' then
      -- Make sure there are no hills/mountains on this tile.
      if square.ground ~= 'arctic' and square.overlay == nil then
        if math.random() <= .95 then
          square.overlay = 'forest'
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
local function clear_buffer_area( buffer )
  local size = world_size()
  on_all( function( coord )
    local x = coord.x
    local y = coord.y
    if y < buffer.top or y >= size.h - buffer.bottom or x <
        buffer.left or x >= size.w - buffer.right then
      set_water( coord )
    end
  end )
end

local function create_arctic_along_row( y )
  local size = world_size()
  -- Note that we don't include the edges.
  for x = 1, size.w - 2 do
    if math.random( 1, 2 ) == 1 then set_arctic{ x=x, y=y } end
  end
end

local function create_arctic( options )
  local size = world_size()
  if size.h < options.min_map_height_for_arctic then return end
  create_arctic_along_row( 0 )
  create_arctic_along_row( size.h - 1 )
end

-----------------------------------------------------------------
-- Sea Lane Generation
-----------------------------------------------------------------
local function create_sea_lanes()
  local size = world_size()

  -- First set all water tiles to sea lane.
  on_all( function( coord, square )
    if is_square_water( square ) then square.sea_lane = true end
  end )

  -- Now clear out the sea lane tiles in the west half of the map
  -- because we don't want sea lane to extend too far west. Also,
  -- the original game seems to do exactly this.
  on_all( function( coord, square )
    if coord.x < size.w / 2 then square.sea_lane = false end
  end )

  -- Now find all land squares and make sure that there are no
  -- sea lane squares in their vicinity (7x7 square). And for
  -- each of those water squares, clear all sea lane squares
  -- along the entire row to the left of it until the map edge.
  -- In order to make this more efficient, instead of applying
  -- this to all land squares, we will only apply it to the
  -- right-most land square in each row, since that will yield an
  -- equivalent result.
  for y = 0, size.h - 1 do
    local coord = right_most_land_square_in_row( y )
    if coord ~= nil then
      local block_edge = {}
      -- We need to do this because if we are are very close to
      -- the right edge of the map (e.g., arctic) then the right
      -- edge of the 7x7 square won't exist; in that case, just
      -- move it over to the left by one, since that will have
      -- the same effect.
      repeat
        block_edge = surrounding_squares_7x7_right_edge( coord )
        block_edge = filter_on_map( block_edge )
        coord.x = coord.x - 1
      until #block_edge > 0
      for _, s in ipairs( block_edge ) do
        -- Walk from the right to the left until we either get to
        -- the left edge of the map or we find an ocean square
        -- with no sea lane, which means we've already cleared
        -- the remainder as part of another row, so we can stop.
        for x = s.x, 0, -1 do
          local coord = { x=x, y=s.y }
          local square = square_at( coord )
          if square.surface == 'water' and not square.sea_lane then
            break
          end
          if square.sea_lane then
            square.sea_lane = false
          end
        end
      end
    end
  end

  -- At this point, some rows (that contain no land tiles and are
  -- far from a row that does) will be all sea lane. So we will
  -- start at the center of the map and move upward (downward) to
  -- find them and we will set their sea lane width (i.e., the
  -- width on the right side of the map) to what it was below
  -- (above) that row.
  --
  -- Run through all rows and find the row that is not entirely
  -- sea lane that is closest to the center of the map.
  local closest_row = 0 -- row zero has been cleared of sea lane.
  local y_mid = size.h / 2
  for y = 0, size.h - 1 do
    if row_has_land( y ) then
      if math.abs( y - y_mid ) < math.abs( closest_row - y_mid ) then
        -- We've found a non-sea-lane row that is closer to the
        -- middle then what we've found so far.
        closest_row = y
      end
    end
  end
  debug_log( 'starting row: %d', closest_row )
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
  debug_log( 'curr width: %d', curr_sea_lane_width )
  -- Now start at the row that we found and go upward.
  for y = closest_row - 1, 0, -1 do
    if row_has_land( y ) then
      curr_sea_lane_width = sea_lane_width( y )
    else
      -- Clear the sea lane and make it have the width of the row
      -- below it.
      for x = 0, size.w - 1 - curr_sea_lane_width do
        square_at{ x=x, y=y }.sea_lane = false
      end
    end
  end
  -- Now start at the row that we found and go downward.
  curr_sea_lane_width = sea_lane_width( closest_row )
  for y = closest_row + 1, size.h - 1 do
    if row_has_land( y ) then
      curr_sea_lane_width = sea_lane_width( y )
    else
      -- Clear the sea lane and make it have the width of the row
      -- above it.
      for x = 0, size.w - 1 - curr_sea_lane_width do
        square_at{ x=x, y=y }.sea_lane = false
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
-- Natives
-----------------------------------------------------------------
-- Generates partitions, colors the land according to them, and
-- returns the partitions.
local function paint_native_land_partitions( partitions )
  local size = world_size()
  local partition_to_ground = {
    [0]='grassland',
    [1]='prairie',
    [2]='savannah',
    [3]='plains',
    [4]='desert',
    [5]='arctic',
    [6]='tundra',
    [7]='swamp'
  }
  for rasterized_coord, n in pairs( partitions ) do
    local coord = {
      x=rasterized_coord % size.w,
      y=rasterized_coord // size.w
    }
    local square = assert( square_at( coord ) )
    if not is_on_map_edge( size, coord ) and square.surface ==
        'land' then
      square.overlay = nil
      square.ground_resource = nil
      square.forest_resource = nil
      square.lost_city_rumor = nil
      square.river = nil
      square.ground = assert( partition_to_ground[n] )
    end
  end
end

local function has_dwelling_in_surroundings( coord )
  local squares =
      filter_on_map( surrounding_squares_3x3( coord ) )
  local natives = ROOT.natives
  for _, coord in ipairs( squares ) do
    local square = square_at( coord )
    if natives:has_dwelling_on_square( coord ) then
      return true
    end
  end
  return false
end

local function create_brave_for_dwelling( dwelling )
  local natives = ROOT.natives
  local location = natives:coord_for_dwelling( dwelling.id )
  local squares = filter_on_map(
                      surrounding_squares_3x3( location ) )
  squares = filter( squares, function( coord )
    if square_at( coord ).surface ~= 'land' then return false end
    local tribe = society.tribe_on_square( coord )
    local dwelling_tribe = natives:tribe_for_dwelling(
                               dwelling.id )
    if tribe ~= nil and tribe ~= dwelling_tribe then
      return false
    end
    return true
  end )
  append( squares, location )
  local coord = assert( random_list_elem( squares ) )
  unit_mgr.create_native_unit_on_map( dwelling.id, 'brave', coord )
end

-- FIXME: need to get this from config files after they are ex-
-- posed to lua.
local function set_dwelling_population( tribe, dwelling )
  if tribe == 'apache' or tribe == 'sioux' or tribe == 'tupi' then
    dwelling.population = 3
    return
  end
  if tribe == 'arawak' or tribe == 'cherokee' or tribe == 'iroquois' then
    dwelling.population = 5
    if dwelling.is_capital then
      dwelling.population = 6
    end
    return
  end
  if tribe == 'aztec' then
    dwelling.population = 7
    if dwelling.is_capital then
      dwelling.population = 8
    end
    return
  end
  if tribe == 'inca' then
    dwelling.population = 9
    if dwelling.is_capital then
      dwelling.population = 11
    end
    return
  end
  error( 'tribe ' .. tribe .. ' not handled.' )
end

local function add_dwelling( coord, tribe )
  assert( coord )
  local square = square_at( coord )
  local natives = ROOT.natives
  natives:create_or_add_tribe( tribe )
  local dwelling = natives:new_dwelling( tribe, coord )
  dwelling.teaches =
      native_expertise.select_expertise_for_dwelling( dwelling )
  -- Get rid of any forest if we're placing one of the city
  -- dwellings. The OG does not do this, but they don't really
  -- look good floating above a forest given that they are sup-
  -- posed to represent advanced cities, so we will remove
  -- them.
  if tribe == 'aztec' or tribe == 'inca' then
    square.overlay = nil
  end
  square.road = true
  -- We'll try not to build on LCRs or mountains, but occasion-
  -- ally we are forced to place a settlement there, so clear
  -- them anyway.
  square.lost_city_rumor = false
  if square.overlay == 'mountains' then square.overlay = nil end
  -- Mark the land around the dwelling as owned by this village.
  -- If it's already owned by another dwelling that's ok, we'll
  -- just overwrite it.
  local owned_squares
  owned_squares = filter_on_map(
                      surrounding_squares_tribe_owned( tribe,
                                                       coord ) )
  natives:mark_land_owned( dwelling.id, coord )
  for _, coord in ipairs( owned_squares ) do
    natives:mark_land_owned( dwelling.id, coord )
  end
  -- Create the brave associated with this dwelling.
  create_brave_for_dwelling( dwelling )

  return dwelling
end

local function create_indian_villages_using_partition(options,
                                                      partitions )
  -- Note that indian villages never seem to be placed on top of
  -- the lost city rumors, though they can be placed on top of
  -- natural resources.
  local size = world_size()
  local city_tribes = { 'inca', 'aztec' }
  local non_city_tribes = {
    'apache', 'sioux', 'tupi', 'arawak', 'cherokee', 'iroquois'
  }
  -- Helps to make sure that we place at least one dwelling for
  -- each tribe.
  local placed_at_least_one = {}
  -- This shuffling and recombination ensures that the tribes are
  -- placed into random partitions subject to the constraint that
  -- the incas and aztecs should go toward the left half of the
  -- map.
  shuffle( city_tribes )
  shuffle( non_city_tribes )
  local tribes = non_city_tribes
  table.insert( tribes, 2, city_tribes[1] )
  table.insert( tribes, 5, city_tribes[2] )
  local dwellings = {}
  for _, tribe in ipairs( tribes ) do dwellings[tribe] = {} end
  local coords_for_partition = {}
  for i = 1, #tribes do coords_for_partition[i] = {} end
  for rasterized_coord, n in pairs( partitions ) do
    local tribe = tribes[n + 1]
    local coord = {
      x=rasterized_coord % size.w,
      y=rasterized_coord // size.w
    }
    if is_on_map_edge( size, coord ) then goto continue end
    table.insert( coords_for_partition[n + 1], coord )
    local square = square_at( coord )
    -- The OG does not seem to place native dwellings on moun-
    -- tains, hills, or desert.
    local should_place = not square.lost_city_rumor and
                             square.overlay ~= 'mountains' and
                             square.overlay ~= 'hills' and
                             square.ground ~= 'desert' and
                             not has_dwelling_in_surroundings(
                                 coord ) and math.random() < .15
    if not should_place then goto continue end
    -- We're clear to place the dwelling.
    local dwelling = add_dwelling( coord, tribe )
    table.insert( dwellings[tribe], dwelling )
    placed_at_least_one[tribe] = true
    ::continue::
  end
  -- Check if we haven't placed any dwellings for each tribe and,
  -- if not, just pick a random place to put it within its parti-
  -- tion.
  for n, tribe in ipairs( tribes ) do
    if not placed_at_least_one[tribe] and
        #coords_for_partition[n] > 0 then
      local coord = random_list_elem( coords_for_partition[n] )
      assert( coord )
      local dwelling = add_dwelling( coord, tribe )
      table.insert( dwellings[tribe], dwelling )
      placed_at_least_one[tribe] = true
    end
  end
  -- Pick a capital for each tribe.
  for _, tribe in ipairs( tribes ) do
    local tribe_dwellings = dwellings[tribe]
    shuffle( tribe_dwellings )
    if #tribe_dwellings > 0 then
      tribe_dwellings[1].is_capital = true
    end
  end
  -- Assign dwelling populations now that we've populated the
  -- "capital" field.
  for tribe, dwellings in pairs( dwellings ) do
    for _, dwelling in ipairs( dwellings ) do
      set_dwelling_population( tribe, dwelling )
    end
  end
end

local tribe_level = {
  apache='semi_nomadic',
  sioux='semi_nomadic',
  tupi='semi_nomadic',
  arawak='agrarian',
  cherokee='agrarian',
  iroquois='agrarian',
  aztec='civilized',
  inca='civilized'
}

local function log_dwelling_expertises( level )
  local dwelling_id = 1
  local natives = ROOT.natives
  local histogram = {}
  local total_dwellings = 0
  while natives:dwelling_exists( dwelling_id ) do
    local dwelling = natives:dwelling_for_id( dwelling_id )
    local tribe = natives:tribe_for_dwelling( dwelling_id )
    if level == nil or assert( tribe_level[tribe] ) == level then
      local teaches = assert( dwelling.teaches )
      if histogram[teaches] == nil then
        histogram[teaches] = 0
      end
      histogram[teaches] = histogram[teaches] + 1
      total_dwellings = total_dwellings + 1
    end
    dwelling_id = dwelling_id + 1
  end
  local sorted_buckets = {}
  for expertise, weight in pairs( histogram ) do
    table.insert( sorted_buckets,
                  { expertise=expertise, weight=weight } )
  end
  table.sort( sorted_buckets,
              function( l, r ) return l.weight > r.weight end )
  log.debug( string.format( 'Dwelling expertise weights [%s]:',
                            level or 'all' ) )
  for _, pair in ipairs( sorted_buckets ) do
    local fraction = pair.weight / total_dwellings
    local num_dwellings = math.floor(
                              total_dwellings * fraction + .5 )
    log.debug( string.format( ' |%18s: %6s, %4d dwellings.',
                              pair.expertise, string.format(
                                  '%.1f%%', fraction * 100.0 ),
                              num_dwellings ) )
  end
end

local function create_indian_villages( options )
  local size = world_size()
  local function has_land( coord )
    return square_at( coord ).surface == 'land'
  end
  local partitions = partition.generate( size,
                                         #options.native_tribes,
                                         has_land )
  create_indian_villages_using_partition( options, partitions )
  log_dwelling_expertises( 'semi_nomadic' )
  log_dwelling_expertises( 'agrarian' )
  log_dwelling_expertises( 'civilized' )
  log_dwelling_expertises()
end

-----------------------------------------------------------------
-- Lost City Rumors
-----------------------------------------------------------------
local function distribute_lost_city_rumors( placement_seed )
  local size = world_size()
  local coords = dist.compute_lost_city_rumors( size,
                                                placement_seed )
  for _, coord in ipairs( coords ) do
    local square = square_at( coord )
    if square.surface == 'land' and square.ground ~= 'arctic' then
      square.lost_city_rumor = true
    end
  end
  log.debug( 'lost city rumor count=' .. tostring( #coords ) ..
                 ', density=' ..
                 tostring( #coords / (size.w * size.h) ) )
end

-----------------------------------------------------------------
-- Prime Resource Generation
-----------------------------------------------------------------
local RESOURCES_GROUND = {
  ['arctic']=nil,
  ['desert']='oasis',
  ['grassland']='tobacco',
  ['marsh']='minerals',
  ['plains']='wheat',
  ['prairie']='cotton',
  ['savannah']='sugar',
  ['swamp']='minerals',
  ['tundra']='minerals'
}

-- If a forest tile has a prime resource then this table will
-- give the resource that it would have based on the ground tile
-- in accordance with the original game's rules.
local RESOURCES_FOREST = {
  ['arctic']=nil,
  ['desert']='oasis',
  ['grassland']='tree',
  ['marsh']='minerals',
  ['plains']='beaver',
  ['prairie']='deer',
  ['savannah']='tree',
  ['swamp']='minerals',
  ['tundra']='deer'
}

-- The original game checks to see if there is at least one land
-- square within *two* tiles from the water square in order to
-- allow placing a fish there. There isn't any point to going be-
-- yond a one-tile distance in the check because those are the
-- only squares that will be accessible to any colony. That be-
-- havior may have been left from a previous time when colonies
-- were able to work squares at a radius of two around them, and
-- was never changed when that radius was lowered to one). In any
-- case, we could just check that we are one square away from
-- land, but we will replicate the behavior of the original game
-- and place them two squares away, since 1) there is no harm,
-- and 2) it does make navigating by ship a bit easier since you
-- can effectively spot land from further away by noticing
-- fishes, which is a help.
local function can_place_fish( coord )
  local squares =
      filter_on_map( surrounding_squares_5x5( coord ) )
  for _, coord in ipairs( squares ) do
    if square_at( coord ).surface == 'land' then return true end
  end
  return false
end

-- This includes water, hills, and mountains, i.e. everything but
-- forests. That said, it will still potentially be called on
-- forested tiles, and in that case it adds a non-forested re-
-- source. This is to mirror the original game's behavior where a
-- forested tile can have a ground resource (placed there by the
-- fixed pattern) which then appears only after the forest is
-- cleared (the ground resource has no effect on production until
-- the forest is cleared).
local function add_ground_prime_resource( coord, square )
  if square.surface == 'water' then
    if can_place_fish( coord ) then
      square.ground_resource = 'fish'
    end
    return
  end
  if square.overlay == 'hills' then
    square.ground_resource = 'ore'
    return
  end
  if square.overlay == 'mountains' then
    square.ground_resource = 'silver'
    return
  end
  -- We apply the ground resource whether or not the tile has
  -- forest (see above).
  local resource = RESOURCES_GROUND[square.ground]
  if resource then square.ground_resource = resource end
end

local function add_forest_prime_resource( coord, square )
  assert( square.overlay == 'forest' )
  assert( square.surface ~= 'water' )
  local resource = RESOURCES_FOREST[square.ground]
  if resource then square.forest_resource = resource end
end

local function distribute_prime_ground_resources( y_offset )
  local size = world_size()
  local coords = dist.compute_prime_ground_resources( size,
                                                      y_offset )
  for _, coord in ipairs( coords ) do
    local square = square_at( coord )
    add_ground_prime_resource( coord, square )
  end
  log.debug( 'prime ground resources density: ' ..
                 tostring( #coords / (size.w * size.h) ) )
end

local function distribute_prime_forest_resources( y_offset )
  local size = world_size()
  local coords = dist.compute_prime_forest_resources( size,
                                                      y_offset )
  for _, coord in ipairs( coords ) do
    local square = square_at( coord )
    if square.surface == 'land' and square.overlay == 'forest' then
      -- Our distribution algorithm will never place a forest
      -- prime resource on the same square as a ground prime re-
      -- source, but theoretically that would not be a problem if
      -- it happened.
      add_forest_prime_resource( coord, square )
    end
  end
  log.debug( 'prime forest resources density: ' ..
                 tostring( #coords / (size.w * size.h) ) )
end

local function distribute_prime_resources( placement_seed )
  -- These need to both have the same seed so that we can make
  -- sure that they remain four tiles apart (horizontally), which
  -- they will due to their hard coded x_offsets, in order to
  -- mirror the original game's placement logic.
  distribute_prime_ground_resources( placement_seed )
  distribute_prime_forest_resources( placement_seed )
end

local function set_random_placement_seed()
  local placement_seed = math.random( 0, 256 )
  ROOT.terrain:set_placement_seed( placement_seed )
  return placement_seed
end

-----------------------------------------------------------------
-- Ground Terrain Assignment
-----------------------------------------------------------------
local function assign_dry_ground_types()
  local size = world_size()
  local dry_weights = {}
  for y = 0, size.h - 1 do
    dry_weights[y] = weights.dry_weights_for_row( size.h, y )
  end
  on_all( function( coord, square )
    if is_water( square ) then return end
    if coord.y == 0 or coord.y == size.h - 1 then
      square.ground = 'arctic'
      return
    end
    square.ground = weights.select_from_weights(
                        dry_weights[coord.y] )
  end )
end

-- Is this tile or an adjacent tile a direct water source, e.g.
-- an ocean/lake or river.
local function has_water_source( coord )
  local squares =
      filter_on_map( surrounding_squares_3x3( coord ) )
  table.insert( squares, coord )
  for _, adjacent in ipairs( squares ) do
    if square_is_indirect_water_source( square_at( adjacent ) ) then
      return true
    end
  end
  return false
end

local function assign_wet_ground_types()
  local size = world_size()
  local wet_weights = {}
  for y = 0, size.h - 1 do
    wet_weights[y] = weights.wet_weights_for_row( size.h, y )
  end
  on_all( function( coord, square )
    if is_water( square ) then return end
    if coord.y == 0 or coord.y == size.h - 1 then return end
    if not has_water_source( coord ) then return end
    -- This is a land tile not on the border and it is on or ad-
    -- jacent to a river or ocean/lake.
    local type = weights.select_from_weights(
                     wet_weights[coord.y] )
    if type == 'none' then return end
    square.ground = type
  end )
end

-----------------------------------------------------------------
-- Patchwork/Cleanup
-----------------------------------------------------------------
-- The land generated by the original game does not appear to
-- have any land/water Xs in it, e.g.:
--
--   L O            O L
--   O L     or     L O
--
-- Indications are that Civilization 1 removed these as a final
-- stage of its map generation, and so Colonization 1 might be
-- doing the same. These are not ideal visually because, if we
-- are to render them, then extra rendering complications are in-
-- troduced because we need to signal to the player that they are
-- actually connected in that a ship can sail between them (the
-- naive rendering would draw the ocean squares as ocean sin-
-- gleton tiles and so there would be no visual indication that
-- they are connected). It is possibel that Civilization 1 wanted
-- to avoid this rendering complication and so it just removes
-- them. And since Colonization may have borrowed from the Civ
-- engine, it may do the same thing for that reason.
--
-- Colonization, however, is able to properly render these Xs
-- with a visual cue (thin blue "canal" between the diagonally
-- adjacent water tiles) to signal to the player that they are
-- connected) as evidenced by the fact that we can create maps
-- containing them in the map editor. This game can also render
-- them similarly, and so there is really no reason to omit them.
-- But, we will do so here for two reasons: 1) because the orig-
-- inal game did, and 2) we otherwise seem to end up with too
-- many of them and they don't really look good.
local function remove_Xs()
  local size = world_size()
  on_all( function( coord, square )
    if coord.y < size.h - 1 and coord.x < size.w - 1 then
      local square_right = square_at{ x=coord.x + 1, y=coord.y }
      local square_down = square_at{ x=coord.x, y=coord.y + 1 }
      local square_diag =
          square_at{ x=coord.x + 1, y=coord.y + 1 }
      if is_land( square ) and is_water( square_right ) and
          is_water( square_down ) and is_land( square_diag ) then
        square_diag.surface = 'water'
      end
      if is_water( square ) and is_land( square_right ) and
          is_land( square_down ) and is_water( square_diag ) then
        square_diag.surface = 'land'
      end
    end
  end )
end

-- We cannot have islands in the game because that would give the
-- player a way to have a coastal colony that could not be at-
-- tacked by the royal forces during the war of independence be-
-- cause they would have no adjacent land squares on which to
-- land. The exception is the arctic, in which we will allow is-
-- lands. If the player wants to use that as a loop hole, so be
-- it.
local function remove_islands()
  local size = world_size()
  on_all( function( coord, square )
    if coord.y > 0 and coord.y < size.h - 1 then
      local surrounding = filter_on_map(
                              surrounding_squares_3x3( coord ) )
      local has_land = false
      for _, square in ipairs( surrounding ) do
        if square_at( square ).surface == 'land' then
          has_land = true
          break
        end
      end
      if not has_land then
        -- We have an island.
        square.surface = 'water'
      end
    end
  end )
end

-- Although there is nothing wrong with river islands from the
-- point of view of game rules, they don't look good and they are
-- not realistic (and the player might confuse them with ocean
-- lakes where ships can enter).
local function remove_river_islands()
  on_all( function( coord, square )
    -- Note that we only consider the adjacent squares in the
    -- cardinal directions because those are how rivers are
    -- joined to each other in the game and visually.
    local surrounding = filter_on_map(
                            surrounding_squares_cardinal( coord ) )
    local has_river = false
    for _, square in ipairs( surrounding ) do
      if square_at( square ).river ~= nil then
        has_river = true
        break
      end
    end
    if not has_river then
      -- We have an island.
      square.river = nil
    end
  end )
end

-- The river generation algorithm we are using has a tendency to
-- generate a lot of 2x2 squares of rivers, which look kind of
-- unnatural. This will remove them by removing the river from
-- the lower right tile.
local function remove_river_quads()
  local size = world_size()
  on_all( function( coord, square )
    if not square_has_river( square ) then return end
    if coord.y == size.h - 1 or coord.x == size.w - 1 then
      return
    end
    local square_right = square_at{ x=coord.x + 1, y=coord.y }
    if not square_has_river( square_right ) then return end

    local square_down = square_at{ x=coord.x, y=coord.y + 1 }
    if not square_has_river( square_down ) then return end

    local square_diag = square_at{ x=coord.x + 1, y=coord.y + 1 }
    if not square_has_river( square_diag ) then return end

    square_diag.river = nil
  end )
end

-----------------------------------------------------------------
-- River Generation
-----------------------------------------------------------------
local function create_river_segment( options, coord, river_type )
  local pos = { x=coord.x, y=coord.y }
  -- This does not have to be long, since even a short range will
  -- give rise to long segments since in many cases two consecu-
  -- tive segments will be pointing in the same direction.
  local length = math.random( 1, 3 )
  local delta = random_cardinal_direction()
  for i = 1, length do
    if not square_exists( pos ) then return nil end
    local square = square_at( pos )
    if is_arctic_square( square ) then return nil end
    if square.overlay == 'hills' or square.overlay == 'mountains' then
      return nil
    end
    square.river = river_type
    pos.x = pos.x + delta.w
    pos.y = pos.y + delta.h
    -- If we reach the ocean that we have to stop since we have
    -- no where else to go, but note that we did want to set the
    -- river attribute on this final ocean tile because that is
    -- what gives the river bonus to fisherman.
    if is_water( square ) then return nil end
  end
  -- Keep going.
  return pos
end

local function create_river( options, coord )
  local num_segments = math.random( 1, 8 )
  local river_type = 'minor'
  if random_bool( options.major_river_fraction ) then
    river_type = 'major'
  end
  for i = 1, num_segments do
    coord = create_river_segment( options, coord, river_type )
    -- We've either reached the ocean or off-map.
    if not coord then break end
  end
end

-- TODO: in the original game, all rivers seem to originate on
-- ocean tiles, even inland ocean tiles (which makes sense).
-- Also, it looks like rivers can fork mid-way.
local function create_rivers( options )
  on_all( function( coord, square )
    if is_land( square ) then
      if random_bool( options.river_density ) then
        -- Before starting a new river make sure there are no ex-
        -- isting rivers in the vicinity. Without this, the
        -- rivers tend to clump too much.
        local squares = filter_on_map(
                            surrounding_squares_3x3( coord ) )
        for _, coord in ipairs( squares ) do
          if square_at( coord ).river then goto skip end
        end
        create_river( options, coord )
        ::skip::
      end
    end
  end )
  remove_river_quads()
  -- Should be last.
  remove_river_islands()
end

-----------------------------------------------------------------
-- Proto Squares.
-----------------------------------------------------------------
-- This will set the four squares that represent those off of the
-- map in each of the four cardinal directions. Squares off of
-- the map are not directly visible and are not accessible in the
-- game, but it is useful to have them for rendering purposes
-- when rendering tiles at the edges of the map. Also, these
-- proto squares are returned by the square_at method so that
-- code doesn't have to filter out non-exstent squares in every
-- algorithm.
local function generate_proto_squares()
  local size = world_size()

  -- Arctic.
  set_square_arctic( ROOT.terrain:proto_square( 'n' ) )
  set_square_arctic( ROOT.terrain:proto_square( 's' ) )

  -- Sea lane.
  set_square_sea_lane( ROOT.terrain:proto_square( 'e' ) )
  set_square_sea_lane( ROOT.terrain:proto_square( 'w' ) )
end

-----------------------------------------------------------------
-- Continent Generation
-----------------------------------------------------------------
-- For each continent that we ask the continent generator to gen-
-- erate, we need to give it an area. One way to do that would be
-- to select the area randomly. However, that then causes prob-
-- lems where continents with a seed close to the map edge get a
-- large area and then end up with a flat cutoff along one of the
-- sides, which looks unnatural. So instead we choose the conti-
-- nent aread based on where the seed is. The closer the seed is
-- to a map edge along a given dimension (x or y), the less its
-- area will (statistically) extend along that dimensions.
--
-- scale will affect continent size.
--
local function continent_stretch_for_seed( seed_square, scale )
  local size = world_size()
  local stretch_x = min( seed_square.x, size.w - seed_square.x )
  local stretch_y = min( seed_square.y, size.h - seed_square.y )
  local stretch = scale * min( stretch_x, stretch_y )
  return { x=stretch, y=stretch }
end

local function set_land_if_needed( coord )
  if not square_exists( coord ) then return 0 end
  if square_at( coord ).surface == 'land' then
    -- Bail here so that we don't clame to have added land where
    -- it already existed, which would mess up the land density
    -- calculations.
    return 0
  end
  set_land( coord )
  return 1
end

-- A "brush" is a function that paints land on the map with a
-- given shape and returns the number of water squares that were
-- changed to land in the process.
local brushes = {
  single=function( self, x, y )
    local count = 0
    count = count + set_land_if_needed{ x=x, y=y }
    return count
  end,
  cross=function( self, x, y )
    local count = 0
    count = count + set_land_if_needed{ x=x, y=y }
    count = count + set_land_if_needed{ x=x - 1, y=y }
    count = count + set_land_if_needed{ x=x + 1, y=y }
    count = count + set_land_if_needed{ x=x, y=y - 1 }
    count = count + set_land_if_needed{ x=x, y=y + 1 }
    return count
  end,
  mixed=function( self, x, y )
    if random_bool() then
      return self:single( x, y )
    else
      return self:cross( x, y )
    end
  end,
  rand=function( self, x, y )
    local count = 0
    local squares = surrounding_squares_cardinal{ x=x, y=y }
    for _, coord in ipairs( squares ) do
      if random_bool( .5 ) then
        count = count + set_land_if_needed( coord )
      end
    end
    return count
  end
}

-- Start at the seed square and do a biased 2D random walk
-- filling in land squares.
local function generate_continent( options, seed_square, stretch )
  local square = seed_square
  local area = (stretch.x * 2) * (stretch.y * 2)
  local land_squares = 0
  local p_horizontal = stretch.x / (stretch.x + stretch.y)
  local p_vertical = 1 - p_horizontal

  set_land( square )
  local brush = assert( brushes[options.brush] )
  for i = 1, area - 1 do
    local delta_x = random_choice( p_horizontal, 1, 0 ) *
                        random_choice( .5, -1, 1 )
    local delta_y = random_choice( p_vertical, 1, 0 ) *
                        random_choice( .5, -1, 1 )
    square = { x=square.x + delta_x, y=square.y + delta_y }
    land_squares = land_squares +
                       brush( brushes, square.x, square.y )
  end
  return land_squares
end

-----------------------------------------------------------------
-- Land Generation
-----------------------------------------------------------------
local function total_land_density( land_count )
  local size = world_size()
  local total_count = size.w * size.h
  local density = land_count / total_count
  assert( density <= 1.0 )
  return density
end

-- This will generate a continent with a seed in the given rec-
-- tangle. Note that the continent generated may escape the rect.
local function generate_continent_in_rect( options, seed_rect )
  local square = random_point_in_rect( seed_rect )
  local scale = math.random( 12, 42 ) / 100
  local stretch = continent_stretch_for_seed( square, scale )
  return generate_continent( options, square, stretch )
end

local function round_buffer( target )
  return min( max( round( target ), 1 ), 10 )
end

local function generate_land( options )
  local size = world_size()
  -- The buffer zone will have no land in it, so it should be
  -- relatively small. These are calculated so that for the orig-
  -- inal game's map size they should yeild the buffer values
  -- that the original game appears to use. These need to be
  -- scaled by the map size otherwise for small maps the seed
  -- square will be too small.
  local buffer = {
    top=round_buffer( size.h / 70 ),
    bottom=round_buffer( size.h / 70 ),
    left=round_buffer( 3 * size.w / 56 ),
    right=round_buffer( 2 * size.w / 56 )
  }
  -- Seeds will be chosen from this rect, which is a bit smaller
  -- than the buffer to allow for outward growth.
  local seed_rect = {
    x=buffer.left * 2,
    y=buffer.top * 2,
    w=max( size.w - buffer.left * 2 - buffer.right * 4, 2 ),
    h=max( size.h - buffer.top * 2 - buffer.bottom * 2, 2 )
  }
  local quadrants = {
    { -- upper left
      x=seed_rect.x,
      y=seed_rect.y,
      w=seed_rect.w // 2,
      h=seed_rect.h // 2
    }, { -- upper right
      x=seed_rect.x + seed_rect.w // 2,
      y=seed_rect.y,
      w=seed_rect.w // 2,
      h=seed_rect.h // 2
    }, { -- lower left
      x=seed_rect.x,
      y=seed_rect.y + seed_rect.h // 2,
      w=seed_rect.w // 2,
      h=seed_rect.h // 2
    }, { -- lower right
      x=seed_rect.x + seed_rect.w // 2,
      y=seed_rect.y + seed_rect.h // 2,
      w=seed_rect.w // 2,
      h=seed_rect.h // 2
    }
  }
  local land_squares = 0
  for i = 1, 1000 do
    for _, rect in ipairs( quadrants ) do
      land_squares = land_squares +
                         generate_continent_in_rect( options,
                                                     rect )
    end
    local density = total_land_density( land_squares )
    local target = options.land_density
    if density > target then
      log.info( 'land density actual/target: ' ..
                    percent( density ) .. '/' ..
                    percent( target ) )
      break
    end
  end
  clear_buffer_area( buffer )
  if options.remove_Xs then remove_Xs() end
  remove_islands()
  create_arctic( options )
  assign_dry_ground_types()
  -- We need to have already created the rivers before this.
  assign_wet_ground_types()

  -- These need to be done before the rivers since rivers don't
  -- seem to flow on hills/mountains in the OG.
  create_hills( options )
  create_mountains( options )

  create_rivers( options )

  forest_cover()

  local placement_seed = set_random_placement_seed()
  distribute_prime_resources( placement_seed )
  distribute_lost_city_rumors( placement_seed )
end

-----------------------------------------------------------------
-- Testing
-----------------------------------------------------------------
---- FIXME move this
local function generate_testing_land()
  on_all( function( coord, square )
    local main = { x=coord.x, y=coord.y - 2 }
    if main.x > 5 and main.x < 50 and main.y > 5 and main.y < 60 then
      square.surface = 'land'
      square.ground = (main.y // 6) % 9 -- "plains"
      if main.x // 5 % 2 == 1 then square.overlay = 'forest' end
      if main.x >= 40 and main.x <= 44 then
        if main.y // 6 % 2 == 1 then
          square.overlay = 'hills'
        else
          square.overlay = 'mountains'
        end
      end
    else
      square.surface = 'water'
    end
    if coord.y < 4 or coord.y > 65 then
      square.surface = 'land'
      square.ground = math.random( 0, 8 )
      if math.random( 1, 5 ) == 1 then
        square.overlay = math.random( 0, 3 )
      end
    end
  end )

  local placement_seed = set_random_placement_seed()
  distribute_prime_resources( placement_seed )
  distribute_lost_city_rumors( placement_seed )

  -- on_all( function( coord, square )
  --   if square.surface == "land" then
  --     square.lost_city_rumor = true
  --     square.road = true
  --   end
  -- end )
end

local function generate_circles_land( options )
  local circles = {
    { x=10, y=10, r=3 }, { x=46, y=10, r=3 },
    { x=10, y=60, r=3 }, { x=46, y=60, r=3 }, --
    { x=28, y=20, r=3 }, { x=28, y=50, r=3 },
    { x=10, y=35, r=3 }, { x=46, y=35, r=3 }
  }
  on_all( function( coord, square )
    local in_circle = false
    for _, circle in ipairs( circles ) do
      local dist = math.sqrt( (coord.x - circle.x) ^ 2 +
                                  (coord.y - circle.y) ^ 2 )
      if dist <= circle.r then
        in_circle = true
        break
      end
    end
    if not in_circle then return end
    square.surface = 'land'
    square.ground = 'grassland'
  end )
end

-- FIXME move this
local function generate_battlefield()
  local size = world_size()
  on_all( function( coord, square )
    square.surface = 'land'
    square.ground = 'grassland'
  end )
end

-- FIXME move this
local function generate_half_land()
  local size = world_size()
  on_all( function( coord, square )
    if coord.x == size.w // 2 then
      square.surface = 'land'
      square.ground = 'desert'
    elseif coord.x < size.w // 2 then
      square.surface = 'land'
      square.ground = 'grassland'
    end
  end )
end

-- This will clear all resources and lost city rumors and redis-
-- tribute them (with a random seed). This is useful when cre-
-- ating a map with the map editor where you'd like to have the
-- standard distribution algorithm applied after the map is fin-
-- ished.
function M.redistribute_resources( placement_seed )
  on_all( function( coord, square )
    square.lost_city_rumor = false
    square.ground_resource = nil
    square.forest_resource = nil
  end )
  placement_seed = placement_seed or set_random_placement_seed()
  distribute_prime_resources( placement_seed )
  distribute_lost_city_rumors( placement_seed )
  ROOT_TS.map_updater:redraw()
end

-- This will recompute the distribution of resources but with the
-- same placement seed.
function M.refresh_resources()
  M.redistribute_resources( ROOT.terrain:placement_seed() )
end

function M.remake_rivers( options )
  -- FIXME: merge these options with the default ones.
  options = options or M.default_options()
  on_all( function( coord, square ) square.river = nil end )
  create_rivers( options )
  ROOT_TS.map_updater:redraw()
end

-- Note that this will not regenerate the indian dwellings.
function M.regenerate_native_land_partitions(
    paint_map_by_partition )
  local size = world_size()
  local NUM_TRIBES = 8
  local partitions = partition.generate( size, NUM_TRIBES,
                                         is_coord_land )
  if paint_map_by_partition then
    paint_native_land_partitions( partitions )
  end
  ROOT_TS.map_updater:redraw()
end

-----------------------------------------------------------------
-- Map Generator
-----------------------------------------------------------------
function M.regen( options )
  M.generate( options )
  ROOT_TS.map_updater:redraw()
end

local function generate( options )
  options = options or {}
  -- Merge the options with the default ones so that any missing
  -- fields will have their default values.
  -- TODO: move this into a dedicated options module that can be
  -- shared.
  for k, v in pairs( M.default_options() ) do
    if options[k] == nil then options[k] = v end
  end
  options = secure_options( options )

  log.info( 'generating map...' );

  reset_terrain( options )

  generate_proto_squares()

  if options.type == 'battlefield' then
    generate_battlefield( options )
    return
  elseif options.type == 'land-partition' then
    generate_land( options )
    create_sea_lanes()
    M.regenerate_native_land_partitions( true )
    return
  elseif options.type == 'half_and_half' then
    generate_half_land()
  elseif options.type == 'testing' then
    generate_testing_land()
  else
    generate_land( options )
  end

  create_sea_lanes()

  create_indian_villages( options )
end

function M.generate( ... )
  timer.log_time( 'map generation', generate, ... )
end

return M
