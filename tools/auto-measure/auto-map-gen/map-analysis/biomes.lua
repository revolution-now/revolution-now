-- This lambda will run on multiple savs and will gather some
-- statistics on the distribution of ground terrains.
-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local Q = require( 'lib.query' )
local json = require( 'moon.json' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local insert = table.insert
local format = string.format
local concat = table.concat

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

local GNUPLOT_FILE_TEMPLATE = [[
#!/usr/bin/env -S gnuplot -p
set title "Terrain Distribution ({{MODE}} [{{COUNT}}])"
set datafile separator ","
set key outside right
set grid
set xlabel "Map Row (Y)"
set ylabel "value"

# Use the first row as column headers for titles.
set key autotitle columnhead

set yrange [0:0.7]

# Plot: x is column 1, then plot columns 2..N as separate lines.
plot for [col=2:*] "{{CSV_STEM}}" using 1:col with lines lw 2
]]

local function write_gnuplot_file( csv_fname, mode, count )
  assert( mode )
  assert( count )
  local gnuplot_fname =
      format( '%s/%s.gnuplot', PLOTS_DIR, mode )
  printfln( 'writing gnuplot file %s...', gnuplot_fname )
  local f<close> = assert( io.open( gnuplot_fname, 'w' ) )
  local body = GNUPLOT_FILE_TEMPLATE
  local subs = {
    { key='{{CSV_STEM}}', val=csv_fname }, --
    { key='{{MODE}}', val=mode }, --
    { key='{{COUNT}}', val=count }, --
  }
  for _, p in ipairs( subs ) do
    body = body:gsub( assert( p.key ), assert( p.val ) )
  end
  f:write( body )
end

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
  --   adjacency_count_per_row,
  -- }
  adjacency={},
}

