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
    -- The original game seems to have a land density of about
    -- 25% on normal map generation settings. However we will put
    -- it slightly lower because it tends to end up slightly
    -- higher than the target.
    land_density=.22,
    remove_Xs=false,
    brush='cross'
  }
end

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

local function random_point_in_rect( rect )
  local size = { w=rect.w, h=rect.h }
  local x = math.random( 0, rect.w - 1 ) + rect.x
  local y = math.random( 0, rect.h - 1 ) + rect.y
  return { x=x, y=y }
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
function M.initial_ships_pos()
  local size = map_gen.world_size()
  local y = size.h / 2
  local x = size.w - 1
  while map_gen.at{ x=x, y=y }.sea_lane do x = x - 1 end
  return { [e.nation.dutch]={ x=x + 1, y=y } }
end

-----------------------------------------------------------------
-- Terrain Modification
-----------------------------------------------------------------
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
local function reset_terrain( options )
  map_gen.reset_terrain( options.world_size )
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

local function is_land( square )
  return square.surface == e.surface.land
end

local function is_water( square )
  return square.surface == e.surface.water
end

local function right_most_land_square_in_row( row )
  local size = map_gen.world_size()
  for x = size.w - 1, 0, -1 do
    local coord = { x=x, y=row }
    local square = map_gen.at( coord )
    if square.surface == e.surface.land then return coord end
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

local function square_exists( square )
  local size = map_gen.world_size()
  return
      square.x >= 0 and square.y >= 0 and square.x < size.w and
          square.y < size.h
end

local function filter_existing_squares( squares )
  local exists = {}
  for _, val in ipairs( squares ) do
    if square_exists( val ) then append( exists, val ) end
  end
  return exists
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
        block_edge = filter_existing_squares( block_edge )
        coord.x = coord.x - 1
      until #block_edge > 0
      for _, s in ipairs( block_edge ) do
        -- Walk from the right to the left until we either get to
        -- the left edge of the map or we find an ocean square
        -- with no sea lane, which means we've already cleared
        -- the remainder as part of another row, so we can stop.
        for x = s.x, 0, -1 do
          local coord = { x=x, y=s.y }
          local square = map_gen.at( coord )
          if square.surface == e.surface.water and
              not square.sea_lane then break end
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
  curr_sea_lane_width = sea_lane_width( closest_row )
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

