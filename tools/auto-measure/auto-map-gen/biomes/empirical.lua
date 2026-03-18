-- This lambda will run on multiple savs and will gather some
-- statistics on the distribution of ground terrains.
-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local Q = require( 'lib.query' )
local json = require( 'moon.json' )
local plot = require( 'moon.plot' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local insert = table.insert
local format = string.format
local concat = table.concat
local max = math.max

-----------------------------------------------------------------
-- Constants.
-----------------------------------------------------------------
local DESERT_DENSITY_BUCKET_SIZE = .0001

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

-----------------------------------------------------------------
-- Data.
-----------------------------------------------------------------
-- The data here spans multiple savs.
local D = {
  total_savs=0,
  total_land=0,

  -- key=row
  land={},

  -- key1=ground_type, key2=row, value=count
  ground_per_row={},

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

local GROUND_TYPES = {
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

-----------------------------------------------------------------
-- Lambda.
-----------------------------------------------------------------
local function terrain_at( json_o, tile )
  -- Can intercept here.
  return Q.terrain_at( json_o, tile )
  -- local t = Q.terrain_at( json, tile )
  -- return {
  --   surface=t.surface,
  --   ground=assert( GROUND_TYPES[math.random( #GROUND_TYPES - 1 )] ),
  -- }
end

local function lambda( json_o )
  D.total_savs = D.total_savs + 1
  for _, ground_type in ipairs( GROUND_TYPES ) do
    D.ground_per_row[ground_type] =
        D.ground_per_row[ground_type] or {}
    D.adjacency[ground_type] = D.adjacency[ground_type] or {}
    local A = assert( D.adjacency[ground_type] )
    A.count = A.count or 0
    A.adjacency_count = A.adjacency_count or 0
  end

  Q.on_all_tiles( function( tile )
    for _, ground_type in ipairs( GROUND_TYPES ) do
      D.land[tile.y] = D.land[tile.y] or 0
      local o = assert( D.ground_per_row[ground_type] )
      o[tile.y] = o[tile.y] or 0
    end
  end )

  Q.on_all_tiles( function( tile )
    for _, ground_type in ipairs( GROUND_TYPES ) do
      D.adjacency[ground_type] = D.adjacency[ground_type] or {}
      local o = D.adjacency[ground_type]
      o.surrounding_land_on_row = o.surrounding_land_on_row or {}
      o.surrounding_land_on_row[tile.y] =
          o.surrounding_land_on_row[tile.y] or 0
    end
  end )

  Q.on_all_tiles( function( tile )
    local terrain = terrain_at( json_o, tile )

    D.land[tile.y] = D.land[tile.y] or 0
    if terrain.surface == 'land' then
      D.land[tile.y] = D.land[tile.y] + 1
      D.total_land = D.total_land + 1
    else
      return
    end

    local o = assert( D.ground_per_row[terrain.ground] )
    o[tile.y] = o[tile.y] + 1
  end )

  -- Terrain adjacency.
  Q.on_all_tiles( function( tile )
    local center = terrain_at( json_o, tile )
    if center.surface == 'water' then return end
    assert( center )
    assert( center.ground )
    D.adjacency[center.ground] = D.adjacency[center.ground] or {}
    local A = assert( D.adjacency[center.ground] )
    A.count = A.count or 0
    A.adjacency_count = A.adjacency_count or 0
    A.count = A.count + 1
    local surround = Q.surrounding_coords( tile )
    for _, cc in ipairs( surround ) do
      local terrain = terrain_at( json_o, cc )
      if terrain.surface == 'land' then
        A.surrounding_land_on_row[tile.y] =
            A.surrounding_land_on_row[tile.y] + 1
      end
      if terrain.ground == center.ground then
        A.adjacency_count = A.adjacency_count + 1
      end
    end
  end )

  local function swamp_marsh_adjacency( target_terrain, S )
    assert( target_terrain )
    assert( S )
    Q.on_all_tiles( function( tile )
      do
        local terrain = terrain_at( json_o, tile )
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
      for _, cc in ipairs( surround ) do
        local terrain = terrain_at( json_o, cc )
        if terrain.surface == 'land' then
          has_swamp_adjacent = has_swamp_adjacent or
                                   (terrain.ground == 'swamp')
          has_marsh_adjacent = has_swamp_adjacent or
                                   (terrain.ground == 'marsh')
          has_river_adjacent = has_river_adjacent or
                                   Q.has_river( json_o, cc )
        else
          -- Ocean tiles can have rivers as well.
          has_river_adjacent = has_river_adjacent or
                                   Q.has_river( json_o, cc )
          has_ocean_adjacent = true
        end
        has_swamp_or_marsh_adjacent =
            has_swamp_adjacent or has_marsh_adjacent
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
    end )

  end

  -- Swamp.
  swamp_marsh_adjacency( 'swamp', D.swamp )
  swamp_marsh_adjacency( 'marsh', D.marsh )

  local max_savannah_dist = 0
  Q.on_all_tiles( function( tile )
    local terrain = terrain_at( json_o, tile )
    if terrain.ground == 'savannah' then
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
  Q.on_all_tiles( function( tile )
    if tile.y ~= 34 and tile.y ~= 35 and tile.y ~= 36 and tile.y ~=
        37 then return end
    local terrain = terrain_at( json_o, tile )
    if terrain.surface == 'land' then
      land_count_center = land_count_center + 1
    end
    if terrain.ground == 'desert' then
      desert_count_center = desert_count_center + 1
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
      title=format( 'Terrain Distribution (empirical) (%s) [%d]',
                    mode, D.total_savs ),
      x_label='Map Row (Y)',
      y_label='Value',
      y_range='0:0.7',
    }
    local csv_data = { header={ 'y' }, rows={} }
    for _, ground in ipairs( GROUND_TYPES ) do
      insert( csv_data.header, ground )
    end
    for y_real = 1, 70 do
      local y = clamp( y_real, 4, 66 )
      local row = { y }
      local land = assert( D.land[y] )
      for _, ground in ipairs( GROUND_TYPES ) do
        local count = assert( D.ground_per_row[ground][y] )
        local density = count / land
        table.insert( row, format( format( '%f', density ) ) )
      end
      table.insert( csv_data.rows, row )
    end
    local path =
        format( '%s/%s.spatial.gnuplot', PLOTS_DIR, mode )
    plot.line_graph_to_file( path, csv_data, opts )
  end

  printfln( 'Total Land: %d', D.total_land )

  -- Swamp/Marsh adjacency.
  local function add_swamp_marsh_adjacency( o, name )
    assert( name )
    local S = assert( D[name] )
    o.wet = o.wet or {}
    o.wet[name] = {}
    local O = o.wet[name]
    O.count = S.count
    O.density = S.count / D.total_land
    O.with_swamp_adjacent = S.with_swamp_adjacent / S.count
    O.with_marsh_adjacent = S.with_marsh_adjacent / S.count
    O.with_swamp_or_marsh_adjacent =
        S.with_swamp_or_marsh_adjacent / S.count
    O.with_river_adjacent = S.with_river_adjacent / S.count
    O.with_ocean_adjacent = S.with_ocean_adjacent / S.count
    O.with_river_or_ocean_adjacent =
        S.with_river_or_ocean_adjacent / S.count
    O.with_any_adjacent = S.with_any_adjacent / S.count
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
    biome.__key_order = GROUND_TYPES

    -- General adjacency.
    local adjacency_relative = {}
    for _, ground in ipairs( GROUND_TYPES ) do
      local adjacency_baseline = 0
      for y = 1, 70 do
        local land_on_row = assert( D.land[y] )
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
      local density = A.count / D.total_land
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
    general.results = { __key_order=GROUND_TYPES }
    for _, ground in ipairs( GROUND_TYPES ) do
      general.results[ground] = assert(
                                    adjacency_relative[ground] )
    end
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
            mode, D.total_savs ),
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
          row[2] = D.max_savannah_row[i] / D.total_savs
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
            mode, D.total_savs ),
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
            mode, D.total_savs ),
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
          row[2] = D.desert_density_center[i] / D.total_savs
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
            mode, D.total_savs ),
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
    'bbbm', 'bbbb', 'new',
  }
  local o = {}
  o.modes = {}
  o.biomes = {}
  o.biome_averages = {}
  o.modes.__key_order = MODES
  o.biomes.__key_order = GROUND_TYPES
  o.biome_averages.__key_order = GROUND_TYPES
  local csv_buckets = {}
  for _, mode in ipairs( MODES ) do
    local adjacency_file = format( '%s/%s.adjacency.json',
                                   PLOTS_DIR, mode )
    local f<close> = assert( io.open( adjacency_file, 'r' ) )
    local mode_json = json.read( f:read( '*all' ) )
    local results = assert( mode_json.general.results )
    o.modes[mode] = results
    o.modes[mode].__key_order = GROUND_TYPES
    for _, biome in ipairs( GROUND_TYPES ) do
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
  end

  do
    local opts = {
      title=format(
          'Biome Adjacency Histogram (empirical) (%s) [%d]',
          'collected', D.total_savs ),
      x_label='Relative Adjacency',
      y_label='Count',
      x_range='.6:2.5',
      y_range='0:6',
    }
    local csv_data = { header={ 'value' }, rows={} }
    for _, biome in ipairs( GROUND_TYPES ) do
      table.insert( csv_data.header, biome )
    end
    for i = 0, 3 * BUCKET_FRACTION do
      local row = {}
      table.insert( row, format( '%.3f', i / BUCKET_FRACTION ) )
      for _, biome in ipairs( GROUND_TYPES ) do
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
    local json_out<close> = assert(
                                io.open( collected_json_file, 'w' ) )
    json.write( o, 2, function( bit ) json_out:write( bit ) end )
  end
end

-----------------------------------------------------------------
-- Lambda.
-----------------------------------------------------------------
return { lambda=lambda, finished=finished, collect=collect }