-- This lambda will run on multiple savs and will gather some
-- statistics on the density of land as a function of x/y.
-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local Q = require( 'lib.query' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local insert = table.insert
local floor = math.floor
local sort = table.sort
local format = string.format
local concat = table.concat

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function printfln( ... ) print( string.format( ... ) ) end

local PLOTS_DIR = 'land-density/empirical'

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

set yrange [{{YRANGE}}]
set xrange [{{XRANGE}}]

# Plot: x is column 1, then plot columns 2..N as separate lines.
plot for [col=2:*] "{{CSV_STEM}}" using 1:col with lines lw 2
]]

local function write_spatial_gnuplot_file( csv_fname, mode, count )
  assert( mode )
  assert( count )
  local gnuplot_fname = format( '%s/%s.spatial.gnuplot',
                                PLOTS_DIR, mode )
  printfln( 'writing gnuplot file %s...', gnuplot_fname )
  local f<close> = assert( io.open( gnuplot_fname, 'w' ) )
  local body = GNUPLOT_FILE_TEMPLATE
  local subs = {
    { key='{{TITLE}}', val='Spatial Land Density' },
    { key='{{CSV_STEM}}', val=csv_fname }, --
    { key='{{MODE}}', val=mode }, --
    { key='{{COUNT}}', val=count }, --
    { key='{{XRANGE}}', val='0:1.0' }, --
    { key='{{YRANGE}}', val='0:1.0' }, --
    { key='{{XLABEL}}', val='X or Y coordinate' }, --
    { key='{{YLABEL}}', val='density' }, --
  }
  for _, p in ipairs( subs ) do
    body = body:gsub( assert( p.key ), assert( p.val ) )
  end
  f:write( body )
end

local function write_overall_gnuplot_file( csv_fname, mode, count )
  assert( mode )
  assert( count )
  local gnuplot_fname = format( '%s/%s.overall.gnuplot',
                                PLOTS_DIR, mode )
  printfln( 'writing gnuplot file %s...', gnuplot_fname )
  local f<close> = assert( io.open( gnuplot_fname, 'w' ) )
  local body = GNUPLOT_FILE_TEMPLATE
  local subs = {
    { key='{{TITLE}}', val='Overall Land Density' },
    { key='{{CSV_STEM}}', val=csv_fname }, --
    { key='{{MODE}}', val=mode }, --
    { key='{{COUNT}}', val=count }, --
    { key='{{XRANGE}}', val='0:0.5' }, --
    { key='{{YRANGE}}', val='0:*' }, --
    { key='{{XLABEL}}', val='density' }, --
    { key='{{YLABEL}}', val='frequency' }, --
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
  land_no_arctic=0,
  land_by_col={},
  land_by_row={},
  arctic_by_row={},
  overall_density={},
}

-----------------------------------------------------------------
-- Lambda.
-----------------------------------------------------------------
-- Run once per sav.
local function lambda( json )
  D.total_savs = D.total_savs + 1
  local total_land_no_arctic = 0
  Q.on_all_tiles( function( tile )
    D.land_by_col[tile.x] = D.land_by_col[tile.x] or 0
    D.land_by_row[tile.y] = D.land_by_row[tile.y] or 0
    D.arctic_by_row[tile.y] = D.arctic_by_row[tile.y] or 0
    local terrain = Q.terrain_at( json, tile )
    if terrain.surface ~= 'land' then goto continue end
    -- Exclude arctic from density stats.
    if tile.y == 1 or tile.y == 70 then
      D.arctic_by_row[tile.y] = D.arctic_by_row[tile.y] + 1
      assert( terrain.ground == 'arctic' )
    else
      total_land_no_arctic = total_land_no_arctic + 1
      D.land_no_arctic = D.land_no_arctic + 1
      D.land_by_col[tile.x] = D.land_by_col[tile.x] + 1
      D.land_by_row[tile.y] = D.land_by_row[tile.y] + 1
    end
    ::continue::
  end )
  local overall_density = total_land_no_arctic / (56 * (70 - 2))
  local overall_density_bucket =
      floor( overall_density * 100 ) / 100
  D.overall_density[overall_density_bucket] =
      D.overall_density[overall_density_bucket] or 0
  D.overall_density[overall_density_bucket] =
      D.overall_density[overall_density_bucket] + 1
end

-----------------------------------------------------------------
-- Final.
-----------------------------------------------------------------
local function write_spatial_density( mode )
  local csv_fname = format( '%s.spatial.csv', mode )
  local csv_path = format( '%s/%s', PLOTS_DIR, csv_fname )
  write_spatial_gnuplot_file( csv_fname, mode, D.total_savs )
  printfln( 'writing csv file %s...', csv_path )
  local f<close> = assert( io.open( csv_path, 'w' ) )
  local emit = function( fmt, ... )
    f:write( format( fmt, ... ) )
  end
  local header = { 'coordinate', 'x', 'y', 'overall', 'arctic' }
  emit( '%s\n', concat( header, ',' ) )
  local density = D.land_no_arctic /
                      (56 * (70 - 2) * D.total_savs)
  for p = 0, 1, .001 do
    emit( '%.3f', p )
    local x = floor( p * 56 + 1 )
    local y = floor( p * 70 + 1 )
    assert( x >= 1 )
    assert( y >= 1 )
    assert( x <= 56 )
    assert( y <= 70 )
    local count_x = assert( D.land_by_col[x] )
    local count_y = assert( D.land_by_row[y] )
    -- x means "by column", and there are 70 tiles in a column.
    -- y means "by row", and there are 56 tiles in a row.
    local density_x = count_x / ((70 - 2) * D.total_savs)
    local density_y = count_y / (56 * D.total_savs)
    local arctic_density = assert( D.arctic_by_row[y] ) /
                               (56 * D.total_savs)
    emit( ',%f,%f,%f,%f', density_x, density_y, density,
          arctic_density )
    emit( '\n' )
  end
end

local function write_overall_density( mode )
  local csv_fname = format( '%s.overall.csv', mode )
  local csv_path = format( '%s/%s', PLOTS_DIR, csv_fname )
  write_overall_gnuplot_file( csv_fname, mode, D.total_savs )
  printfln( 'writing csv file %s...', csv_path )
  local f<close> = assert( io.open( csv_path, 'w' ) )
  local emit = function( fmt, ... )
    f:write( format( fmt, ... ) )
  end
  local header = { 'density', 'frequency' }
  emit( '%s\n', concat( header, ',' ) )
  local sorted = {}
  for k, v in pairs( D.overall_density ) do
    -- 100 because our bucket size is .01
    insert( sorted,
            { bucket=k, frequency=100 * v / D.total_savs } )
  end
  sort( sorted, function( l, r )
    return assert( l.bucket ) < assert( r.bucket )
  end )
  for _, pair in ipairs( sorted ) do
    emit( '%f,%f\n', assert( pair.bucket ),
          assert( pair.frequency ) )
  end
end

-- Run once when finished alls avs.
local function finished( mode )
  write_spatial_density( mode )
  write_overall_density( mode )
end

-----------------------------------------------------------------
-- Lambda.
-----------------------------------------------------------------
return { lambda=lambda, finished=finished }