-- This lambda will run on multiple savs and will gather some
-- statistics on the distribution of ground terrains.
-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local Q = require( 'lib.query' )
local json = require( 'moon.json' )
local plot = require( 'moon.plot' )
local tbl = require( 'moon.tbl' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local insert = table.insert
local format = string.format
local max = math.max
local min = math.min
local abs = math.abs
local log = math.log
local deep_copy = assert( tbl.deep_copy )

-----------------------------------------------------------------
-- Constants.
-----------------------------------------------------------------
local DESERT_DENSITY_BUCKET_SIZE = .0001

local WETNESS_ACCUMULATION = 1 / 3
local WETNESS_CONSUMPTION = .20

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function printfln( ... ) print( string.format( ... ) ) end

local function clamp( n, l, h )
  if n < l then return l end
  if n > h then return h end
  return n
end

local PLOTS_DIR = 'biomes/empirical'

local function ocean_adjacent( J, tile )
  local has = false
  Q.on_surrounding_tiles_cardinal( tile, function( coord )
    local adjacent = Q.terrain_at( J, coord )
    if adjacent.surface == 'water' then has = true end
  end )
  return has
end

local function river_adjacent( J, tile )
  local has = false
  Q.on_surrounding_tiles_cardinal( tile, function( coord )
    if Q.has_river( J, coord ) then has = true end
  end )
  return has
end

local function ocean_adjacent_side( J, tile )
  local has = false
  Q.on_surrounding_tiles_sides( tile, function( coord )
    local adjacent = Q.terrain_at( J, coord )
    if adjacent.surface == 'water' then has = true end
  end )
  return has
end

local function river_adjacent_side( J, tile )
  local has = false
  Q.on_surrounding_tiles_sides( tile, function( coord )
    if Q.has_river( J, coord ) then has = true end
  end )
  return has
end

local function compute_wetness_full( J )
  local m = {}
  for y = 1, 70 do
    insert( m, {} )
    for _ = 1, 56 do insert( m[y], 0 ) end
  end
  -- There is no need to change MAX_WETNESS because all other ab-
  -- solute quantities are scaled by it, and so it will only lead
  -- to a change in the overall scale of the wetness, but that
  -- will be normalized away anyway.
  local MAX_WETNESS = 1.0
  local OCEAN_WETNESS = MAX_WETNESS * WETNESS_ACCUMULATION
  local wetness
  local function apply( y, x )
    assert( wetness )
    -- Note that MAX_WETNESS is only the max for a given pass;
    -- the actual value can go above that because each pass con-
    -- tributes additively.
    m[y][x] = m[y][x] + wetness
    local square = Q.terrain_at( J, { x=x, y=y } )
    if square.surface == 'water' then
      wetness = min( wetness + OCEAN_WETNESS, MAX_WETNESS )
    else
      wetness = wetness * WETNESS_CONSUMPTION
    end
  end
  for y = 1, 70 do
    wetness = MAX_WETNESS
    for x = 1, 56 do apply( y, x ) end
    wetness = MAX_WETNESS
    for x = 56, 1, -1 do apply( y, x ) end
  end
  return m
end

-----------------------------------------------------------------
-- Data.
-----------------------------------------------------------------
-- The data here spans multiple savs.
local D = {
  savs=0,
  land=0,
  land_ocean_adjacent=0,
  land_ocean_adjacent_side=0,
  land_wetness=0,

  -- key=row
  land_by_row={},
  land_by_col={},

  -- key=row
  land_by_row_dry={},
  land_by_row_wet={},

  -- key1=ground_type, key2=row, value=count
  ground_per_row={},
  ground_per_col={},

  -- key=biome, val=count
  with_river_adjacent={},
  with_ocean_adjacent={},
  with_river_adjacent_side={},
  with_ocean_adjacent_side={},
  wetness={},

  -- value={ count=N, metric=? }
  swamp={
    count=0,
    with_swamp_adjacent=0,
    with_marsh_adjacent=0,
    with_swamp_or_marsh_adjacent=0,
    with_river_adjacent=0,
    with_ocean_adjacent=0,
    with_river_or_ocean_adjacent=0,
    with_any_adjacent=0,
    with_river=0,
  },

  marsh={
    count=0,
    with_swamp_adjacent=0,
    with_marsh_adjacent=0,
    with_swamp_or_marsh_adjacent=0,
    with_river_adjacent=0,
    with_ocean_adjacent=0,
    with_river_or_ocean_adjacent=0,
    with_any_adjacent=0,
    with_river=0,
  },

  -- Terrain adjacency.
  -- key=ground_type,
  -- val={
  --   count=N,
  --   adjacency_count=N,
  --   surrounding_land_on_row,
  -- }
  adjacency={},

  -- Histogram of largest distance from equator where there is
  -- savannah. This is used to estimate the temperature distribu-
  -- tion on the `new` mode. key=distance, val=count.
  max_savannah_row={},

  -- Histogram of the density of desert in the middle two rows.
  -- This is used to estimate the temperature distribution on the
  -- `new` mode. key=density bucket, val=count.
  desert_density_center={},
}

local BIOME_ORDERING = {
  'savannah', --
  'grassland', --
  'tundra', --
  'plains', --
  'prairie', --
  'desert', --
  'swamp', --
  'marsh', --
  'arctic', --
}

local BIOME_ORDERING_NON_ARCTIC = {
  'savannah', --
  'grassland', --
  'tundra', --
  'plains', --
  'prairie', --
  'desert', --
  'swamp', --
  'marsh', --
}

local WET_DRY_TYPE = {
  savannah='wet', --
  grassland='dry', --
  tundra='dry', --
  plains='dry', --
  prairie='dry', --
  desert='dry', --
  swamp='wet', --
  marsh='wet', --
  arctic='dry', --
}

-----------------------------------------------------------------
-- Lambda.
-----------------------------------------------------------------
local function terrain_at( J, tile )
  -- Can intercept here.
  return Q.terrain_at( J, tile )
  -- local t = Q.terrain_at( json, tile )
  -- return {
  --   surface=t.surface,
  --   ground=assert( BIOME_ORDERING[math.random( #BIOME_ORDERING - 1 )] ),
  -- }
end

local function lambda( J )
  D.savs = D.savs + 1

  local wetness_full = assert( compute_wetness_full( J ) )

  for _, ground_type in ipairs( BIOME_ORDERING ) do
    D.ground_per_row[ground_type] =
        D.ground_per_row[ground_type] or {}
    D.ground_per_col[ground_type] =
        D.ground_per_col[ground_type] or {}
    D.adjacency[ground_type] = D.adjacency[ground_type] or {}
    D.with_ocean_adjacent[ground_type] =
        D.with_ocean_adjacent[ground_type] or 0
    D.with_river_adjacent[ground_type] =
        D.with_river_adjacent[ground_type] or 0
    D.with_ocean_adjacent_side[ground_type] =
        D.with_ocean_adjacent_side[ground_type] or 0
    D.with_river_adjacent_side[ground_type] =
        D.with_river_adjacent_side[ground_type] or 0
    D.wetness[ground_type] = D.wetness[ground_type] or 0
    local A = assert( D.adjacency[ground_type] )
    A.count = A.count or 0
    A.adjacency_count = A.adjacency_count or 0
  end

  Q.on_all_tiles( function( tile )
    for _, ground_type in ipairs( BIOME_ORDERING ) do
      D.land_by_row[tile.y] = D.land_by_row[tile.y] or 0
      D.land_by_col[tile.x] = D.land_by_col[tile.x] or 0
      D.land_by_row_dry[tile.y] = D.land_by_row_dry[tile.y] or 0
      D.land_by_row_wet[tile.y] = D.land_by_row_wet[tile.y] or 0
      do
        local o = assert( D.ground_per_row[ground_type] )
        o[tile.y] = o[tile.y] or 0
      end
      do
        local o = assert( D.ground_per_col[ground_type] )
        o[tile.x] = o[tile.x] or 0
      end
    end
  end )

  Q.on_all_tiles( function( tile )
    for _, ground_type in ipairs( BIOME_ORDERING ) do
      D.adjacency[ground_type] = D.adjacency[ground_type] or {}
      local o = D.adjacency[ground_type]
      o.surrounding_land_on_row = o.surrounding_land_on_row or {}
      o.surrounding_land_on_row[tile.y] =
          o.surrounding_land_on_row[tile.y] or 0
    end
  end )

  Q.on_all_tiles( function( tile )
    local terrain = terrain_at( J, tile )

    D.land_by_row[tile.y] = D.land_by_row[tile.y] or 0
    D.land_by_col[tile.x] = D.land_by_col[tile.x] or 0
    D.land_by_row_dry[tile.y] = D.land_by_row_dry[tile.y] or 0
    D.land_by_row_wet[tile.y] = D.land_by_row_wet[tile.y] or 0
    if terrain.surface == 'land' then
      D.land_by_row[tile.y] = D.land_by_row[tile.y] + 1
      D.land_by_col[tile.x] = D.land_by_col[tile.x] + 1
      if assert( WET_DRY_TYPE[terrain.ground] ) == 'dry' then
        D.land_by_row_dry[tile.y] = D.land_by_row_dry[tile.y] + 1
      end
      if assert( WET_DRY_TYPE[terrain.ground] ) == 'wet' then
        D.land_by_row_wet[tile.y] = D.land_by_row_wet[tile.y] + 1
      end
      D.land = D.land + 1
    else
      return
    end
    do
      local o = assert( D.ground_per_row[terrain.ground] )
      o[tile.y] = o[tile.y] + 1
    end
    do
      local o = assert( D.ground_per_col[terrain.ground] )
      o[tile.x] = o[tile.x] + 1
    end
  end )

  -- Terrain adjacency.
  Q.on_all_tiles( function( tile )
    local center = terrain_at( J, tile )
    if center.surface == 'water' then return end
    assert( center )
    assert( center.ground )
    D.adjacency[center.ground] = D.adjacency[center.ground] or {}
    -- local wetness = assert( compute_wetness( J, tile ) )
    local wetness = assert( wetness_full[tile.y][tile.x] )
    D.wetness[center.ground] = D.wetness[center.ground] + wetness
    D.land_wetness = D.land_wetness + wetness
    if ocean_adjacent( J, tile ) then
      D.land_ocean_adjacent = D.land_ocean_adjacent + 1
      D.with_ocean_adjacent[center.ground] =
          D.with_ocean_adjacent[center.ground] + 1
    end
    if river_adjacent( J, tile ) then
      D.with_river_adjacent[center.ground] =
          D.with_river_adjacent[center.ground] + 1
    end
    if ocean_adjacent_side( J, tile ) then
      D.land_ocean_adjacent_side = D.land_ocean_adjacent_side + 1
      D.with_ocean_adjacent_side[center.ground] =
          D.with_ocean_adjacent_side[center.ground] + 1
    end
    if river_adjacent_side( J, tile ) then
      D.with_river_adjacent_side[center.ground] =
          D.with_river_adjacent_side[center.ground] + 1
    end
    local A = assert( D.adjacency[center.ground] )
    A.count = A.count or 0
    A.adjacency_count = A.adjacency_count or 0
    A.count = A.count + 1
    local surround = Q.surrounding_coords( tile )
    for _, cc in ipairs( surround ) do
      local terrain = terrain_at( J, cc )
      if terrain.surface == 'land' then
        A.surrounding_land_on_row[tile.y] =
            A.surrounding_land_on_row[tile.y] + 1
        if terrain.ground == center.ground then
          A.adjacency_count = A.adjacency_count + 1
        end
      end
    end
  end )

  local function swamp_marsh_adjacency( target_terrain, S )
    assert( target_terrain )
    assert( S )
    Q.on_all_non_arctic_tiles( function( tile )
      do
        local terrain = terrain_at( J, tile )
        if terrain.surface ~= 'land' then return end
        if terrain.ground ~= target_terrain then return end
      end
      S.count = S.count + 1
      local surround = Q.surrounding_coords( tile )
      local has_swamp_adjacent = false
      local has_marsh_adjacent = false
      local has_swamp_or_marsh_adjacent = false
      local has_river_adjacent = false
      local has_ocean_adjacent = false
      local has_river_or_ocean_adjacent = false
      local has_any_adjacent = false
      local has_river = Q.has_river( J, tile )
      for _, cc in ipairs( surround ) do
        local terrain = terrain_at( J, cc )
        if terrain.surface == 'land' then
          has_swamp_adjacent = has_swamp_adjacent or
                                   (terrain.ground == 'swamp')
          has_marsh_adjacent = has_swamp_adjacent or
                                   (terrain.ground == 'marsh')
          has_river_adjacent = has_river_adjacent or
                                   Q.has_river( J, cc )
        else
          -- Ocean tiles can have rivers as well.
          has_river_adjacent = has_river_adjacent or
                                   Q.has_river( J, cc )
          has_ocean_adjacent = true
        end
        has_swamp_or_marsh_adjacent =
            has_swamp_or_marsh_adjacent or has_swamp_adjacent or
                has_marsh_adjacent
        has_river_or_ocean_adjacent =
            has_river_or_ocean_adjacent or has_river_adjacent or
                has_ocean_adjacent
        has_any_adjacent = has_any_adjacent or
                               has_swamp_or_marsh_adjacent or
                               has_river_or_ocean_adjacent
      end
      S.with_swamp_adjacent = S.with_swamp_adjacent +
                                  (has_swamp_adjacent and 1 or 0)
      S.with_marsh_adjacent = S.with_marsh_adjacent +
                                  (has_marsh_adjacent and 1 or 0)
      S.with_swamp_or_marsh_adjacent =
          S.with_swamp_or_marsh_adjacent +
              (has_swamp_or_marsh_adjacent and 1 or 0)
      S.with_river_adjacent = S.with_river_adjacent +
                                  (has_river_adjacent and 1 or 0)
      S.with_ocean_adjacent = S.with_ocean_adjacent +
                                  (has_ocean_adjacent and 1 or 0)
      S.with_river_or_ocean_adjacent =
          S.with_river_or_ocean_adjacent +
              (has_river_or_ocean_adjacent and 1 or 0)
      S.with_any_adjacent = S.with_any_adjacent +
                                (has_any_adjacent and 1 or 0)
      S.with_river = S.with_river + (has_river and 1 or 0)
    end )

  end

  -- Swamp.
  swamp_marsh_adjacency( 'swamp', D.swamp )
  swamp_marsh_adjacency( 'marsh', D.marsh )

  local max_savannah_dist = 0
  Q.on_all_non_arctic_tiles( function( tile )
    local terrain = terrain_at( J, tile )
    if terrain.surface == 'land' and terrain.ground == 'savannah' then
      if tile.y >= 36 then
        local delta = tile.y - 36
        max_savannah_dist = max( max_savannah_dist, delta )
      else
        local delta = 35 - tile.y
        max_savannah_dist = max( max_savannah_dist, delta )
      end
    end
  end )
  assert( max_savannah_dist )
  assert( type( max_savannah_dist ) == 'number' )
  assert( math.type( max_savannah_dist ) == 'integer' )
  assert( max_savannah_dist > 0 )
  D.max_savannah_row[max_savannah_dist] =
      D.max_savannah_row[max_savannah_dist] or 0
  D.max_savannah_row[max_savannah_dist] =
      D.max_savannah_row[max_savannah_dist] + 1

  local desert_count_center = 0
  local land_count_center = 0
  Q.on_all_non_arctic_tiles( function( tile )
    if tile.y ~= 34 and tile.y ~= 35 and tile.y ~= 36 and tile.y ~=
        37 then return end
    local terrain = terrain_at( J, tile )
    if terrain.surface == 'land' then
      land_count_center = land_count_center + 1
      if terrain.ground == 'desert' then
        desert_count_center = desert_count_center + 1
      end
    end
  end )
  if land_count_center > 0 then
    local desert_density_center =
        desert_count_center / land_count_center
    local bucket = math.floor( desert_density_center /
                                   DESERT_DENSITY_BUCKET_SIZE )
    D.desert_density_center[bucket] =
        D.desert_density_center[bucket] or 0
    D.desert_density_center[bucket] =
        D.desert_density_center[bucket] + 1
  end
end

local function finished( mode )
  do
    local opts = {
      title=format(
          'Terrain Row Distribution (empirical) (%s) [%d]', mode,
          D.savs ),
      x_label='Map Row (Y)',
      y_label='Value',
      y_range='0:0.7',
      x_range='1:70',
    }
    local csv_data = { header={ 'y' }, rows={} }
    for _, ground in ipairs( BIOME_ORDERING ) do
      insert( csv_data.header, ground )
    end
    for y_real = 1, 70 do
      local y = clamp( y_real, 4, 66 )
      local row = { y }
      local land = assert( D.land_by_row[y] )
      for _, ground in ipairs( BIOME_ORDERING ) do
        local count = assert( D.ground_per_row[ground][y] )
        local density = count / land
        table.insert( row, format( format( '%f', density ) ) )
      end
      table.insert( csv_data.rows, row )
    end
    local path = format( '%s/%s.rows.gnuplot', PLOTS_DIR, mode )
    plot.line_graph_to_file( path, csv_data, opts )
  end

  do
    local opts = {
      title=format(
          'Terrain Dry Row Distribution (empirical) (%s) [%d]',
          mode, D.savs ),
      x_label='Map Row (Y)',
      y_label='Value',
      y_range='0:1.0',
      x_range='1:70',
    }
    local csv_data = { header={ 'y' }, rows={} }
    for _, ground in ipairs( BIOME_ORDERING ) do
      insert( csv_data.header, ground )
    end
    for y_real = 1, 70 do
      local y = clamp( y_real, 4, 66 )
      local row = { y }
      local land = assert( D.land_by_row_dry[y] )
      for _, ground in ipairs( BIOME_ORDERING ) do
        if assert( WET_DRY_TYPE[ground] ) == 'dry' then
          local count = assert( D.ground_per_row[ground][y] )
          local density = count / land
          table.insert( row, format( format( '%f', density ) ) )
        else
          table.insert( row, 0 )
        end
      end
      table.insert( csv_data.rows, row )
    end
    local path = format( '%s/%s.rows.dry.gnuplot', PLOTS_DIR,
                         mode )
    plot.line_graph_to_file( path, csv_data, opts )
  end

  do
    local opts = {
      title=format(
          'Terrain Wet Row Distribution (empirical) (%s) [%d]',
          mode, D.savs ),
      x_label='Map Row (Y)',
      y_label='Value',
      y_range='0:1.0',
      x_range='1:70',
    }
    local csv_data = { header={ 'y' }, rows={} }
    for _, ground in ipairs( BIOME_ORDERING ) do
      insert( csv_data.header, ground )
    end
    for y_real = 1, 70 do
      local y = clamp( y_real, 4, 66 )
      local row = { y }
      local land = assert( D.land_by_row_wet[y] )
      for _, ground in ipairs( BIOME_ORDERING ) do
        if assert( WET_DRY_TYPE[ground] ) == 'wet' then
          local count = assert( D.ground_per_row[ground][y] )
          local density = count / land
          table.insert( row, format( format( '%f', density ) ) )
        else
          table.insert( row, 0 )
        end
      end
      table.insert( csv_data.rows, row )
    end
    local path = format( '%s/%s.rows.wet.gnuplot', PLOTS_DIR,
                         mode )
    plot.line_graph_to_file( path, csv_data, opts )
  end

  do
    local opts = {
      title=format(
          'Terrain Column Distribution (empirical) (%s) [%d]',
          mode, D.savs ),
      x_label='Map Column (X)',
      y_label='Value',
      y_range='0:0.4',
      x_range='1:56',
    }
    local csv_data = { header={ 'x' }, rows={} }
    for _, ground in ipairs( BIOME_ORDERING ) do
      insert( csv_data.header, ground )
    end
    for x_real = 1, 56 do
      local x = clamp( x_real, 1, 56 )
      local row = { x }
      local land = assert( D.land_by_col[x] )
      for _, ground in ipairs( BIOME_ORDERING ) do
        local count = assert( D.ground_per_col[ground][x] )
        local density = count / land
        table.insert( row, format( format( '%f', density ) ) )
      end
      table.insert( csv_data.rows, row )
    end
    local path = format( '%s/%s.cols.gnuplot', PLOTS_DIR, mode )
    plot.line_graph_to_file( path, csv_data, opts )
  end

  printfln( 'Total Land: %d', D.land )

  -- Swamp/Marsh adjacency.
  local function add_swamp_marsh_adjacency( o, name )
    assert( name )
    local S = assert( D[name] )
    o.wet = o.wet or {}
    o.wet[name] = {}
    local O = o.wet[name]
    O.__key_order = {
      'with_swamp_adjacent', --
      'with_marsh_adjacent', --
      'with_swamp_or_marsh_adjacent', --
      'with_river_adjacent', --
      'with_ocean_adjacent', --
      'with_river_or_ocean_adjacent', --
      'with_any_adjacent', --
      'with_river', --
      'count', --
      'density', --
    }
    O.count = S.count
    O.density = S.count / D.land
    O.with_swamp_adjacent = S.with_swamp_adjacent / S.count
    O.with_marsh_adjacent = S.with_marsh_adjacent / S.count
    O.with_swamp_or_marsh_adjacent =
        S.with_swamp_or_marsh_adjacent / S.count
    O.with_river_adjacent = S.with_river_adjacent / S.count
    O.with_ocean_adjacent = S.with_ocean_adjacent / S.count
    O.with_river_or_ocean_adjacent =
        S.with_river_or_ocean_adjacent / S.count
    O.with_any_adjacent = S.with_any_adjacent / S.count
    O.with_river = S.with_river / S.count
  end

  do
    local adjacency_file = format( '%s/%s.adjacency.json',
                                   PLOTS_DIR, mode )
    printfln( 'writing file %s...', adjacency_file )
    local f<close> = assert( io.open( adjacency_file, 'w' ) )
    local emit = function( bit ) f:write( bit ) end
    local o = {}
    add_swamp_marsh_adjacency( o, 'swamp' )
    add_swamp_marsh_adjacency( o, 'marsh' )

    o.general = {}
    local general = o.general
    general.biome = {}
    local biome = general.biome
    biome.__key_order = BIOME_ORDERING
    o.ocean = {}
    local ocean = o.ocean
    ocean.__key_order = BIOME_ORDERING
    o.ocean_normalized = {}
    local ocean_normalized = o.ocean_normalized
    ocean_normalized.__key_order = BIOME_ORDERING
    o.river = {}
    local river = o.river
    river.__key_order = BIOME_ORDERING
    o.ocean_side = {}
    local ocean_side = o.ocean_side
    ocean_side.__key_order = BIOME_ORDERING
    o.ocean_side_normalized = {}
    local ocean_side_normalized = o.ocean_side_normalized
    ocean_side_normalized.__key_order = BIOME_ORDERING
    o.river_side = {}
    local river_side = o.river_side
    river_side.__key_order = BIOME_ORDERING
    o.wetness = {}
    local wetness = o.wetness
    wetness.__key_order = BIOME_ORDERING
    o.wetness_normalized = {}
    local wetness_normalized = o.wetness_normalized
    wetness_normalized.__key_order = BIOME_ORDERING
    o.wetness_zero_sum = {}
    local wetness_zero_sum = o.wetness_zero_sum
    wetness_zero_sum.__key_order = BIOME_ORDERING

    -- General adjacency.
    local adjacency_relative = {}
    for _, ground in ipairs( BIOME_ORDERING ) do
      local adjacency_baseline = 0
      for y = 1, 70 do
        local land_on_row = assert( D.land_by_row[y] )
        local biome_on_row =
            assert( D.ground_per_row[ground][y] )
        local surrounding_land_on_row = assert(
                                            D.adjacency[ground]
                                                .surrounding_land_on_row[y] )
        if land_on_row > 0 then
          local density_at_row =
              assert( biome_on_row / land_on_row )
          local adjacency_baseline_at_row =
              density_at_row * surrounding_land_on_row
          adjacency_baseline = adjacency_baseline +
                                   adjacency_baseline_at_row
        end
      end
      local A = assert( D.adjacency[ground] )
      local density = A.count / D.land
      local adjacency_avg = A.adjacency_count / A.count
      adjacency_relative[ground] =
          A.adjacency_count / adjacency_baseline
      local result = adjacency_relative[ground]
      biome[ground] = {}
      biome[ground].count = A.count
      biome[ground].adjacency_count = A.adjacency_count
      biome[ground].density = density
      biome[ground].adjacency_avg = adjacency_avg
      biome[ground].adjacency_baseline = adjacency_baseline
      biome[ground].adjacency_relative = result
    end
    general.results = { __key_order=BIOME_ORDERING }
    for _, ground in ipairs( BIOME_ORDERING ) do
      general.results[ground] = assert(
                                    adjacency_relative[ground] )
    end
    for _, ground in ipairs( BIOME_ORDERING ) do
      ocean[ground] = assert( D.with_ocean_adjacent[ground] /
                                  D.adjacency[ground].count )
      river[ground] = assert( D.with_river_adjacent[ground] /
                                  D.adjacency[ground].count )
      ocean_side[ground] = assert(
                               D.with_ocean_adjacent_side[ground] /
                                   D.adjacency[ground].count )
      river_side[ground] = assert(
                               D.with_river_adjacent_side[ground] /
                                   D.adjacency[ground].count )
      wetness[ground] = assert( D.wetness[ground] /
                                    D.adjacency[ground].count )
      ocean_normalized[ground] =
          10000 * ocean[ground] /
              (D.land_ocean_adjacent / D.savs)
      ocean_side_normalized[ground] =
          10000 * ocean_side[ground] /
              (D.land_ocean_adjacent_side / D.savs)
    end
    local wetness_total = 0
    for _, ground in ipairs( BIOME_ORDERING_NON_ARCTIC ) do
      wetness_total = wetness_total + wetness[ground]
    end
    local wetness_avg = wetness_total /
                            #BIOME_ORDERING_NON_ARCTIC
    for _, ground in ipairs( BIOME_ORDERING_NON_ARCTIC ) do
      wetness_normalized[ground] = wetness[ground] / wetness_avg
      wetness_zero_sum[ground] = wetness_normalized[ground] - 1.0
    end
    wetness_normalized.arctic = 1.0
    wetness_zero_sum.arctic = 0
    json.write( o, 2, emit )
  end

  -- Savannah max row tracking.
  local savannah_rows
  do
    if mode == 'bbtm' or mode == 'bbmm' or mode == 'bbbm' or mode ==
        'new' then
      local opts = {
        title=format(
            'Savannah Row Limit Histograph (empirical) (%s) [%d]',
            mode, D.savs ),
        x_label='Distance from Equator',
        y_label='Frequency',
        x_range='0:20',
        y_range='0:1',
      }

      local csv_data = {
        header={ 'savannah_distance', 'frequency' },
        rows={},
      }

      local max_dist = 0
      for dist, _ in pairs( D.max_savannah_row ) do
        max_dist = max( max_dist, dist )
      end
      for i = 0, max_dist do
        local row = { i, 0 }
        if D.max_savannah_row[i] then
          row[2] = D.max_savannah_row[i] / D.savs
        end
        table.insert( csv_data.rows, row )
      end

      local savannah_file = format( '%s/%s.savannah.gnuplot',
                                    PLOTS_DIR, mode )
      plot.line_graph_to_file( savannah_file, csv_data, opts )
      savannah_rows = csv_data.rows
    end

    if mode == 'new' then
      local function savannah_max_to_temperature( sm )
        return 100 * (sm - 10) / 2
      end

      assert( savannah_rows )
      local opts = {
        title=format(
            'Temperature Histograph from Savannah (empirical) (%s) [%d]',
            mode, D.savs ),
        x_label='Temperature',
        y_label='Frequency',
        x_range='-300:300',
        y_range='0:1',
      }

      local csv_data = {
        header={ 'temperature', 'frequency' },
        rows={},
      }

      for _, row in ipairs( savannah_rows ) do
        row[1] = savannah_max_to_temperature( row[1] )
        table.insert( csv_data.rows, row )
      end

      local file = format( '%s/%s.savannah.temperature.gnuplot',
                           PLOTS_DIR, mode )
      plot.line_graph_to_file( file, csv_data, opts )
    end
  end

  -- Desert center density tracking.
  local desert_rows
  do
    if mode == 'bbtm' or mode == 'bbmm' or mode == 'bbbm' or mode ==
        'new' then
      local opts = {
        title=format(
            'Desert Center Density Histograph (empirical) (%s) [%d]',
            mode, D.savs ),
        x_label='Density',
        y_label='Frequency',
        x_range='0:0.3',
        y_range='0:0.02',
      }

      local csv_data = {
        header={ 'density', 'frequency' },
        rows={},
      }

      local max_bucket = 0
      for bucket, _ in pairs( D.desert_density_center ) do
        max_bucket = max( max_bucket, bucket )
      end
      for i = 0, max_bucket do
        local row = { i * DESERT_DENSITY_BUCKET_SIZE, 0 }
        if D.desert_density_center[i] then
          row[2] = D.desert_density_center[i] / D.savs
        end
        table.insert( csv_data.rows, row )
      end

      local file = format( '%s/%s.desert.gnuplot', PLOTS_DIR,
                           mode )
      plot.line_graph_to_file( file, csv_data, opts )
      desert_rows = csv_data.rows
    end

    if mode == 'new' then
      local function desert_density_to_temperature( sm )
        if sm >= .04 then
          return -100 * (sm - .04) / .08
        else
          return -100 * (sm - .04) / .04
        end
      end

      assert( desert_rows )
      local opts = {
        title=format(
            'Temperature Histograph from Desert (empirical) (%s) [%d]',
            mode, D.savs ),
        x_label='Temperature',
        y_label='Frequency',
        x_range='-300:300',
        y_range='0:.01',
      }

      local csv_data = {
        header={ 'temperature', 'frequency' },
        rows={},
      }

      for _, row in ipairs( desert_rows ) do
        row[1] = desert_density_to_temperature( row[1] )
        table.insert( csv_data.rows, row )
      end

      local file = format( '%s/%s.desert.temperature.gnuplot',
                           PLOTS_DIR, mode )
      plot.line_graph_to_file( file, csv_data, opts )
    end
  end
end

-----------------------------------------------------------------
-- Collection stage.
-----------------------------------------------------------------
-- This gets run once after all modes are run (which happen in
-- separate processes) to collect all of their results.
local function collect()
  print( 'collecting...' )
  local BUCKET_FRACTION = 100
  local MODES = {
    'bbtt', 'bbtm', 'bbtb', 'bbmt', 'bbmm', 'bbmb', 'bbbt',
    'bbbm', 'bbbb', 'tmmm', 'bmmm', 'mtmm', 'mbmm', 'new',
  }
  local o = {}
  o.modes = {}
  o.biomes = {}
  o.biome_averages = {}
  o.modes.__key_order = MODES
  o.biomes.__key_order = BIOME_ORDERING
  o.biome_averages.__key_order = BIOME_ORDERING
  o.wet = {}
  o.wet.__key_order = MODES
  local w = {}
  w.modes = {}
  w.modes.__key_order = MODES
  w.biomes = {}
  w.biomes.__key_order = BIOME_ORDERING
  local csv_buckets = {}
  for _, mode in ipairs( MODES ) do
    local adjacency_file = format( '%s/%s.adjacency.json',
                                   PLOTS_DIR, mode )
    local f<close> = assert( io.open( adjacency_file, 'r' ) )
    local mode_json = json.read( f:read( '*all' ) )
    local results = assert( mode_json.general.results )
    o.modes[mode] = results
    o.modes[mode].__key_order = BIOME_ORDERING
    for _, biome in ipairs( BIOME_ORDERING ) do
      local result = assert( results[biome] )
      o.biomes[biome] = o.biomes[biome] or { __key_order=MODES }
      o.biomes[biome][mode] = assert( o.modes[mode][biome] )
      o.biome_averages[biome] = o.biome_averages[biome] or 0
      o.biome_averages[biome] =
          o.biome_averages[biome] + result / #MODES
      csv_buckets[biome] = csv_buckets[biome] or {}
      local bucket = math.floor( result * BUCKET_FRACTION )
      csv_buckets[biome][bucket] =
          csv_buckets[biome][bucket] or 0
      csv_buckets[biome][bucket] = csv_buckets[biome][bucket] + 1
    end
    o.wet[mode] = {
      swamp={
        with_ocean_adjacent=assert(
            mode_json.wet.swamp.with_ocean_adjacent ),
      },
      marsh={
        with_ocean_adjacent=assert(
            mode_json.wet.marsh.with_ocean_adjacent ),
      },
    }
    w.modes[mode] = assert( mode_json.wetness_zero_sum )
    for _, biome in ipairs( BIOME_ORDERING_NON_ARCTIC ) do
      w.biomes[biome] = w.biomes[biome] or {}
      w.biomes[biome][mode] = assert(
                                  mode_json.wetness_zero_sum[biome] )
    end
    w.biomes.arctic = w.biomes.arctic or {}
    w.biomes.arctic[mode] = 0
  end

  do
    local collected_json_file = format( '%s/wetness.json',
                                        PLOTS_DIR )
    printfln( 'writing %s...', collected_json_file )
    json.write_file( collected_json_file, w, 2 )
  end

  do
    local collected_json_file = format( '%s/adjacency.json',
                                        PLOTS_DIR )
    printfln( 'writing %s...', collected_json_file )
    json.write_file( collected_json_file, o, 2 )
  end

  do
    local opts = {
      title=format(
          'Biome Adjacency Histogram (empirical) (%s) [%d]',
          'collected', D.savs ),
      x_label='Relative Adjacency',
      y_label='Count',
      x_range='.6:2.5',
      y_range='0:6',
    }
    local csv_data = { header={ 'value' }, rows={} }
    for _, biome in ipairs( BIOME_ORDERING ) do
      table.insert( csv_data.header, biome )
    end
    for i = 0, 3 * BUCKET_FRACTION do
      local row = {}
      table.insert( row, format( '%.3f', i / BUCKET_FRACTION ) )
      for _, biome in ipairs( BIOME_ORDERING ) do
        csv_buckets[biome][i] = csv_buckets[biome][i] or 0
        table.insert( row, format( ',%d', csv_buckets[biome][i] ) )
      end
      table.insert( csv_data.rows, row )
    end
    local adjacency_file = format( '%s/adjacency.gnuplot',
                                   PLOTS_DIR )
    plot.line_graph_to_file( adjacency_file, csv_data, opts )
  end

  do
    local collected_json_file = format( '%s/adjacency.json',
                                        PLOTS_DIR )
    printfln( 'writing %s...', collected_json_file )
    json.write_file( collected_json_file, o, 2 )
  end

  do
    local path = format( '%s/biome.config.json', PLOTS_DIR )
    printfln( 'writing %s...', path )
    local config = {
      __key_order={
        'wetness', --
        'clustering', --
      },
      wetness={
        __key_order={
          'parameters', --
          'modulator', --
        },
        parameters={
          __key_order={
            'accumulation', --
            'consumption', --
          },
          accumulation=WETNESS_ACCUMULATION,
          consumption=WETNESS_CONSUMPTION,
        },
        modulator={
          __key_order={
            'value', --
            'stddev', --
            'relative_stddev', --
          },
          value={
            __key_order=BIOME_ORDERING, --
            savannah=0,
            grassland=0,
            tundra=0,
            plains=0,
            prairie=0,
            desert=0,
            swamp=0,
            marsh=0,
            arctic=0,
          },
          stddev={
            __key_order=BIOME_ORDERING, --
            savannah=0,
            grassland=0,
            tundra=0,
            plains=0,
            prairie=0,
            desert=0,
            swamp=0,
            marsh=0,
            arctic=0,
          },
          relative_stddev={
            __key_order=BIOME_ORDERING, --
            savannah=0,
            grassland=0,
            tundra=0,
            plains=0,
            prairie=0,
            desert=0,
            swamp=0,
            marsh=0,
            arctic=0,
          },
        },
      },
      clustering={
        __key_order={
          'climate_normal', --
          'climate_gradient', --
        },
        climate_normal=assert( deep_copy( o.modes.bbmm ) ),
        climate_gradient={
          __key_order=BIOME_ORDERING,
          savannah={},
          grassland={},
          tundra={},
          plains={},
          prairie={},
          desert={},
          swamp={},
          marsh={},
          arctic={},
        },
      },
    }
    local function round( d )
      return math.floor( d * 1000 ) / 1000
    end
    local function cluster( d )
      return round( log( d ) / log( 2 ) )
    end
    local swamp_marsh_water_gravity = -- LuaFormatter off
      assert( o.wet.bbmt.swamp.with_ocean_adjacent ) +
      assert( o.wet.bbmt.marsh.with_ocean_adjacent ) +
      assert( o.wet.bbmm.swamp.with_ocean_adjacent ) +
      assert( o.wet.bbmm.marsh.with_ocean_adjacent ) +
      assert( o.wet.bbmb.swamp.with_ocean_adjacent ) +
      assert( o.wet.bbmb.marsh.with_ocean_adjacent ) +
      0.0
    swamp_marsh_water_gravity = round( swamp_marsh_water_gravity/6 )
    -- LuaFormatter on
    for _, biome in ipairs( BIOME_ORDERING ) do
      config.clustering.climate_normal[biome] = {
        for_self=cluster( config.clustering.climate_normal[biome] ),
        for_water=json.JNULL,
      }
      -- LuaFormatter off
      local gradient =
        (cluster(o.modes.bbmb[biome]) - cluster(o.modes.bbmm[biome])) +
        (cluster(o.modes.bbmm[biome]) - cluster(o.modes.bbmt[biome]))
      config.clustering.climate_gradient[biome] = {
        for_self=round( gradient / 2 ),
        for_water=json.JNULL,
      }
      -- LuaFormatter on
    end
    config.clustering.climate_normal.swamp.for_water =
        swamp_marsh_water_gravity
    config.clustering.climate_normal.marsh.for_water =
        swamp_marsh_water_gravity
    config.clustering.climate_gradient.swamp.for_water = 0
    config.clustering.climate_gradient.marsh.for_water = 0
    config.clustering.climate_normal.arctic = {
      for_self=cluster( 1.0 ),
      for_water=json.JNULL,
    }
    config.clustering.climate_gradient.arctic = {
      for_self=0.0,
      for_water=json.JNULL,
    }
    for _, biome in ipairs( BIOME_ORDERING_NON_ARCTIC ) do
      local mode_total = 0
      local mode_total2 = 0
      for _, mode in ipairs( MODES ) do
        mode_total = mode_total + assert( w.modes[mode][biome] )
        mode_total2 = mode_total2 +
                          assert( w.modes[mode][biome] ) ^ 2
      end
      local mode_avg = mode_total / #MODES
      local mode2_avg = mode_total2 / #MODES
      local mode_stddev = (mode2_avg - mode_avg ^ 2) ^ .5
      config.wetness.modulator.value[biome] = mode_avg
      config.wetness.modulator.stddev[biome] = mode_stddev
      -- LuaFormatter off
      config.wetness.modulator.relative_stddev[biome] =
          abs(config.wetness.modulator.stddev[biome] /
              config.wetness.modulator.value[biome] )
      -- LuaFormatter on
    end
    json.write_file( path, config, 2 )
  end
end

-----------------------------------------------------------------
-- Lambda.
-----------------------------------------------------------------
return { lambda=lambda, finished=finished, collect=collect }