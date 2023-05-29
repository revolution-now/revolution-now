--[[ ------------------------------------------------------------
|
| land-partition.lua
|
| Project: Revolution Now
|
| Created by dsicilia on 2022-10-30.
|
| Description: Partitions the map into segments for placement
|              of the natives.
|
--]] ------------------------------------------------------------
local M = {}

local function land_coords_above_below_line( coords, line )
  local below = {}
  local above = {}
  for _, coord in ipairs( coords ) do
    if coord.x * line.m + line.b < coord.y then
      table.insert( below, coord )
    else
      table.insert( above, coord )
    end
  end
  return above, below
end

local function land_coords_left_right_of_line( coords, line )
  local left = {}
  local right = {}
  for _, coord in ipairs( coords ) do
    if coord.y * line.m + line.b > coord.x then
      table.insert( left, coord )
    else
      table.insert( right, coord )
    end
  end
  return left, right
end

local function log( fmt, ... )
  -- io.write( string.format( fmt .. '\n', ... ) )
end

local VARIATION = .4
assert( VARIATION < .5 )

local function split_vertical( land_coords, world_size )
  local x_min = 0
  local x_max = world_size.w - 1
  for _, coord in ipairs( land_coords ) do
    x_min = math.min( x_min, coord.x )
    x_max = math.max( x_max, coord.x )
  end
  assert( x_max >= x_min )

  local x_top
  local x_bottom
  local scale = (1.0 - VARIATION) + math.random() *
                    (VARIATION * 2)
  for iters = 1, 1000 do
    log( 'iter 1' )
    local iters = 0
    repeat
      iters = iters + 1
      if iters > 100 then return land_coords, {} end
      x_top = math.random( x_min, x_max )
      x_bottom = x_min
      local line = { m=(x_bottom - x_top) / world_size.h,
                     b=x_top }
      local left, right = land_coords_left_right_of_line(
                              land_coords, line )
      log( 'iter 2: left=%d, right=%d', #left, #right )
    until #right > #left * scale
    for i = 1, x_max - x_min do
      local line = { m=(x_bottom - x_top) / world_size.h,
                     b=x_top }
      local left, right = land_coords_left_right_of_line(
                              land_coords, line )
      log( 'iter 3: left=%d, right=%d', #left, #right )
      if #right < #left * scale then return left, right end
      x_bottom = x_bottom + 1
    end
  end
  -- We weren't able to split; typically because the map is too
  -- small.
  return land_coords, {}
end

local function split_horizontal( land_coords, world_size )
  local y_min = 0
  local y_max = world_size.h - 1
  for _, coord in ipairs( land_coords ) do
    y_min = math.min( y_min, coord.y )
    y_max = math.max( y_max, coord.y )
  end
  assert( y_max >= y_min )

  local y_left
  local y_right
  local scale = (1.0 - VARIATION) + math.random() *
                    (VARIATION * 2)
  for iters = 1, 1000 do
    log( 'iter a' )
    local iters = 0
    repeat
      iters = iters + 1
      if iters > 100 then return land_coords, {} end
      y_left = math.random( y_min, y_max )
      y_right = y_min
      local line = {
        m=(y_right - y_left) / world_size.w,
        b=y_left,
      }
      local above, below = land_coords_above_below_line(
                               land_coords, line )
      log( 'iter b: above=%d, below=%d', #above, #below )
    until #below > #above * scale
    for i = 1, y_max - y_min do
      local line = {
        m=(y_right - y_left) / world_size.w,
        b=y_left,
      }
      local above, below = land_coords_above_below_line(
                               land_coords, line )
      log( 'iter c: above=%d, below=%d', #above, #below )
      if #below < #above * scale then return above, below end
      y_right = y_right + 1
    end
  end
  -- We weren't able to split; typically because the map is too
  -- small.
  return land_coords, {}
end

local function set_coords_to_partition(output, world_size,
                                       land_coords, n )
  for _, coord in ipairs( land_coords ) do
    output[coord.y * world_size.w + coord.x] = n
  end
end

local function do_split(output, split_funcs, n_range,
                        land_coords, world_size )
  if (n_range.min == n_range.max) then
    set_coords_to_partition( output, world_size, land_coords,
                             n_range.min )
    return
  end
  local side, other_side = split_funcs.func( land_coords,
                                             world_size )
  local min = n_range.min
  local max = n_range.max
  local range = { min=min, max=min + (max - min) // 2 }
  local other_range = { min=min + (max - min) // 2 + 1, max=max }
  do_split( output, split_funcs.next, range, side, world_size )
  do_split( output, split_funcs.next, other_range, other_side,
            world_size )
end

function M.generate( world_size, partitions, has_land )
  local land_coords = {}
  for y = 0, world_size.h - 1 do
    for x = 0, world_size.w - 1 do
      if has_land{ x=x, y=y } then
        table.insert( land_coords, { x=x, y=y } )
      end
    end
  end

  -- Holds a mapping from (rasterized) coordinate to partition
  -- number.
  local res = {}
  local split_funcs = {
    func=split_horizontal,
    next={ func=split_vertical, next={ func=split_horizontal } },
  }
  do_split( res, split_funcs, { min=0, max=partitions - 1 },
            land_coords, world_size )
  return res
end

return M
