-- This lambda will run on multiple savs and will gather some
-- statistics on the distribution of ground terrains.
-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local Q = require( 'lib.query' )

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

local PLOTS_DIR = 'gamegen/plots'

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
local D = { total_savs=0, land={}, ground={} }

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
local function lambda( json )
  D.total_savs = D.total_savs + 1
  for _, ground_type in ipairs( GROUND_TYPES ) do
    D.ground[ground_type] = D.ground[ground_type] or {}
  end
  Q.on_all_tiles( function( tile )
    for _, ground_type in ipairs( GROUND_TYPES ) do
      D.land[tile.y] = D.land[tile.y] or 0
      local o = assert( D.ground[ground_type] )
      o[tile.y] = o[tile.y] or 0
    end
  end )
  Q.on_all_tiles( function( tile )
    local terrain = Q.terrain_at( json, tile )

    D.land[tile.y] = D.land[tile.y] or 0
    if terrain.surface == 'land' then
      D.land[tile.y] = D.land[tile.y] + 1
    else
      return
    end

    local o = assert( D.ground[terrain.ground],
                      format( 'terrain.ground=%s not found',
                              tostring( terrain.ground ) ) )
    o[tile.y] = o[tile.y] + 1
  end )
end

local function finished( mode )
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
      local count = assert( D.ground[ground][y] )
      sum = sum + count
      local density = count / land
      emit( ',%f', density )
    end
    if INCLUDE_SUM then emit( ',%f', sum / land ) end
    emit( '\n' )
  end
end

-----------------------------------------------------------------
-- Lambda.
-----------------------------------------------------------------
return { lambda=lambda, finished=finished }