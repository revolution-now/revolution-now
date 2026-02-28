-- This lambda will run on multiple savs and will gather some
-- statistics on the distribution of inland water tiles.
-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local Q = require( 'lib.query' )
local json = require( 'moon.json' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local format = string.format
local insert = table.insert

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function printfln( ... ) print( string.format( ... ) ) end

local PLOTS_DIR = 'lakes/empirical'

-----------------------------------------------------------------
-- Data.
-----------------------------------------------------------------
-- The data here spans multiple savs.
local D = {
  total_tiles=0,
  total_savs=0,
  total_land=0,
  total_water=0,
  total_inland_water_tiles=0,
  total_water_tiles_adjacent_to_land=0,
  total_land_tiles_adjacent_to_water=0,
  total_land_tiles_adjacent_to_sealane=0,
  metric=0,
}

--
--       1          total_inland_water_tiles
--  ----------   * ------------------------- ~ .37
--          1.5             total_land
--   density
--
--       1          total_inland_water_tiles
--  ----------   * ------------------------- ~ .37
--          2.5           total_tiles
--   density
--
--  For C++ to compute target:
--                                                          2.5
--    total_inland_water_tiles ~ S * total_tiles * density
--
--    where S is a factor that depends on perlin scale.
--
local function compute_metric(total_land,
                              total_inland_water_tiles,
                              land_density )
  return (1 / land_density) ^ 1.5 * total_inland_water_tiles /
             total_land
end

-----------------------------------------------------------------
-- Lambda.
-----------------------------------------------------------------
local function terrain_at( json_o, tile )
  return Q.terrain_at( json_o, tile )
end

local function lambda( json_o )
  D.total_savs = D.total_savs + 1

  T = {}
  T.total_tiles = 0
  T.total_savs = 0
  T.total_land = 0
  T.total_water = 0
  T.total_inland_water_tiles = 0
  T.total_water_tiles_adjacent_to_land = 0
  T.total_land_tiles_adjacent_to_water = 0
  T.total_land_tiles_adjacent_to_sealane = 0
  T.metric = 0

  Q.on_all_tiles( function( tile )
    if tile.y == 1 or tile.y == 70 then return end
    D.total_tiles = D.total_tiles + 1
    T.total_tiles = T.total_tiles + 1
    local square = terrain_at( json_o, tile )
    if square.surface == 'land' then
      D.total_land = D.total_land + 1
      T.total_land = T.total_land + 1
      local has_water_surrounding = false
      local has_sealane_surrounding = false
      Q.on_surrounding_tiles( tile, function( coord )
        local adjacent = terrain_at( json_o, coord )
        if adjacent.surface == 'water' then
          has_water_surrounding = true
          if Q.is_water_region_1( json_o, coord ) then
            has_sealane_surrounding = true
          end
        end
        if coord.y == 1 or coord.y == 70 then
          has_water_surrounding = true
          has_sealane_surrounding = true
        end
      end )
      if has_water_surrounding then
        D.total_land_tiles_adjacent_to_water =
            D.total_land_tiles_adjacent_to_water + 1
      end
      if has_sealane_surrounding then
        D.total_land_tiles_adjacent_to_sealane =
            D.total_land_tiles_adjacent_to_sealane + 1
        T.total_land_tiles_adjacent_to_sealane =
            T.total_land_tiles_adjacent_to_sealane + 1
      end
      return
    end
    -- This is a water tile.
    D.total_water = D.total_water + 1
    local region_id = Q.region_id( json_o, tile )
    -- Region ID 0 is reserved for the outter tiles that are not
    -- visible on the game map, and we should not be iterating
    -- over those here.
    assert( region_id ~= 0 )
    assert( region_id >= 1 and region_id <= 15 )
    local is_inland_lake = (region_id ~= 1)
    if is_inland_lake then
      D.total_inland_water_tiles = D.total_inland_water_tiles + 1
      T.total_inland_water_tiles = T.total_inland_water_tiles + 1
    end
    local has_land_surrounding = false
    Q.on_surrounding_tiles( tile, function( coord )
      local adjacent = terrain_at( json_o, coord )
      if adjacent.surface == 'land' then
        has_land_surrounding = true
      end
    end )
    if has_land_surrounding then
      D.total_water_tiles_adjacent_to_land =
          D.total_water_tiles_adjacent_to_land + 1
    end
  end )

  local land_density = T.total_land / T.total_tiles
  T.metric = compute_metric( T.total_land,
                             T.total_inland_water_tiles,
                             land_density )
  D.metric = D.metric + T.metric
end

local DATA_KEY_ORDER = {
  'total_savs', --
  'total_land', --
  'total_water', --
  'total_inland_water_tiles_per_map', --
  'metric_local', --
  'metric_global', --
}

local function finished( mode )
  local o = { __key_order=DATA_KEY_ORDER }
  o.total_savs = D.total_savs
  o.total_land = D.total_land
  o.total_water = D.total_water
  o.total_inland_water_tiles_per_map =
      D.total_inland_water_tiles / D.total_savs

  local land_density = D.total_land / D.total_tiles
  o.metric_global = compute_metric( D.total_land,
                                    D.total_inland_water_tiles,
                                    land_density )
  o.metric_local = D.metric / D.total_savs

  do
    local lakes_file = format( '%s/%s.lakes.json', PLOTS_DIR,
                               mode )
    printfln( 'writing file %s...', lakes_file )
    local f<close> = assert( io.open( lakes_file, 'w' ) )
    local emit = function( bit ) f:write( bit ) end
    json.write( o, 2, emit )
  end
end

-----------------------------------------------------------------
-- Collection stage.
-----------------------------------------------------------------
-- This gets run once after all modes are run (which happen in
-- separate processes) to collect all of their results.
local function collect()
  print( 'collecting...' )
  local MODES = {
    'ttmm', 'mmmm', 'bbmm', 'mtmm', 'mbmm', 'tmmm', 'bmmm',
    'tbmm', 'btmm', 'new',
  }
  local o = {}
  o.modes = {}
  o.modes.__key_order = MODES
  for _, mode in ipairs( MODES ) do
    local lakes_file = format( '%s/%s.lakes.json', PLOTS_DIR,
                               mode )
    local f<close> = assert( io.open( lakes_file, 'r' ) )
    local mode_json = json.read( f:read( '*all' ) )
    local results = assert( mode_json )
    o.modes[mode] = results
    o.modes[mode].__key_order = DATA_KEY_ORDER
  end
  local collected_json_file =
      format( '%s/lakes.json', PLOTS_DIR )
  printfln( 'writing %s...', collected_json_file )
  local json_out<close> = assert(
                              io.open( collected_json_file, 'w' ) )
  json.write( o, 2, function( bit ) json_out:write( bit ) end )
end

-----------------------------------------------------------------
-- Lambda.
-----------------------------------------------------------------
return { lambda=lambda, finished=finished, collect=collect }