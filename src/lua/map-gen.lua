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

local dist = require( 'map-gen.classic.resource-dist' )

-----------------------------------------------------------------
-- Constants
-----------------------------------------------------------------
-- The maps in the original game are 58x72, but the tiles on the
-- edges are not visible, so effectively we have 56x70.
local WORLD_SIZE = { w=56, h=70 }

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
  local y = size.h / 2
  local x = size.w - 1
  while map_gen.at{ x=x, y=y }.sea_lane do x = x - 1 end
  return { x=x, y=y }
end

local function unit_type( type, base_type )
  if base_type == nil then
    return unit_composer.UnitComposition.create_with_type_obj(
               utype.UnitType.create( type ) )
  else
    return unit_composer.UnitComposition.create_with_type_obj(
               utype.UnitType.create_with_base( type, base_type ) )
  end
end

local function create_initial_ships()
  -- Dutch ------------------------------------------------------
  local nation = e.nation.dutch
  local coord = map_gen.initial_ship_pos()
  local merchantman = unit_type( e.unit_type.merchantman )
  local soldier = unit_type( e.unit_type.soldier )
  local pioneer = unit_type( e.unit_type.pioneer )

  local merchantman_unit = ustate.create_unit_on_map( nation,
                                                      merchantman,
                                                      coord )
  ustate.create_unit_in_cargo( nation, soldier,
                               merchantman_unit:id() )
  ustate.create_unit_in_cargo( nation, pioneer,
                               merchantman_unit:id() )
end