local INCLUDE_LAND = false
local INCLUDE_SUM = false

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
local function terrain_at( json, tile )
  -- Can intercept here.
  return Q.terrain_at( json, tile )
  -- local t = Q.terrain_at( json, tile )
  -- return {
  --   surface=t.surface,
  --   ground=assert( GROUND_TYPES[math.random( #GROUND_TYPES - 1 )] ),
  -- }
end

local function lambda( json )
  D.total_savs = D.total_savs + 1
  for _, ground_type in ipairs( GROUND_TYPES ) do
    D.ground_per_row[ground_type] =
        D.ground_per_row[ground_type] or {}
  end

  Q.on_all_tiles( function( tile )
    for _, ground_type in ipairs( GROUND_TYPES ) do
      D.land[tile.y] = D.land[tile.y] or 0
      local o = assert( D.ground_per_row[ground_type] )
      o[tile.y] = o[tile.y] or 0
    end
  end )

  Q.on_all_tiles( function( tile )
    local terrain = terrain_at( json, tile )

    D.land[tile.y] = D.land[tile.y] or 0
    if terrain.surface == 'land' then
      D.land[tile.y] = D.land[tile.y] + 1
      D.total_land = D.total_land + 1
    else
      return
    end

    local o = assert( D.ground_per_row[terrain.ground],
                      format( 'terrain.ground=%s not found',
                              tostring( terrain.ground ) ) )
    o[tile.y] = o[tile.y] + 1
  end )

  -- Terrain adjacency.
  Q.on_all_tiles( function( tile )
    local center = terrain_at( json, tile )
    if center.surface == 'water' then return end
    assert( center )
    assert( center.ground )
    D.adjacency[center.ground] = D.adjacency[center.ground] or {}
    local A = assert( D.adjacency[center.ground] )
    A.count = A.count or 0
    A.adjacency_count = A.adjacency_count or 0
    A.count = A.count + 1
    A.adjacency_count_per_row = A.adjacency_count_per_row or {}
    A.adjacency_count_per_row[tile.y] =
        A.adjacency_count_per_row[tile.y] or 0
    local surround = Q.surrounding_coords( tile )
    for _, cc in ipairs( surround ) do
      local terrain = terrain_at( json, cc )
      if terrain.ground == center.ground then
        A.adjacency_count = A.adjacency_count + 1
        A.adjacency_count_per_row[tile.y] =
            A.adjacency_count_per_row[tile.y] + 1
      end
    end
  end )

  local function swamp_marsh_adjacency( target_terrain, S )
    assert( target_terrain )
    assert( S )
    Q.on_all_tiles( function( tile )
      do
        local terrain = terrain_at( json, tile )
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
        local terrain = terrain_at( json, cc )
        if terrain.surface == 'land' then
          has_swamp_adjacent = has_swamp_adjacent or
                                   (terrain.ground == 'swamp')
          has_marsh_adjacent = has_swamp_adjacent or
                                   (terrain.ground == 'marsh')
          has_river_adjacent = has_river_adjacent or
                                   Q.has_river( json, cc )
        else
          -- Ocean tiles can have rivers as well.
          has_river_adjacent = has_river_adjacent or
                                   Q.has_river( json, cc )
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
end

local function finished( mode )
  do
    local csv_fname = format( '%s.csv', mode )
    local csv_path = format( '%s/%s', PLOTS_DIR, csv_fname )
    write_gnuplot_file( csv_fname, mode, D.total_savs )
    printfln( 'writing csv file %s...', csv_path )
    local f<close> = assert( io.open( csv_path, 'w' ) )
    local emit = function( fmt, ... )
      f:write( format( fmt, ... ) )
    end
    local header = { 'y' }
    if INCLUDE_LAND then insert( header, 'land' ) end
    if INCLUDE_SUM then insert( header, 'sum' ) end
    for _, ground in ipairs( GROUND_TYPES ) do
      insert( header, ground )
    end
    emit( '%s\n', concat( header, ',' ) )
    for y_real = 1, 70 do
      local y = clamp( y_real, 4, 66 )
      local land = assert( D.land[y] )
      emit( '%d', y )
      if INCLUDE_LAND then
        emit( ',%f', land / (56 * D.total_savs) )
      end
      local sum = 0
      for _, ground in ipairs( GROUND_TYPES ) do
        local count = assert( D.ground_per_row[ground][y] )
        sum = sum + count
        local density = count / land
        emit( ',%f', density )
      end
      if INCLUDE_SUM then emit( ',%f', sum / land ) end
      emit( '\n' )
    end
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
        if land_on_row > 0 then
          local density_at_row = assert(
                                     D.ground_per_row[ground][y] ) /
                                     land_on_row
          local adjacency_baseline_at_row =
              density_at_row * 8 * D.ground_per_row[ground][y]
          adjacency_baseline = adjacency_baseline +
                                   adjacency_baseline_at_row
        end
      end
      local val = assert( D.adjacency[ground] )
      local density = val.count / D.total_land
      local adjacency_avg = val.adjacency_count / val.count
      adjacency_relative[ground] =
          val.adjacency_count / adjacency_baseline
      local result = adjacency_relative[ground]
      biome[ground] = {}
      biome[ground].count = val.count
      biome[ground].adjacency_count = val.adjacency_count
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
  o.biomes = {}
  o.averages = {}
  o.averages.__key_order = GROUND_TYPES
  local csv_buckets = {}
  for _, mode in ipairs( MODES ) do
    local adjacency_file = format( '%s/%s.adjacency.json',
                                   PLOTS_DIR, mode )
    local f<close> = assert( io.open( adjacency_file, 'r' ) )
    local mode_json = json.read( f:read( '*all' ) )
    local results = assert( mode_json.general.results )
    o.biomes[mode] = results
    o.biomes[mode].__key_order = GROUND_TYPES
    for _, biome in ipairs( GROUND_TYPES ) do
      local result = assert( results[biome] )
      o.averages[biome] = o.averages[biome] or 0
      o.averages[biome] = o.averages[biome] + result / #MODES
      csv_buckets[biome] = csv_buckets[biome] or {}
      local bucket = math.floor( result * BUCKET_FRACTION )
      csv_buckets[biome][bucket] =
          csv_buckets[biome][bucket] or 0
      csv_buckets[biome][bucket] = csv_buckets[biome][bucket] + 1
    end
  end
  local collected_csv_file = format( '%s/adjacency.csv',
                                     PLOTS_DIR )
  printfln( 'writing %s...', collected_csv_file )
  local csv_out<close> = assert(
                             io.open( collected_csv_file, 'w' ) )
  csv_out:write( 'value' )
  for _, biome in ipairs( GROUND_TYPES ) do
    csv_out:write( ',' .. biome )
  end
  csv_out:write( '\n' )
  for i = 0, 2 * BUCKET_FRACTION do
    csv_out:write( format( '%.3f', i / BUCKET_FRACTION ) )
    for _, biome in ipairs( GROUND_TYPES ) do
      csv_buckets[biome][i] = csv_buckets[biome][i] or 0
      csv_out:write( format( ',%d', csv_buckets[biome][i] ) )
    end
    csv_out:write( '\n' )
  end
  local collected_json_file = format( '%s/adjacency.json',
                                      PLOTS_DIR )
  printfln( 'writing %s...', collected_json_file )
  local json_out<close> = assert(
                              io.open( collected_json_file, 'w' ) )
  json.write( o, 2, function( bit ) json_out:write( bit ) end )
end

-----------------------------------------------------------------
-- Lambda.
-----------------------------------------------------------------
return { lambda=lambda, finished=finished, collect=collect }