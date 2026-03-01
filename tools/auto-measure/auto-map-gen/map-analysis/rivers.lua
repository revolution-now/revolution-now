-- This lambda will run on multiple savs and will gather some
-- statistics on the properties of rivers.
-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local Q = require( 'lib.query' )
local json = require( 'moon.json' )
local csv = require( 'ftcsv' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local format = string.format
local insert = table.insert

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function printfln( ... ) print( string.format( ... ) ) end

local PLOTS_DIR = 'rivers/empirical'

local BIOME_ORDERING = {
  'savannah', --
  'grassland', --
  'tundra', --
  'plains', --
  'prairie', --
  'desert', --
  'swamp', --
  'marsh', --
  -- 'arctic', --
}

local GNUPLOT_FILE_TEMPLATE = [[
#!/usr/bin/env -S gnuplot -p
set title "{{TITLE}} ({{MODE}} [{{COUNT}}])"
set datafile separator ","
set key outside right
set grid
set xlabel "{{XLABEL}}"
set ylabel "{{YLABEL}}"

# Use the first row as column headers for titles.
set key autotitle columnhead

set xrange [{{XRANGE}}]
set yrange [{{YRANGE}}]

# Plot: x is column 1, then plot columns 2..N as separate lines.
plot for [col=2:*] "{{CSV_STEM}}" using 1:col with lines lw 2
]]

local function write_gnuplot_file( mode, stem, opts )
  assert( mode )
  assert( stem )
  assert( opts )
  local csv_fname = format( '%s.%s.csv', mode, stem )
  local gnuplot_fname = format( '%s/%s.%s.gnuplot', PLOTS_DIR,
                                mode, stem )
  printfln( 'writing gnuplot file %s...', gnuplot_fname )
  local f<close> = assert( io.open( gnuplot_fname, 'w' ) )
  local body = GNUPLOT_FILE_TEMPLATE
  local subs = {
    { key='{{TITLE}}', val=assert( opts.title ) }, --
    { key='{{XLABEL}}', val=assert( opts.x_label ) }, --
    { key='{{YLABEL}}', val=assert( opts.y_label ) }, --
    { key='{{XRANGE}}', val=assert( opts.x_range ) }, --
    { key='{{YRANGE}}', val=assert( opts.y_range ) }, --
    { key='{{CSV_STEM}}', val=assert( csv_fname ) }, --
    { key='{{MODE}}', val=assert( opts.mode ) }, --
    { key='{{COUNT}}', val=assert( opts.count ) }, --
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
  savs=0,
  tiles=0,

  land=0,
  water=0,
  water_adjacent_to_land=0,

  maj=0,
  min=0,
  any=0,

  maj_on_water=0,
  min_on_water=0,
  any_on_water=0,

  maj_by_row={},
  min_by_row={},
  any_by_row={},

  maj_forks=0,
  min_forks=0,
  any_forks=0,

  __segments=0,
  __river_tiles={}, -- key=rastorized_tile, value=segment

  num_connected=0,
  num_with_water=0,
  num_without_water=0,
  lengths={}, -- key=length, value=count

  starts=0,
  ends=0,
  starts_by_row={},

  land_islands=0,
  water_islands=0,
  any_islands=0,

  with_hills=0,

  biome_river={}, -- key=biome, value=count
  biome_counts={}, -- key=biome, value=count
}

-----------------------------------------------------------------
-- Lambda.
-----------------------------------------------------------------
local function terrain_at( J, tile )
  return Q.terrain_at( J, tile )
end

local function assign_segment( J, start, segment )
  local rastorized_start = Q.rastorize( start )
  assert( type( segment ) == 'number' )
  assert( D.__river_tiles[rastorized_start] == 0 )
  D.__river_tiles[rastorized_start] = segment
  assert( segment > 0 )
  assert( Q.has_river( J, start ) )
  local is_this_water = Q.is_water( J, start )
  Q.on_surrounding_tiles_cardinal( start, function( coord )
    local is_surrounding_water = Q.is_water( J, coord )
    -- Can't connect a single river segment across two water
    -- tiles.
    if is_this_water and is_surrounding_water then return end
    local rastorized_coord = Q.rastorize( coord )
    if D.__river_tiles[rastorized_coord] and
        D.__river_tiles[rastorized_coord] > 0 then
      assert( Q.has_river( J, coord ) )
      assert( D.__river_tiles[rastorized_coord] == segment )
      return
    end
    if Q.has_river( J, coord ) then
      assign_segment( J, coord, segment )
    end
  end )
end

local function find_connected( J )
  for rastorized_tile, segment in pairs( D.__river_tiles ) do
    local tile = Q.unrastorize( rastorized_tile )
    -- printfln( 'exploring river tile: %s', tile )
    assert( Q.has_river( J, tile ) )
    if segment == 0 then
      D.__segments = D.__segments + 1
      assign_segment( J, tile, D.__segments )
    end
  end
  local segment_length = {}
  local segment_has_water = {}
  Q.on_all_non_arctic_tiles( function( tile )
    if not Q.has_river( J, tile ) then return end
    local rastorized = Q.rastorize( tile )
    local segment = assert( D.__river_tiles[rastorized],
                            tostring( D.__river_tiles[rastorized] ) ..
                                ', ' ..
                                tostring(
                                    Q.unrastorize( rastorized ) ) )
    assert( segment > 0 )
    segment_length[segment] = segment_length[segment] or 0
    segment_length[segment] = segment_length[segment] + 1
    if Q.is_water( J, tile ) then
      segment_has_water[segment] = true
    end
  end )
  local max_length = 0
  for segment = 1, D.__segments do
    local length = assert( segment_length[segment],
                           tostring( segment ) )
    max_length = math.max( max_length, length )
    D.lengths[length] = D.lengths[length] or 0
    D.lengths[length] = D.lengths[length] + 1
    if segment_has_water[segment] then
      D.num_with_water = D.num_with_water + 1
    else
      D.num_without_water = D.num_without_water + 1
    end
  end
  -- Fill any gaps.
  for length = 1, max_length do
    D.lengths[length] = D.lengths[length] or 0
  end
  D.num_connected = D.num_connected + assert( D.__segments )
end

local function lambda( J )
  D.__river_tiles = {}
  D.__segments = 0

  D.savs = D.savs + 1

  Q.on_all_non_arctic_tiles( function( tile )
    D.tiles = D.tiles + 1
    local square = terrain_at( J, tile )

    local biome = square.ground -- water won't have this.
    if biome then
      D.biome_counts[biome] = D.biome_counts[biome] or 0
      D.biome_counts[biome] = D.biome_counts[biome] + 1
      if Q.has_river( J, tile ) then
        D.biome_river[biome] = D.biome_river[biome] or 0
        D.biome_river[biome] = D.biome_river[biome] + 1
      end
    end

    D.maj_by_row[tile.y] = D.maj_by_row[tile.y] or 0
    D.min_by_row[tile.y] = D.min_by_row[tile.y] or 0
    D.any_by_row[tile.y] = D.any_by_row[tile.y] or 0
    D.starts_by_row[tile.y] = D.starts_by_row[tile.y] or 0

    if square.surface == 'water' then
      D.water = D.water + 1

      local has_adjacent_land = false
      Q.on_non_arctic_surrounding_tiles_cardinal( tile,
                                                  function( coord )
        local adjacent = terrain_at( J, coord )
        if adjacent.surface == 'land' then
          has_adjacent_land = true
        end
      end )

      if has_adjacent_land then
        D.water_adjacent_to_land = D.water_adjacent_to_land + 1
      end
    end

    if square.surface == 'land' then D.land = D.land + 1 end

    if Q.has_river( J, tile ) then
      D.__river_tiles[Q.rastorize( tile )] = 0
      D.any = D.any + 1
      D.any_by_row[tile.y] = D.any_by_row[tile.y] + 1
      if Q.has_hills( J, tile ) then
        D.with_hills = D.with_hills + 1
      end
      if Q.has_minor_river( J, tile ) then
        D.min = D.min + 1
        D.min_by_row[tile.y] = D.min_by_row[tile.y] + 1
      end
      if Q.has_major_river( J, tile ) then
        D.maj = D.maj + 1
        D.maj_by_row[tile.y] = D.maj_by_row[tile.y] + 1
      end
      local rivers_surrounding = 0
      Q.on_surrounding_tiles_cardinal( tile, function( coord )
        if Q.has_river( J, coord ) then
          rivers_surrounding = rivers_surrounding + 1
        end
      end )
      if rivers_surrounding > 2 then
        D.any_forks = D.any_forks + 1
        if Q.has_minor_river( J, tile ) then
          D.min_forks = D.min_forks + 1
        end
        if Q.has_major_river( J, tile ) then
          D.maj_forks = D.maj_forks + 1
        end
      end
      if square.surface == 'water' then
        D.any_on_water = D.any_on_water + 1
        D.starts = D.starts + 1
        D.starts_by_row[tile.y] = D.starts_by_row[tile.y] + 1
        if Q.has_minor_river( J, tile ) then
          D.min_on_water = D.min_on_water + 1
        end
        if Q.has_major_river( J, tile ) then
          D.maj_on_water = D.maj_on_water + 1
        end
        if rivers_surrounding == 0 then
          D.water_islands = D.water_islands + 1
          D.any_islands = D.any_islands + 1
        end
      end
      if square.surface == 'land' then
        if rivers_surrounding == 0 then
          D.land_islands = D.land_islands + 1
          D.any_islands = D.any_islands + 1
        end
        if rivers_surrounding == 1 then
          D.ends = D.ends + 1
        end
      end
    end
  end )

  find_connected( J )
end

local DATA_KEY_ORDER = {
  'savs', --
  'tiles', --
  'land', --
  'water', --
  'water_adjacent_to_land', --
  'maj', --
  'min', --
  'any', --
  'maj_on_water', --
  'min_on_water', --
  'any_on_water', --
  'maj_forks', --
  'min_forks', --
  'any_forks', --
  'num_connected', --
  'num_connected_per_shore', --
  'num_with_water', --
  'num_without_water', --
  'starts', --
  'ends', --
  'land_islands', --
  'water_islands', --
  'any_islands', --
  'with_hills', --
  'biome_density', --
}

local function finished( mode )
  local o = { __key_order=DATA_KEY_ORDER }

  o.savs = D.savs
  o.tiles = D.tiles / D.savs

  o.land = D.land / D.savs
  o.water = D.water / D.savs
  o.water_adjacent_to_land = D.water_adjacent_to_land / D.savs

  o.maj = D.maj / D.savs
  o.min = D.min / D.savs
  o.any = D.any / D.savs

  o.maj_on_water = D.maj_on_water / D.savs
  o.min_on_water = D.min_on_water / D.savs
  o.any_on_water = D.any_on_water / D.savs

  o.maj_forks = D.maj_forks / D.savs
  o.min_forks = D.min_forks / D.savs
  o.any_forks = D.any_forks / D.savs

  o.num_connected = D.num_connected / D.savs
  o.num_connected_per_shore = D.num_connected /
                                  D.water_adjacent_to_land
  o.num_with_water = D.num_with_water / D.savs
  o.num_without_water = D.num_without_water / D.savs

  o.starts = D.starts / D.savs
  o.ends = D.ends / D.savs

  o.land_islands = D.land_islands / D.savs
  o.water_islands = D.water_islands / D.savs
  o.any_islands = D.any_islands / D.savs

  o.with_hills = D.with_hills / D.savs

  o.biome_density = { __key_order=BIOME_ORDERING }
  for biome, river_count in pairs( D.biome_river ) do
    assert( type( biome ) == 'string' )
    assert( type( river_count ) == 'number' )
    local total_count = assert( D.biome_counts[biome] )
    if total_count > 0 then
      o.biome_density[biome] = river_count / total_count
    end
  end

  do
    local file = format( '%s/%s.data.json', PLOTS_DIR, mode )
    printfln( 'writing file %s...', file )
    local f<close> = assert( io.open( file, 'w' ) )
    local emit = function( bit ) f:write( bit ) end
    json.write( o, 2, emit )
  end

  do
    local file = format( '%s/%s.rows.csv', PLOTS_DIR, mode )
    printfln( 'writing file %s...', file )
    local f<close> = assert( io.open( file, 'w' ) )
    local rows = {}
    for y = 2, 70 - 1 do
      local row = {
        y=y,
        ['maj-by-row']=assert( D.maj_by_row[y] ) / D.savs,
        ['min-by-row']=assert( D.min_by_row[y] ) / D.savs,
        ['any-by-row']=assert( D.any_by_row[y] ) / D.savs,
        ['starts-by-row']=assert( D.starts_by_row[y] ) / D.savs,
      }
      insert( rows, row )
    end
    local csv_opts = {
      fieldsToKeep={
        'y', --
        'any-by-row', --
        'min-by-row', --
        'maj-by-row', --
        'starts-by-row', --
      },
    }
    f:write( csv.encode( rows, csv_opts ) )
    write_gnuplot_file( mode, 'rows', {
      title='River Frequency by Row', --
      x_label='Y', --
      y_label='Count per Map', --
      x_range='1:70', --
      y_range='0:10', --
      mode=mode, --
      count=assert( D.savs ), --
    } )
  end

  do
    local file = format( '%s/%s.lengths.csv', PLOTS_DIR, mode )
    printfln( 'writing file %s...', file )
    local f<close> = assert( io.open( file, 'w' ) )
    local rows = {}
    for length = 1, #D.lengths do
      local count = D.lengths[length]
      local row = { length=length, count=count / D.savs }
      insert( rows, row )
    end
    local csv_opts = { fieldsToKeep={ 'length', 'count' } }
    f:write( csv.encode( rows, csv_opts ) )
    write_gnuplot_file( mode, 'lengths', {
      title='River Length Histogram', --
      x_label='Length', --
      y_label='Count per Map', --
      x_range='1:20', --
      y_range='0:20', --
      mode=mode, --
      count=assert( D.savs ), --
    } )
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
    'ttmm', 'tmmm', 'mtmm', 'mmmm', 'mbmm', 'bmmm', 'bbmm',
    'bbtt', 'bbmt', 'bbmb', 'bbtm', 'bbbm', 'bbbb', 'new',
  }
  local o = {}
  o.modes = {}
  o.modes.__key_order = MODES
  o.avg = {}
  o.avg.__key_order = DATA_KEY_ORDER
  o.avg.biome_density = {}
  o.avg.biome_density.__key_order = BIOME_ORDERING
  for _, mode in ipairs( MODES ) do
    local file = format( '%s/%s.data.json', PLOTS_DIR, mode )
    local f<close> = assert( io.open( file, 'r' ) )
    local mode_json = json.read( f:read( '*all' ) )
    local results = assert( mode_json )
    o.modes[mode] = results
    o.modes[mode].__key_order = DATA_KEY_ORDER
    o.modes[mode].biome_density.__key_order = BIOME_ORDERING
    for k, v in pairs( results ) do
      if type( v ) ~= 'table' then
        o.avg[k] = o.avg[k] or 0
        o.avg[k] = o.avg[k] + v
      end
    end
    for biome, v in pairs( results.biome_density ) do
      if biome == '__key_order' then goto continue end
      assert( type( biome ) == 'string' )
      assert( type( v ) == 'number' )
      o.avg.biome_density[biome] =
          o.avg.biome_density[biome] or 0
      o.avg.biome_density[biome] = o.avg.biome_density[biome] + v
      ::continue::
    end
  end
  for k, v in pairs( o.avg ) do
    if k == '__key_order' then goto continue end
    if k == 'biome_density' then goto continue end
    o.avg[k] = v / #MODES
    ::continue::
  end
  for k, v in pairs( o.avg.biome_density ) do
    if k == '__key_order' then goto continue end
    o.avg.biome_density[k] = v / #MODES
    ::continue::
  end
  local collected_json_file =
      format( '%s/rivers.json', PLOTS_DIR )
  printfln( 'writing %s...', collected_json_file )
  local json_out<close> = assert(
                              io.open( collected_json_file, 'w' ) )
  json.write( o, 2, function( bit ) json_out:write( bit ) end )
end

-----------------------------------------------------------------
-- Lambda.
-----------------------------------------------------------------
return { lambda=lambda, finished=finished, collect=collect }