-- The original game checks to see if there is at least one land
-- square within *two* tiles from the water square in order to
-- allow placing a fish there. However, there isn't any point to
-- going beyond a one-tile distance in the check because those
-- are the only squares that will be accessible to any colony. So
-- here we will just do one square.
local function can_place_fish( coord )
  local squares = filter_existing_squares(
                      surrounding_squares_3x3( coord ) )
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
-- Ground Terrain Assignment
-----------------------------------------------------------------
local function assign_ground_types()
  local size = map_gen.world_size()
  on_all( function( coord, square )
    if is_water( square ) then return end
    if coord.y == 0 or coord.y == size.h - 1 then
      square.ground = e.ground_terrain.arctic
      return
    end
    -- TODO
    square.ground = random_list_elem{
      e.ground_terrain.plains, e.ground_terrain.grassland,
      e.ground_terrain.prairie, e.ground_terrain.marsh,
      e.ground_terrain.savannah, e.ground_terrain.desert,
      e.ground_terrain.swamp
    }
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
--
-- This function will find all such Xs and will remove them by
-- choosing one of the four tiles at random and flipping its sur-
-- face type.
local function remove_Xs()
  local size = map_gen.world_size()
  on_all( function( coord, square )
    if coord.y < size.h - 1 and coord.x < size.w - 1 then
      local square_right = map_gen.at{ x=coord.x + 1, y=coord.y }
      local square_down = map_gen.at{ x=coord.x, y=coord.y + 1 }
      local square_diag = map_gen.at{
        x=coord.x + 1,
        y=coord.y + 1
      }
      if is_land( square ) and is_water( square_right ) and
          is_water( square_down ) and is_land( square_diag ) then
        square_diag.surface = e.surface.water
      end
      if is_water( square ) and is_land( square_right ) and
          is_land( square_down ) and is_water( square_diag ) then
        square_diag.surface = e.surface.land
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
  local size = map_gen.world_size()
  on_all( function( coord, square )
    if coord.y > 0 and coord.y < size.h - 1 then
      local surrounding = surrounding_squares_3x3( coord )
      surrounding = filter_existing_squares( surrounding )
      local has_land = false
      for _, square in ipairs( surrounding ) do
        if map_gen.at( square ).surface == e.surface.land then
          has_land = true
          break
        end
      end
      if not has_land then
        -- We have an island.
        square.surface = e.surface.water
      end
    end
  end )
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
  local size = map_gen.world_size()
  local stretch_x = math.min( seed_square.x,
                              size.w - seed_square.x )
  local stretch_y = math.min( seed_square.y,
                              size.h - seed_square.y )
  local stretch = scale * math.min( stretch_x, stretch_y )
  return { x=stretch, y=stretch }
end

local function set_land_if_needed( coord )
  if not square_exists( coord ) then return 0 end
  if map_gen.at( coord ).surface == e.surface.land then
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
  single=function( x, y )
    local count = 0
    count = count + set_land_if_needed{ x=x, y=y }
    return count
  end,
  cross=function( x, y )
    local count = 0
    count = count + set_land_if_needed{ x=x, y=y }
    count = count + set_land_if_needed{ x=x - 1, y=y }
    count = count + set_land_if_needed{ x=x + 1, y=y }
    count = count + set_land_if_needed{ x=x, y=y - 1 }
    count = count + set_land_if_needed{ x=x, y=y + 1 }
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
    land_squares = land_squares + brush( square.x, square.y )
  end
  return land_squares
end

-----------------------------------------------------------------
-- Land Generation
-----------------------------------------------------------------
local function total_land_density( land_count )
  local size = map_gen.world_size()
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

local function generate_land( options )
  local size = map_gen.world_size()
  -- The buffer zone will have no land in it, so it should be
  -- relatively small.
  local buffer = { top=1, bottom=1, left=4, right=3 }
  -- Seeds will be chosen from this rect, which is a bit smaller
  -- than the buffer to allow for outward growth.
  local seed_rect = {
    x=buffer.left * 2,
    y=buffer.top * 2,
    w=(size.w - buffer.left * 2 - buffer.right * 4),
    h=(size.h - buffer.top * 2 - buffer.bottom * 2)
  }
  local quadrants = {
    upper_left={
      x=seed_rect.x,
      y=seed_rect.y,
      w=seed_rect.w // 2,
      h=seed_rect.h // 2
    },
    upper_right={
      x=seed_rect.x + seed_rect.w // 2,
      y=seed_rect.y,
      w=seed_rect.w // 2,
      h=seed_rect.h // 2
    },
    lower_left={
      x=seed_rect.x,
      y=seed_rect.y + seed_rect.h // 2,
      w=seed_rect.w // 2,
      h=seed_rect.h // 2
    },
    lower_right={
      x=seed_rect.x + seed_rect.w // 2,
      y=seed_rect.y + seed_rect.h // 2,
      w=seed_rect.w // 2,
      h=seed_rect.h // 2
    }
  }
  local land_squares = 0
  for i = 1, 1000 do
    for quadrant_name, rect in pairs( quadrants ) do
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
  create_arctic()
  assign_ground_types()
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
function M.regen( options )
  M.generate( options )
  render_terrain.redraw()
end

function M.generate( options )
  options = options or {}
  -- Merge the options with the default ones so that any missing
  -- fields will have their default values.
  -- TODO: move this into a dedicated options module that can be
  -- shared.
  for k, v in pairs( M.default_options() ) do
    if options[k] == nil then options[k] = v end
  end
  options = secure_options( options )

  reset_terrain( options )

  generate_land( options )
  -- generate_testing_land()

  create_sea_lanes()

  create_indian_villages()
end

return M