-----------------------------------------------------------------
-- Terrain Modification
-----------------------------------------------------------------
local function set_land( coord )
  local square = map_gen.at( coord )
  square.surface = e.surface.land
  square.ground = random_list_elem{
    e.ground_terrain.plains, e.ground_terrain.grassland,
    e.ground_terrain.prairie, e.ground_terrain.marsh,
    e.ground_terrain.savannah, e.ground_terrain.desert,
    e.ground_terrain.swamp
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
  local square = map_gen.at( coord )
  square.sea_lane = true
end

-- This will create a new empty map set all squares to water.
local function reset_terrain()
  map_gen.reset_terrain( WORLD_SIZE )
  on_all( set_water )
end

-- row is zero-based.
local function row_has_land( row )
  local size = map_gen.world_size()
  for x = 0, size.w - 1 do
    local square = map_gen.at{ x=x, y=row }
    if square.surface == e.surface.land then return true end
  end
  return false
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
--
-- This function will find all such Xs and will remove them by
-- choosing one of the four tiles at random and flipping its sur-
-- face type.
local function remove_some_Xs()
  -- TODO
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

-----------------------------------------------------------------
-- Forest Generation
-----------------------------------------------------------------
-- TODO: tweak the density of forest to match the original game.
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
local function clear_buffer_area( buffer )
  local size = map_gen.world_size()
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
          if square.sea_lane then
            square.sea_lane = false
          end
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
      if square.sea_lane then square.sea_lane = false end
    end
  end
  for y = size.h - 4, size.h - 1 do
    for x = 0, size.w - 1 do
      local coord = { x=x, y=y }
      local square = map_gen.at( coord )
      if square.sea_lane then square.sea_lane = false end
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
    if row_has_land( y ) then
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
    if row_has_land( y ) then
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
    if row_has_land( y ) then
      curr_sea_lane_width = sea_lane_width( y )
    else
      -- Clear the sea lane and make it have the width of the row
      -- above it.
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
-- Villages
-----------------------------------------------------------------
local function create_indian_villages()
  -- Note that indian villages never seem to be placed on top of
  -- the lost city rumors, though they can be placed on top of
  -- natural resources.
end

-----------------------------------------------------------------
-- Lost City Rumors
-----------------------------------------------------------------
local function distribute_lost_city_rumors( placement_seed )
  local size = map_gen.world_size()
  local coords = dist.compute_lost_city_rumors( size,
                                                placement_seed )
  for _, coord in ipairs( coords ) do
    local square = map_gen.at( coord )
    if square.surface == e.surface.land and square.ground ~=
        e.ground_terrain.arctic then
      square.lost_city_rumor = true
    end
  end
  log.debug( 'lost city rumor density: ' ..
                 tostring( #coords / (size.w * size.h) ) )
end

-----------------------------------------------------------------
-- Prime Resource Generation
-----------------------------------------------------------------
local RESOURCES_GROUND = {
  [e.ground_terrain.arctic]=nil,
  [e.ground_terrain.desert]=e.natural_resource.oasis,
  [e.ground_terrain.grassland]=e.natural_resource.tobacco,
  [e.ground_terrain.marsh]=e.natural_resource.minerals,
  [e.ground_terrain.plains]=e.natural_resource.wheat,
  [e.ground_terrain.prairie]=e.natural_resource.cotton,
  [e.ground_terrain.savannah]=e.natural_resource.sugar,
  [e.ground_terrain.swamp]=e.natural_resource.minerals,
  [e.ground_terrain.tundra]=e.natural_resource.minerals
}

-- If a forest tile has a prime resource then this table will
-- give the resource that it would have based on the ground tile
-- in accordance with the original game's rules.
local RESOURCES_FOREST = {
  [e.ground_terrain.arctic]=nil,
  [e.ground_terrain.desert]=e.natural_resource.oasis,
  [e.ground_terrain.grassland]=e.natural_resource.tree,
  [e.ground_terrain.marsh]=e.natural_resource.minerals,
  [e.ground_terrain.plains]=e.natural_resource.beaver,
  [e.ground_terrain.prairie]=e.natural_resource.deer,
  [e.ground_terrain.savannah]=e.natural_resource.tree,
  [e.ground_terrain.swamp]=e.natural_resource.minerals,
  [e.ground_terrain.tundra]=e.natural_resource.deer
}

-- Checks if there is at least one land square within two tiles
-- from the square.
local function can_place_fish( coord )
  local squares = filter_existing_squares(
                      surrounding_squares_5x5( coord ) )
  for _, coord in ipairs( squares ) do
    if map_gen.at( coord ).surface == e.surface.land then
      return true
    end
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
  if square.surface == e.surface.water then
    if can_place_fish( coord ) then
      square.ground_resource = e.natural_resource.fish
    end
    return
  end
  if square.overlay == e.land_overlay.hills then
    square.ground_resource = e.natural_resource.ore
    return
  end
  if square.overlay == e.land_overlay.mountains then
    square.ground_resource = e.natural_resource.silver
    return
  end
  -- We apply the ground resource whether or not the tile has
  -- forest (see above).
  local resource = RESOURCES_GROUND[square.ground]
  if resource then square.ground_resource = resource end
end

local function add_forest_prime_resource( coord, square )
  assert( square.overlay == e.land_overlay.forest )
  assert( square.surface ~= e.surface.water )
  local resource = RESOURCES_FOREST[square.ground]
  if resource then square.forest_resource = resource end
end

local function distribute_prime_ground_resources( y_offset )
  local size = map_gen.world_size()
  local coords = dist.compute_prime_ground_resources( size,
                                                      y_offset )
  for _, coord in ipairs( coords ) do
    local square = map_gen.at( coord )
    add_ground_prime_resource( coord, square )
  end
  log.debug( 'prime ground resources density: ' ..
                 tostring( #coords / (size.w * size.h) ) )
end

local function distribute_prime_forest_resources( y_offset )
  local size = map_gen.world_size()
  local coords = dist.compute_prime_forest_resources( size,
                                                      y_offset )
  for _, coord in ipairs( coords ) do
    local square = map_gen.at( coord )
    if square.surface == e.surface.land and square.overlay ==
        e.land_overlay.forest then
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
  map_gen.terrain_state():set_placement_seed( placement_seed )
  return placement_seed
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
  render_terrain.redraw()
end

-- This will recompute the distribution of resources but with the
-- same placement seed.
function M.refresh_resources()
  M.redistribute_resources(
      map_gen.terrain_state():placement_seed() )
end

-----------------------------------------------------------------
-- Land Generation
-----------------------------------------------------------------
local function generate_land()
  local size = map_gen.world_size()
  local buffer = { top=2, bottom=2, left=4, right=3 }
  local initial_square = {
    x=size.w - buffer.left * 2,
    y=size.h / 2
  }
  local initial_area = math.random( 5, 50 )
  generate_continent( initial_square, initial_area )
  for i = 1, 8 do
    local square = random_point_in_rect(
                       {
          x=buffer.left,
          y=buffer.top,
          w=size.w - buffer.right - buffer.left,
          h=size.h - buffer.bottom - buffer.top
        } )
    local area = math.random( 10, 300 )
    generate_continent( square, area )
  end
  clear_buffer_area( buffer )
  remove_some_Xs()
  create_arctic()
  forest_cover()

  local placement_seed = set_random_placement_seed()
  distribute_prime_resources( placement_seed )
  distribute_lost_city_rumors( placement_seed )
end

-----------------------------------------------------------------
-- Testing
-----------------------------------------------------------------
local function generate_testing_land()
  on_all( function( coord, square )
    local main = { x=coord.x, y=coord.y - 2 }
    if main.x > 5 and main.x < 50 and main.y > 5 and main.y < 60 then
      square.surface = e.surface.land
      square.ground = (main.y // 6) % 9 -- e.ground_terrain.plains
      if main.x // 5 % 2 == 1 then
        square.overlay = e.land_overlay.forest
      end
      if main.x >= 40 and main.x <= 44 then
        if main.y // 6 % 2 == 1 then
          square.overlay = e.land_overlay.hills
        else
          square.overlay = e.land_overlay.mountains
        end
      end
    else
      square.surface = e.surface.water
    end
    if coord.y < 4 or coord.y > 65 then
      square.surface = e.surface.land
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
  --   if square.surface == e.surface.land then
  --     square.lost_city_rumor = true
  --     square.road = true
  --   end
  -- end )
end

-----------------------------------------------------------------
-- Map Generator
-----------------------------------------------------------------
function M.generate()
  reset_terrain()

  generate_land()
  -- generate_testing_land()

  create_sea_lanes()

  create_initial_ships()

  create_indian_villages()

end

return M
