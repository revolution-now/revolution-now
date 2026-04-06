-----------------------------------------------------------------
-- Imports.
-----------------------------------------------------------------
local plot = require( 'moon.plot' )
local mmath = require( 'moon.math' )

-----------------------------------------------------------------
-- Aliases.
-----------------------------------------------------------------
local format = string.format
local max = math.max
local log = math.log
local shuffle = assert( mmath.shuffle )
local insert = table.insert

-----------------------------------------------------------------
-- Constants.
-----------------------------------------------------------------
local ITERS = 10000000

local M = 30

local DRAW_MAPS = false

local PLOTS_DIR = 'prototype'

-----------------------------------------------------------------
-- Data.
-----------------------------------------------------------------
local CONFIG = {
  mountains={
    probability=.19, --
    max_length=60, --
    data={
      num_ranges=0,
      count_range_length={}, -- key=length
    },
  },
  hills={
    probability=.09, --
    max_length=60, --
    data={
      num_ranges=0,
      count_range_length={}, -- key=length
    },
  },
  clearing={
    probability=.40, --
    max_length=60, --
    data={
      num_ranges=0,
      count_range_length={}, -- key=length
    },
  },
}

-----------------------------------------------------------------
-- Helpers.
-----------------------------------------------------------------
local function coin_flip( p )
  assert( p >= 0 )
  assert( p <= 1.0 )
  return math.random() < p
end

-----------------------------------------------------------------
-- Implementation.
-----------------------------------------------------------------
local function plot_lengths()
  local opts = {
    title=format( 'Range Length Histogram (prototype) [%d]',
                  ITERS ),
    x_label='Length (cardinal adjacent)',
    y_label='Frequency',
    x_range='1:30',
    y_range='-20:0',
  }
  local csv_data = {
    header={ 'length', 'mountains', 'hills', 'clearing' },
    rows={},
  }
  local max_length = 0
  for _, kind in ipairs{ 'mountains', 'hills', 'clearing' } do
    for length, count in pairs( CONFIG[kind].data
                                    .count_range_length ) do
      if count > 0 then
        max_length = max( max_length, length )
      end
    end
  end
  local length_1_val = {
    mountains=1, --
    hills=1, --
    clearing=1, --
  }
  for i = 1, max_length do
    local row = { i, 0, 0, 0 }

    if CONFIG.mountains.data.num_ranges > 0 then
      CONFIG.mountains.data.count_range_length[i] =
          CONFIG.mountains.data.count_range_length[i] or 0
      row[2] = log( CONFIG.mountains.data.count_range_length[i] /
                        CONFIG.mountains.data.num_ranges )
      if i == 1 then length_1_val.mountains = row[2] end
      row[2] = row[2] - length_1_val.mountains
    end

    if CONFIG.hills.data.num_ranges > 0 then
      CONFIG.hills.data.count_range_length[i] =
          CONFIG.hills.data.count_range_length[i] or 0
      row[3] = log( CONFIG.hills.data.count_range_length[i] /
                        CONFIG.hills.data.num_ranges )
      if i == 1 then length_1_val.hills = row[3] end
      row[3] = row[3] - length_1_val.hills
    end

    if CONFIG.clearing.data.num_ranges > 0 then
      CONFIG.clearing.data.count_range_length[i] =
          CONFIG.clearing.data.count_range_length[i] or 0
      row[4] = log( CONFIG.clearing.data.count_range_length[i] /
                        CONFIG.clearing.data.num_ranges )
      if i == 1 then length_1_val.clearing = row[4] end
      row[4] = row[4] - length_1_val.clearing

      table.insert( csv_data.rows, row )
    end
  end
  local path = format( '%s/lengths.gnuplot', PLOTS_DIR )
  plot.line_graph_to_file( path, csv_data, opts )
end

local function draw_map( map )
  local function bar()
    io.write( '+' )
    io.write( string.rep( '-', M ) )
    io.write( '+\n' )
  end
  bar()
  for y = 1, M do
    io.write( '|' )
    for x = 1, M do
      local val = map[y][x]
      if val then
        io.write( 'X' )
      else
        io.write( '.' )
      end
    end
    io.write( '|\n' )
  end
  bar()
end

local function exists( tile )
  assert( type( tile ) == 'table' )
  if tile.x < 1 or tile.y < 1 or tile.x > M or tile.y > M then
    return false
  end
  return true
end

local function find_adjacent_free_tile( map, tile )
  assert( tile )
  assert( tile.x )
  assert( tile.y )
  local try = {
    { w=-1, h=0 }, --
    { w=1, h=0 }, --
    { w=0, h=-1 }, --
    { w=0, h=1 }, --
  }
  shuffle( try )
  for _, delta in ipairs( try ) do
    local moved = { x=tile.x + delta.w, y=tile.y + delta.h }
    if not exists( moved ) then goto continue end
    if not map[moved.y][moved.x] then return moved end
    ::continue::
  end
  return nil
end

local function coords_that_can_grow( map )
  local res = {}
  for y = 1, M do
    for x = 1, M do
      if map[y][x] then
        if find_adjacent_free_tile( map, { x=x, y=y } ) then
          insert( res, { x=x, y=y } )
        end
      end
    end
  end
  return res
end

local function grow_tile( map, tile )
  local moved = find_adjacent_free_tile( map, tile )
  assert( moved )
  assert( moved.x )
  assert( moved.y )
  map[moved.y][moved.x] = true
end

local function run_growth_single( name )
  local config = assert( CONFIG[name] )
  local D = assert( config.data )
  local P = assert( config.probability )
  local max_length = assert( config.max_length )
  local map = {}
  for y = 1, M do
    map[y] = {}
    for x = 1, M do map[y][x] = false end
  end
  local length = 1
  local start_x = M / 2
  local start_y = M / 2
  map[start_y][start_x] = true
  while length < max_length do
    local can_grow = coords_that_can_grow( map )
    if #can_grow == 0 then break end
    shuffle( can_grow )
    local grew = false
    for _, tile in ipairs( can_grow ) do
      local probability = P
      local should_grow = coin_flip( probability )
      if not should_grow then goto continue end
      length = length + 1
      grow_tile( map, tile )
      grew = true
      do break end
      ::continue::
    end
    if not grew then break end
  end
  D.num_ranges = D.num_ranges + 1
  D.count_range_length[length] =
      D.count_range_length[length] or 0
  D.count_range_length[length] = D.count_range_length[length] + 1
  if DRAW_MAPS then draw_map( map ) end
end

local function run_growth()
  print( 'running mountains...' )
  for _ = 1, ITERS do run_growth_single( 'mountains' ) end
  print( 'running hills...' )
  for _ = 1, ITERS do run_growth_single( 'hills' ) end
  print( 'running clearing...' )
  for _ = 1, ITERS / 10 do run_growth_single( 'clearing' ) end
end

-----------------------------------------------------------------
-- Main.
-----------------------------------------------------------------
local function main()
  math.randomseed( os.time() )
  run_growth()
  plot_lengths()
end

main()
